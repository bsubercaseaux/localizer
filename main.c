#include <stdio.h>
#include <float.h>
#include <stdbool.h> 
#include <unistd.h>
#include <string.h>
#include "utils.c"
#include "solver.c"


void print_usage() {
    color_printf(RED, "Usage: <orientation_file> [-i sub_iterations] [-d min_dist] [-o output_file] [-s random_seed] [-r reset_interval]\n");
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
    int random_seed = 42;
    double reset_interval = 2.0;

    char* output_file = malloc(256 * sizeof(char));
    if (output_file != NULL) {
        strcpy(output_file, "output.txt");
    }

    // Parse optional arguments
    int opt;

    while ((opt = getopt(argc - 1, argv + 1, "i:s:d:o:r:")) != -1) {
        switch (opt) {
            case 'i':
                sub_iterations = atoi(optarg);
                break;
            case 's':
                random_seed = atoi(optarg);
                break;
            case 'd':
                min_dist = atof(optarg);
                break;
            case 'o':
                strncpy(output_file, optarg, sizeof(output_file) - 1);
                break;
            case 'r':
                reset_interval = atof(optarg);
                break;
            default:
                print_usage();
                return 1;
        }
    }

    srand(random_seed);


    int N, constraint_count;
    Constraint constraints[MAX_CONSTRAINTS];
    int constraints_per_point[MAX_POINTS][MAX_CONSTRAINTS];
    int constraints_per_point_count[MAX_POINTS];

    parse_constraints(orientation_file, &N, constraints, &constraint_count, constraints_per_point, constraints_per_point_count);

    color_printf(YELLOW, "Parsed %d constraints over %d points\n\n", constraint_count, N);

    Point points[N];

    solve(N, constraints, constraint_count, constraints_per_point, constraints_per_point_count, sub_iterations, min_dist, points, output_file, reset_interval);

    return 0;
}
