#include "Vcpu.h"

#include <fstream>

using namespace std;

static uint8_t mem[0x02000000];
static Vcpu *top;

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

    // Load program
    {
        ifstream fi("simulated/main.bin");
        fi >> mem;
    }

    // Initialize CPU
    top->clk = 0;
    top->resetn = 0;
    step();
    top->resetn = 1;
    step();

    return 0;
}
