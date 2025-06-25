#include "Vcpu.h"
#include "Vaccelerator.h"
#include "defs.h"

#include <fstream>
#include <iostream>

using namespace std;

static uint8_t mem[0x02000000];
static Vcpu *top;
static Vaccelerator *acc;

#define LOGV(val) (cout << #val << ":" << (val) << " ")

// to make the linker happy
double sc_time_stamp() { return 0; }

static void step(int n = 1)
{
    for (int i = 0; i < n; i++)
    {
        top->clk = 0;
        top->eval();
        top->clk = 1;
        top->eval();
    }
}

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);

    top = new Vcpu;
    acc = new Vaccelerator;

    // Load program
    {
        ifstream fi("simulated/main.bin");
        fi.read((char *)(mem + ROM_START), sizeof(mem) - ROM_START);
    }

    // Initialize CPU
    top->clk = 0;
    top->resetn = 0;
    step();
    top->resetn = 1;
    step();

    // Simulation

    cout << hex;

    int acc_connect = 0;

    for (;;)
    {
        // rising edge
        top->clk = 1;
        top->eval();
        acc->mem_valid = top->mem_valid;
        acc->mem_addr = top->mem_addr;
        acc->mem_wdata = top->mem_wdata;
        acc->mem_wstrb = top->mem_wstrb;
        acc->clk = 1;
        acc->eval();

        // falling edge
        top->clk = 0;
        top->eval();
        acc->mem_valid = top->mem_valid;
        acc->mem_addr = top->mem_addr;
        acc->mem_wdata = top->mem_wdata;
        acc->mem_wstrb = top->mem_wstrb;
        acc->clk = 0;
        acc->eval();

        // Handle memory operations
        if (top->mem_valid)
        {
            // LOGV(top->mem_addr);
            // LOGV((int)top->mem_wstrb);
            // LOGV(top->mem_wdata);
            // cout << endl;
            // cout << "[]"
            uint32_t addr = top->mem_addr;
            if (addr >= sizeof(mem))
            {
                cerr << "Error: address out of range: " << addr << endl;
                throw out_of_range("Memory address out of range");
            }

            if (top->mem_wstrb)
            {
                // Writing to memory
                uint32_t data = top->mem_wdata;
                // if (addr != IO_UART0) {
                //     cout << "[SIM:Writing to address " << addr << "]" << endl;
                // }

                if (addr == IO_UART0)
                {
                    cout << (char)data << flush;
                    top->mem_ready = 1;
                }
                else if (addr == IO_EXIT)
                {
                    cout << "Program exited with code " << (int)data << endl;
                    break;
                }
                else if (addr >= IO_ACC_START && addr < IO_ACC_END)
                {
                    acc_connect = 1;
                }
                else
                {
                    // Write to memory
                    uint32_t write_mask = 0;
                    for (int bit = 0; bit < 4; bit++)
                        if (top->mem_wstrb & (1 << bit))
                            write_mask |= (0xFF << (bit * 8));
                    uint32_t write_data = data & write_mask;

                    uint32_t old_data = *(uint32_t *)(mem + addr) & ~write_mask;

                    uint32_t word_data = write_data | old_data;

                    *(uint32_t *)(mem + addr) = word_data;
                    top->mem_ready = 1;
                }
            }
            else
            {
                // Reading data
                if (addr >= IO_ACC_START && addr < IO_ACC_END)
                {
                    acc_connect = 1;
                }
                else
                {
                    uint32_t word_data = *(uint32_t *)(mem + addr);
                    top->mem_rdata = word_data;
                    top->mem_ready = 1;
                }
            }
        }
        else
        {
            top->mem_ready = 0;
        }

        if (acc_connect)
        {
            // LOGV(acc->mem_addr);
            // LOGV((int)acc->mem_valid);
            // LOGV((int)acc->mem_wstrb);
            // LOGV(acc->mem_wdata);
            // cout << endl;
        }
        if (acc_connect && acc->mem_ready)
        {
            if (!top->mem_wstrb) {
                // LOGV(acc->mem_addr);
                // LOGV(acc->mem_rdata);
                // cout << endl;
            }
            top->mem_ready = 1;
            top->mem_rdata = acc->mem_rdata;
            acc_connect = 0;
        }
    }

    delete top;
    delete acc;
    return 0;
}
