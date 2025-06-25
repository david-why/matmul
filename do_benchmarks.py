import os
from sys import exit

def system(cmd: str):
    code = os.system(cmd)
    if code:
        exit(code)

for acc_in in [8, 16, 32, 64]:
    for acc_out in [acc_in, acc_in * 2]:
        print((acc_in, acc_out))
        system('touch simulated/main.cpp')
        system(f'make -s run DEBUG=0 TEST_ACCELERATOR=0 >bench/bench_in_{acc_in//8}_out_{acc_out//8}_other.json')
        for acc_rows in [4, 8, 16]:
            for acc_cols in [4, 8, 16]:
                print((acc_in, acc_out, acc_rows, acc_cols))
                outf = f'bench/bench_in_{acc_in//8}_out_{acc_out//8}_{acc_rows}x{acc_cols}.json'
                if os.path.exists(outf) and os.stat(outf).st_size > 10:
                    continue
                system('touch hardware/accelerator.v')
                system('touch simulated/main.cpp')
                res = os.system(f'make -s run ACC_ROWS={acc_rows} ACC_COLS={acc_cols} ACC_INPUT_WIDTH={acc_in} ACC_OUTPUT_WIDTH={acc_out} DEBUG=0 TEST_OTHER_METHODS=0 >{outf}')
                if res:
                    exit(res)
