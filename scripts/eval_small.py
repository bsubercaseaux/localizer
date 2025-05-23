import time
from subprocess import TimeoutExpired, check_output, CalledProcessError, STDOUT
from termcolor import colored
import argparse
import build_benchmarks
import validator
import sys

argparser = argparse.ArgumentParser()

argparser.add_argument(
    "-n", type=int, default=8, help="Number of points"
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

def amount(n):
    if n == 8:
        return 2*3315
    elif n == 9:
        return 2*158817
    else:
        raise ValueError("New n needs specification of amount! [line 74]")

## Run
DATA = {}
COMPLETED = {}

DATA[args.n] = []
COMPLETED[args.n] = 0
for i in range(amount(args.n)):
    constraints_filename = f"order_types/signs-{args.n}-{i}.txt"
    output_filename = f"order_types/signs-{args.n}-{i}.output"
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
    DATA[args.n].append(output)
    if output[2] / 1e9 < args.timeout:
        try:
            if not validator.validate(constraints_filename, output_filename):
                print(
                    f"Error in validation!! {colored('N', 'cyan')} = {args.n}, {colored('i', 'cyan')} = {i})"
                )
                sys.exit(1)
            COMPLETED[args.n] += 1
        except Exception as e: 
            print("EXCEPTION", e)
            print(output[1], output[0])
            sys.exit(1)

        print(
            f'{colored("N", "cyan")} = {args.n}, i = {i}, time = {output[2]/1e6:.2f} ms'
        )
    else:
        print(f'{colored("N", "cyan")} = {args.n}, i = {i}, {colored("timeout", "red")}')
print(f'{colored("--------------------", "green")}')

print(colored("\nSaving results to: ", "yellow"), args.output, "\n")
with open(args.output, "w", encoding="utf-8") as f:
    for N, data_N in DATA.items():
        avg = sum(t[2] for t in data_N) / len(data_N)
        print(
            f'{colored("N", "cyan")} = {N},' \
            f'{colored("avg time", "yellow")} = {avg/1e6:.2f}ms,' \
            f'{colored("completed", "green")} = {COMPLETED[N]}/{len(data_N)} '
        )
        f.write(f"{N} {avg/1e6:.2f} {COMPLETED[N]}/{len(data_N)}\n")
