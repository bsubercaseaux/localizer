#include <stdio.h>
#include <float.h>
#include <stdbool.h>
#include "utils.c"
#include "evaluation.c"

#define RESET_MULTIPLIER 1.25
#define MIN_RADIUS 0.5

// Right now this is a full reset, but it should be something smarter soon.
void reset(Point* points, int N) {
    color_printf(RED, "\n================================  RESET ================================\n\n");
    generate_random_assignment(N, points);
}

void print_stats(double time_elapsed, long long int it, int total_violations, double min_distance, int point_with_max_violations, int* violations_per_point) {
    printf("[t "); color_printf(CYAN, "%.2f", time_elapsed);
    printf(" s]\t[kilo itr %lld]\t", it / 1000);
    printf("[unsat"); color_printf(RED, " %d", total_violations);
    printf("]\t[min dist %.3f]\t[max unsat point ", min_distance);
    color_printf(YELLOW, "%d", point_with_max_violations + 1);
    printf(" ; %d]\n", violations_per_point[point_with_max_violations]);
}

void solve(int N,
    Constraint* constraints,
    int constraint_count,
    int constraints_per_point[MAX_POINTS][MAX_CONSTRAINTS],
    int* constraints_per_point_count,
    int sub_iterations,
    double MIN_DIST,
    Point* points,
    char* output_file,
    double reset_interval)
{

    generate_random_assignment(N, points);
    clock_t start_time = clock();
    long long int it = 0;

    int total_violations = INT32_MAX; // initialize to "infinity"
    int violations_per_point[N], point_with_max_violations;
    double min_distance;

    clock_t check_point = start_time;
    
    evaluate(points, N, constraints, constraint_count, constraints_per_point, MIN_DIST,
            &total_violations, violations_per_point, &point_with_max_violations, &min_distance, -1, -1);

    while (total_violations > 0) {

        double time_since_checkpoint = (double)(clock() - check_point) / CLOCKS_PER_SEC;

        if (time_since_checkpoint > reset_interval) {
            reset(points, N);
            reset_interval *= RESET_MULTIPLIER;
            check_point = clock();
             evaluate(points, N, constraints, constraint_count, constraints_per_point, MIN_DIST,
                &total_violations, violations_per_point, &point_with_max_violations, &min_distance, -1, -1);
        }
        
       
        
        if (total_violations == 0) {
            break;
        }
     
        if (it % 50000 == 0) {
            double time_elapsed = (double)(clock() - start_time) / CLOCKS_PER_SEC;
            print_stats(time_elapsed, it, total_violations, min_distance, point_with_max_violations, violations_per_point);
        }

        // points are sampled from a ball with exponentially increasing radius
        // ending at 15.0
        double final_radius = 15.0;
        double starting_radius = final_radius / pow(1.5, sub_iterations);

        for (int sub_it = 0; sub_it < sub_iterations; sub_it++) {
            int chosen_for_replacement = sample_proportional(violations_per_point, N);
            
            // first evaluatoin regarding the chosen point
            int violations_chosen = 0;
            int violations_per_point_relative[N];
            int max_violation_relative = 0;
            double min_dist_relative;
            
            evaluate(points, N, constraints, constraint_count,
                constraints_per_point, MIN_DIST,
                &violations_chosen, violations_per_point_relative, &max_violation_relative, &min_dist_relative, chosen_for_replacement, constraints_per_point_count[chosen_for_replacement]);

            // create copy with updated chosen point
            Point test_pts[N];
            memcpy(test_pts, points, N * sizeof(Point));
            
            Point test_pt = random_point_in_ball(points[chosen_for_replacement],
                                                fmax(MIN_RADIUS, starting_radius * pow(1.5, sub_it)));
            test_pts[chosen_for_replacement] = test_pt;

            // evaluate the updated points
            int total_violations_with_test, temp_violations_per_point[N], temp_max_violation_point;
            double temp_min_dist;
            
            evaluate(test_pts, N, constraints, constraint_count,
                constraints_per_point, MIN_DIST,
                &total_violations_with_test, temp_violations_per_point, &temp_max_violation_point, &temp_min_dist, chosen_for_replacement, constraints_per_point_count[chosen_for_replacement]);
            
            int local_violations = violations_per_point[chosen_for_replacement];
            int local_violations_with_test = temp_violations_per_point[chosen_for_replacement];
            
            // update points if violations didn't decrease
            //  but only break if there is a strict improvement
            if (local_violations_with_test <= local_violations) {
                // update points
                memcpy(points, test_pts, N * sizeof(Point));
                
                // update violations
                for (int i = 0; i < N; i++) {
                    int diff = temp_violations_per_point[i] - violations_per_point_relative[i];
                    violations_per_point[i] += diff;
                }

                // recompute max violation point
                point_with_max_violations = update_max_violations(violations_per_point, N, chosen_for_replacement, local_violations_with_test);
                
                // update check point if there is a strict improvement
                if (local_violations_with_test < local_violations) {
                    total_violations += local_violations_with_test - local_violations;
                    check_point = clock();
                    break;
                }
                

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
