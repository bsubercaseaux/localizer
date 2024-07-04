import time
from subprocess import TimeoutExpired, check_output, CalledProcessError, STDOUT
from termcolor import colored
import argparse 
import build_benchmarks

argparser = argparse.ArgumentParser()
argparser.add_argument("-b", "--build", help="Build benchmarks", action="store_true")
argparser.add_argument("-L", type=int, default=20, help="Lower bound for N points")
argparser.add_argument("-R", type=int, default=25, help="Upper bound for N points")
argparser.add_argument("-n", type=int, default=5, help="Number of iterations for each N")
argparser.add_argument("-t", "--timeout", type=int, default=10, help="Timeout for each iteration [seconds]")
argparser.add_argument("-o", "--output", type=str, default="output.txt", help="Output file")
argparser.add_argument("-r", "--reset", type=float, default="0.25", help="reset parameter")
argparser.add_argument("-i", "--iterations", type=int, default=10, help="number of sub-iterations for realizer")

args = argparser.parse_args()

def system_call(command, timeout=None):
    """
    params:
        command: list of strings, ex. ["ls", "-l"]
        timeout: number of seconds to wait for the command to complete.
    returns: output, return_code
    """
    try:
        output = check_output(command, stderr=STDOUT, timeout=timeout).decode()
        return_code = 0
    except CalledProcessError as e:
        output = e.output.decode()
        return_code = e.returncode
    except TimeoutExpired:
        output = f"Command timed out after {timeout} seconds"
        return_code = (
            -1
        )  # You can use any number that is not a valid return code for a success or normal failure
    return output, return_code

def timed_run_shell(commands, timeout=None):
    """
    params:
        command: list of strings, ex. ["ls", "-l"]
        timeout: number of seconds to wait for the command to complete.
    returns: output, return_code
    """
    start_time = time.perf_counter_ns()
    output, return_code = system_call(commands, timeout=timeout)
    elapsed_time = time.perf_counter_ns() - start_time
    return output, return_code, elapsed_time

## Build benchmarks first
if args.build:
    build_benchmarks.build_benchmarks(args.L, args.R, args.n)

## Run 
DATA = {}
COMPLETED = {}
for N in range(args.L, args.R+1):
    DATA[N] = []
    COMPLETED[N] = 0
    for i in range(args.n):
        output = timed_run_shell(['./realizer', f'benchmarks/{N}/{i}.txt', "-i", str(args.iterations), "-s" , "412", "-r", str(args.reset)], timeout=args.timeout)
        DATA[N].append(output)
        if output[2]/1e9 < args.timeout:
            COMPLETED[N] += 1
            
        print(f'{colored("N", "cyan")} = {N}, i = {i}, time = {output[2]/1e6:.2f} ms')
    print(f'{colored("--------------------", "green")}')
        
print(colored('\nSaving results to: ', 'yellow'), args.output, "\n")
with open(args.output, 'w') as f:
    for N in DATA:
        avg = sum(t[2] for t in DATA[N]) / len(DATA[N])
        print(f'{colored("N", "cyan")} = {N}, {colored("avg time", "yellow")} = {avg/1e6:.2f}ms, {colored("completed", "green")} = {COMPLETED[N]}/{len(DATA[N])} ')
        f.write(f'{N} {avg/1e6:.2f} {COMPLETED[N]}/{len(DATA[N])}\n')