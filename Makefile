RVARCH ?= rv32im
RVABI ?= ilp32
RVGCC ?= riscv-none-elf-
RVLDSCRIPT ?= sim.ld
RVROOT = $(shell whereis -b $(RVGCC)gcc | cut -d : -f 2)
RVLIBPATH = $(shell ls -d `dirname $(RVROOT)`/../*/lib/$(RVARCH)/$(RVABI))
RVGCCPATH = $(shell ls -d `dirname $(RVROOT)`/../lib/gcc/*/*/$(RVARCH)/$(RVABI))

ACC_ROWS ?= 4
ACC_COLS ?= 4
ACC_INPUT_WIDTH ?= 8
ACC_OUTPUT_WIDTH ?= 16

DEBUG ?= 1

TEST_OTHER_METHODS ?= 1
TEST_ACCELERATOR ?= 1

all: a.out simulated/main.bin

synth: hardware/accelerator.v
	yosys synthesis/synth.ys

# simulator stuff

obj_dir/libVcpu.a: hardware/cpu.v hardware/picorv32.v
	CXXFLAGS=-std=c++17 verilator --cc --build --top-module cpu -Ihardware cpu.v --quiet-stats

obj_dir/libVaccelerator.a: hardware/accelerator.v
	CXXFLAGS=-std=c++17 verilator --cc --build --top-module accelerator -Ihardware accelerator.v -GR=$(ACC_ROWS) -GS=$(ACC_COLS) -GINPUT_WIDTH=$(ACC_INPUT_WIDTH) -GRESULT_WIDTH=$(ACC_OUTPUT_WIDTH) --quiet-stats

a.out: obj_dir/libVcpu.a obj_dir/libVaccelerator.a simulator/* include/*
	g++ -std=c++17 simulator/simulator.cpp -Iobj_dir -I/opt/oss-cad-suite/share/verilator/include -Iinclude -Lobj_dir -lVcpu -lVaccelerator -lverilated

# simulated stuff

%.o: %.S
	$(RVGCC)as -march=$(RVARCH) -mabi=$(RVABI) $^ -o $@

simulated/lib.o: simulated/lib.c simulated/lib.h
	$(RVGCC)g++ -Os -fno-pic -march=$(RVARCH) -mabi=$(RVABI) -fno-stack-protector -w -Wl,--no-relax -c simulated/lib.c -o simulated/lib.o -Iinclude

simulated/liblib.a: simulated/lib.o include/*
	$(RVGCC)ar rcs simulated/liblib.a simulated/lib.o
	$(RVGCC)ranlib simulated/liblib.a

simulated/main.elf: simulated/liblib.a simulated/main.cpp include/* simulated/crtrv.o simulated/$(RVLDSCRIPT)
	$(RVGCC)g++ -O2 -g -Iinclude -Ithird_party -fno-pic -march=$(RVARCH) -mabi=$(RVABI) -fno-stack-protector -w -Wl,--no-relax -c simulated/main.cpp -o simulated/main.o -DACC_INPUT_WIDTH=$(ACC_INPUT_WIDTH) -DACC_OUTPUT_WIDTH=$(ACC_OUTPUT_WIDTH) -DDEBUG=$(DEBUG) -DTEST_OTHER_METHODS=$(TEST_OTHER_METHODS) -DTEST_ACCELERATOR=$(TEST_ACCELERATOR)
	$(RVGCC)ld -m elf32lriscv -b elf32-littleriscv --no-relax -Tsimulated/$(RVLDSCRIPT) \
		simulated/main.o -o simulated/main.elf -Lsimulated -llib \
		-L$(RVLIBPATH) -lsupc++ -lstdc++ -lc -lm \
		-L$(RVGCCPATH) -lgcc 
	$(RVGCC)strip -d simulated/main.elf

simulated/main.bin: simulated/main.elf
	$(RVGCC)objcopy simulated/main.elf simulated/main.bin -O binary
	chmod -x simulated/main.bin

clean:
	rm -rf obj_dir simulated/*.o simulated/*.a simulated/*.elf simulated/*.bin a.out

run: all
	./a.out
