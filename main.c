#include <stdio.h>
#include <float.h>
#include <stdbool.h> 
#include "utils.c"
#include "solver.c"

#define MAX_CONSTRAINTS 1000

int main(int argc, char* argv[])
{
    if (argc < 2) {
        color_printf(YELLOW, "Usage: %s <orientation_file> [sub_iterations] [min_dist] [output_file]\n", argv[0]);
        return 1;
    }

    char* orientation_file = argv[1];
    int sub_iterations = 10;
    double min_dist = 0.25;
    char* output_file = "output.txt";

    if (argc > 2) {
        sub_iterations = atoi(argv[2]);
    }
    if (argc > 3) {
        min_dist = atof(argv[3]);
    }
    if (argc > 4) {
        output_file = argv[4];
    }

    int N, constraint_count;
    Constraint constraints[MAX_CONSTRAINTS];
    parse_constraints(orientation_file, &N, constraints, &constraint_count);

    color_printf(YELLOW, "Parsed %d constraints over %d points\n\n", constraint_count, N);
   
    
    Point points[N];
    solve(N, constraints, constraint_count, sub_iterations, min_dist, points, output_file);

    return 0;
}