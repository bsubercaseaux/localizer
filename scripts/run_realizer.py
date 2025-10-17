import time
from subprocess import TimeoutExpired, check_output, CalledProcessError, STDOUT
from plotter import plot_solution
from termcolor import colored

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
    returns: output, return_code, elapsed_time in mseconds
    """
    start_time = time.perf_counter_ns()
    output, return_code = system_call(commands, timeout=timeout)
    elapsed_time = time.perf_counter_ns() - start_time
    return output, return_code, elapsed_time / 1e9


def test_realizability(input_file, timeout=None, realizer_path="realizer"):
    output, return_code, elapsed_time = timed_run_shell([realizer_path, input_file, "-i 10", "-r 30000"], timeout=timeout)
    if return_code == -1:
        return False
    else:
        return True
        
def test_and_output(input_file, timeout=None, points_output_file="out.txt", realizer_path="localizer", show_output=False):
    print("input_file = ", input_file, "points_output_file = ", points_output_file)
    output, return_code, elapsed_time = timed_run_shell([realizer_path, input_file, "-t", "4", "-i", "10", "-r", "30000", "-o", points_output_file], timeout=timeout)
    if return_code == -1:
        return False
    else:
        if show_output:
            print(f"output = {output}")
        plot_solution(points_output_file)
        return True

def run_on_folder(folder_path, timeout=None, realizer_path="realizer"):
    import os
    results = {}
    for filename in os.listdir(folder_path):
        if filename.endswith(".or"):
            full_path = os.path.join(folder_path, filename)
            output_file = filename + ".real"
            is_realizable = test_and_output(full_path, timeout=timeout, points_output_file=output_file, realizer_path=realizer_path)

            results[filename] = is_realizable
            realized_color = 'green' if is_realizable else 'red'
            print(f"File: {colored(filename, 'cyan')}, Realized?: {colored(is_realizable, realized_color)}{' (output saved to ' + output_file + ')' if is_realizable else ''}")
            print("---------------------------------------------------")
    return results

if __name__ == "__main__":
    import argparse
    argparser = argparse.ArgumentParser()
    argparser.add_argument("-f", "--folder", type=str, required=True, help="Path to the input orientation folder")
    argparser.add_argument("-t", "--timeout", type=int, default=5, help="Timeout in seconds")
    argparser.add_argument("-r", "--realizer_path", type=str, default="localizer", help="Path to the realizer executable")
    # argparser.add_argument("-o", "--output", type=str, default="out.txt", help="Output file for points")
    args = argparser.parse_args()
    run_on_folder(args.folder, timeout=args.timeout, realizer_path=args.realizer_path)
