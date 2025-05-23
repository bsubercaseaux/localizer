import time
from subprocess import TimeoutExpired, check_output, CalledProcessError, STDOUT
from termcolor import colored
import argparse
import build_benchmarks
import validator
import sys
import statistics

argparser = argparse.ArgumentParser()
argparser.add_argument("-b", "--build", help="Build benchmarks", action="store_true")
argparser.add_argument("-L", type=int, default=20, help="Lower bound for N points")
argparser.add_argument("-R", type=int, default=25, help="Upper bound for N points")
argparser.add_argument(
    "-n", type=int, default=5, help="Number of iterations for each N"
)
argparser.add_argument(
    "-t", "--timeout", type=int, default=10, help="Timeout for each iteration [seconds]"
)
argparser.add_argument(
    "-o", "--output", type=str, default="output.txt", help="Output file"
)
argparser.add_argument("-r", "--reset", type=int, default=30000, help="reset parameter")
argparser.add_argument(
    "-i",
    "--iterations",
    type=int,
    default=10,
    help="number of sub-iterations for realizer",
)

args = argparser.parse_args()


def system_call(command, timeout=None):
    """
    params:
        command: list of strings, ex. ["ls", "-l"]
        timeout: number of seconds to wait for the command to complete.
    returns: output, return_code
    """
    try:
        cmd_output = check_output(command, stderr=STDOUT, timeout=timeout).decode()
        return_code = 0
    except CalledProcessError as e:
        cmd_output = e.output.decode()
        return_code = e.returncode
    except TimeoutExpired:
        cmd_output = f"Command timed out after {timeout} seconds"
        return_code = (
            -1
        )  # You can use any number that is not a valid return code for a success or normal failure
    return cmd_output, return_code


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
for N in range(args.L, args.R + 1):
    DATA[N] = []
    COMPLETED[N] = 0
    for i in range(args.n):
        constraints_filename = f"benchmarks/{N}/{i}.txt"
        output_filename = f"benchmarks/{N}/{i}.output"
        output = timed_run_shell(
            [
                "./realizer",
                constraints_filename,
                "-i",
                str(args.iterations),
                "-r",
                str(args.reset),
                "-t",
                "8",
                "-o",
                output_filename,
            ],
            timeout=args.timeout,
        )
        DATA[N].append(output)
        if output[2] / 1e9 < args.timeout:
            try:
                if not validator.validate(constraints_filename, output_filename):
                    print(
                        f"Error in validation!! {colored('N', 'cyan')} = {N}, {colored('i', 'cyan')} = {i})"
                    )
                    sys.exit(1)
                COMPLETED[N] += 1
            except Exception as e: 
                print("EXCEPTION", e)
                print(output[1], output[0])
                sys.exit(1)

            print(
                f'{colored("N", "cyan")} = {N}, i = {i}, time = {output[2]/1e6:.2f} ms'
            )
        else:
            print(f'{colored("N", "cyan")} = {N}, i = {i}, {colored("timeout", "red")}')
    print(f'{colored("--------------------", "green")}')

print(colored("\nSaving results to: ", "yellow"), args.output, "\n")
with open(args.output, "w", encoding="utf-8") as f:
    for N, data_N in DATA.items():
        # data_N_filtered = [t for t in data_N if t[2] / 1e9 < args.timeout]
        for t in data_N:
            f.write(
                f"{N} {t[2]/1e9:.2f}\n")
        # if len(data_N_filtered) == 0:
        #     f.write(f"{N} -1 {COMPLETED[N]}/{len(data_N)} 0\n")
        #     continue
        # avg = sum(t[2] for t in data_N_filtered) / len(data_N_filtered)
        # print(
        #     f'{colored("N", "cyan")} = {N}, ' \
        #     f'{colored("avg time", "yellow")} = {avg/1e9:.2f}ms, ' \
        #     f'{colored("completed", "green")} = {COMPLETED[N]}/{len(data_N)} '
        # )
        # f.write(f"{N} {avg/1e9:.2f} {COMPLETED[N]}/{len(data_N)} {statistics.stdev([t[2]/1e6 for t in data_N_filtered])}\n")
