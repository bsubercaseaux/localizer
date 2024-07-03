#include <stdio.h>
#include <float.h>
#include <stdbool.h> 
#include "utils.c"
#include "evaluation.c"

void solve(int N,
    Constraint* constraints,
    int constraint_count,
    int sub_iterations,
    double MIN_DIST,
    Point* points,
    char* output_file)
{
    generate_random_assignment(N, points);
    clock_t start_time = clock();
    long long int it = 0;

    int total_violations = INT32_MAX; // initialize to "infinity"
    int violations_per_point[N], point_with_max_violations;
    double min_distance;

    while (total_violations > 0) {

        evaluate(points, N, constraints, constraint_count, MIN_DIST,
            &total_violations, violations_per_point, &point_with_max_violations, &min_distance);

        if (it % 5000 == 0) {
            double time_elapsed = (double)(clock() - start_time) / CLOCKS_PER_SEC;
            printf("[t "); color_printf(CYAN, "%.2f", time_elapsed); 
            printf(" s]\t[kilo itr %lld]\t", it / 1000);
            printf("[unsat"); color_printf(RED, " %d", total_violations); 
            printf("]\t[min dist %.3f]\t[max unsat point ", min_distance);
            color_printf(YELLOW, "%d", point_with_max_violations + 1);
            printf(" ; %d]\n", violations_per_point[point_with_max_violations]);
        }

        if (total_violations == 0) {
            break;
        }

        double final_radius = 15.0;
        double starting_radius = final_radius / pow(1.5, sub_iterations);

        for (int sub_it = 0; sub_it < sub_iterations; sub_it++) {
            int chosen_for_replacement = sample_proportional(violations_per_point, N);

            Point test_pt = random_point_in_ball(points[chosen_for_replacement],
                fmax(0.5, starting_radius * pow(1.5, sub_it)));
            Point test_pts[N];
            memcpy(test_pts, points, N * sizeof(Point));
            test_pts[chosen_for_replacement] = test_pt;

            int total_violations_with_test, temp_violations_per_point[N], temp_max_violation_point;
            double temp_min_dist;

            evaluate(test_pts, N, constraints, constraint_count, MIN_DIST,
                &total_violations_with_test, temp_violations_per_point,
                &temp_max_violation_point, &temp_min_dist);

            // if (total_violations == 1 && min_distance < MIN_DIST) {
            //     if (total_violations_with_test < total_violations ||
            //         (total_violations_with_test == total_violations && temp_min_dist > min_distance)) {
            //         memcpy(points, test_pts, N * sizeof(Point));
            //         break;
            //     }
            // } else {
            
            if (total_violations_with_test <= total_violations) {
                memcpy(points, test_pts, N * sizeof(Point));
                break;
            }
        }

        it++;
    }

    clock_t end_time = clock();
    double time_elapsed = (double)(end_time - start_time) / CLOCKS_PER_SEC;

    color_printf(GREEN, "\n====================  SOLVED  ====================\n\n");
    color_printf(YELLOW, "Time");  printf(": %.3f seconds\n", time_elapsed);
    color_printf(YELLOW, "Total iterations"); printf(": %lld\n", it);
    color_printf(YELLOW, "Minimum distance"); printf(": %.3f\n", min_distance);
    color_printf(GREEN, "\nSolution:\n");
    for (int i = 0; i < N; i++) {
        printf("\t\t Point %d: (%.6f, %.6f)\n", i + 1, points[i].x, points[i].y);
    }
    printf("\n");
    
    serialize_solution(N, points, output_file);
    color_printf(YELLOW, "Solution saved to %s\n", output_file);
}