import argparse
import matplotlib.pyplot as plt
from termcolor import colored



def parse_solution(solution_file):
    points = []
    with open(solution_file, "r") as S:
        for line in S:
            tokens = line[:-1].split(" ")
            i, x, y = int(tokens[0]), float(tokens[1]), float(tokens[2])
            points.append((x, y))
    return len(points), points


def plot_solution(solution_file):
    N, points = parse_solution(solution_file)
    
    x_coords, y_coords = zip(*points)
    fig, ax = plt.subplots()
    plt.gca().set_aspect('equal')
    ax.set_axisbelow(True)
    ax.grid(color="gray", linestyle="dashed")
    
    plt.scatter(x_coords, y_coords)
    
    plt.title(f"Realization for: {solution_file}")
    
    for i in range(N):
        ax.annotate(f"{i+1}", points[i], fontsize=6, color="black", bbox=dict(boxstyle='round,pad=0.1', fc='yellow', alpha=0.5))
    
    
    output_file = f"{solution_file}.png"
    plt.savefig(output_file, dpi=300)
    
    
    print(f"Saved plot to {colored(output_file, 'cyan')}")


if __name__ == "__main__":
    
    argparser = argparse.ArgumentParser()
    argparser.add_argument("--sol", type=str, required=True)
    
    args = argparser.parse_args()
    solution_file = args.sol
    plot_solution(solution_file)