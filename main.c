#include <stdio.h>
#include <float.h>
#include <stdbool.h> 
#include <unistd.h>
#include <string.h>
#include "utils.c"
#include "solver.c"

#define MAX_CONSTRAINTS 1000

void print_usage() {
    color_printf(RED, "Usage: <orientation_file> [-s sub_iterations] [-d min_dist] [-o output_file]\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    char* orientation_file = argv[1];
    
    // default values
    int sub_iterations = 10;
    double min_dist = 0.25;
    char output_file[256] = "output.txt"; 
    
    // Parse optional arguments
    int opt;
    while ((opt = getopt(argc - 1, argv + 1, "s:d:o:")) != -1) {
        switch (opt) {
            case 's':
                sub_iterations = atoi(optarg);
                break;
            case 'd':
                min_dist = atof(optarg);
                break;
            case 'o':
                strncpy(output_file, optarg, sizeof(output_file) - 1);
                break;
            default:
                print_usage();
                return 1;
        }
    }

    int N, constraint_count;
    Constraint constraints[MAX_CONSTRAINTS];
    parse_constraints(orientation_file, &N, constraints, &constraint_count);

    color_printf(YELLOW, "Parsed %d constraints over %d points\n\n", constraint_count, N);
   
    Point points[N];
    solve(N, constraints, constraint_count, sub_iterations, min_dist, points, output_file);

    return 0;
}