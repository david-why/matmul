all: a.out

obj_dir/Vcpu.h: hardware/cpu.v hardware/picorv32.v
	CXXFLAGS=-std=c++17 verilator --cc --build --top-module cpu -Ihardware cpu.v

a.out: obj_dir/Vcpu.h
	g++ -std=c++17 simulator/simulator.cpp -Iobj_dir -I/opt/oss-cad-suite/share/verilator/include -Lobj_dir -lVcpu -lverilated

clean:
	rm -rvf obj_dir

run: all
	@./a.out
