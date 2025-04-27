#include <stdio.h>
#include <float.h>
#include <stdbool.h> 
#include <pthread.h>

#include "utils.c"

#include "evaluation.c"
#include "threading.c"

#define RESET_MULTIPLIER 1.25
#define MIN_RADIUS 0.5

// Right now this is a full reset, but it should be something smarter soon.
void reset(Point* points, int N, synchronization_t* sync, rng_t* rng, const bool* is_point_fixed, const Point* fixed_points) {
    // color_printf(RED, "\n================================  RESET ================================\n\n");
    int new_violations = 0;
    sync_get_best_solution(sync, points, &new_violations, rng);
    
    // Re-apply fixed points after reset
    for (int i = 0; i < N; i++) {
        if (is_point_fixed[i]) {
            points[i] = fixed_points[i];
        }
    }
}

void print_stats(int thread_id, double time_elapsed, long long int it, int total_violations, double min_distance, int point_with_max_violations, int* violations_per_point, synchronization_t* sync) {
    pthread_mutex_lock(&sync->print_mutex);
    color_printf(YELLOW, "[Thread %d] ", thread_id);
    printf("[t ");
    color_printf(CYAN, "%.2f", time_elapsed);
    printf(" s]\t[kilo itr %lld]\t", it / 1000);
    printf("[unsat"); color_printf(RED, " %d", total_violations);
    printf("]\t[min dist %.3f]\t[max unsat point ", min_distance);
    color_printf(YELLOW, "%d", point_with_max_violations + 1);
    printf(" ; %d]\n", violations_per_point[point_with_max_violations]);
    pthread_mutex_unlock(&sync->print_mutex);
}

void solve(int N,
    const Constraint* constraints,
    int constraint_count,
    const int** constraints_per_point,
    const int* constraints_per_point_count,
    int sub_iterations,
    double MIN_DIST,
    Point* points,
    const char* output_file,
    long long int reset_its,
    int thread_id,
    synchronization_t* sync,
    rng_t* rng,
    const bool* is_point_fixed,
    const Point* fixed_points)
{
    long long int original_reset_its = reset_its;
    
    // each thread starts from a random assignment
    generate_random_assignment(N, points, rng);
    
    // Apply fixed points if any
    for (int i = 0; i < N; i++) {
        if (is_point_fixed[i]) {
            points[i] = fixed_points[i];
        }
    }
    struct timespec start_time = get_time();
    long long int it = 0;

    int total_violations = INT32_MAX; // initialize to "infinity"
    int violations_per_point[N], point_with_max_violations;
    double min_distance = 1.0;

    // evaluate the random assignment over all the constraints
    evaluate(points, N, constraints, constraint_count, constraints_per_point, MIN_DIST,
        &total_violations, violations_per_point, &point_with_max_violations, &min_distance, -1, -1);

    long long its_since_checkpoint = 0;
    
    // points will be sampled from a ball with exponentially increasing radius
    // ending at 10.0
    double final_radius = 15.0;
    double starting_radius = final_radius / pow(1.25, sub_iterations);
    
    Point test_pts[N];
    int violations_per_point_relative[N];
    int violations_chosen = 0;
    int max_violation_relative = 0;
    double min_dist_relative;

    while (total_violations > 0) {
       
        if (its_since_checkpoint > reset_its) {
            reset(points, N, sync, rng, is_point_fixed, fixed_points);

            its_since_checkpoint = 0;
            evaluate(points, N, constraints, constraint_count, constraints_per_point, MIN_DIST,
                &total_violations, violations_per_point, &point_with_max_violations, &min_distance, -1, -1);
        }


        if (total_violations == 0) {
            break;
        }

        // every X iterations we check if a different thread has finished, in which case this call terminates
        if(it % 1000 == 0) {
            if(sync_should_stop(sync)) {
                return;
            }
        }

        // twice per reset we print states
        if (it % (reset_its) == 0) {
            double time_elapsed = elapsed_time_sec(start_time, get_time());
            print_stats(thread_id, time_elapsed, it, total_violations, min_distance, point_with_max_violations, violations_per_point, sync);
        }

      
        for (int sub_it = 0; sub_it < sub_iterations; sub_it++) {
            // choose a point to move proportionally to constraint violations
            int chosen_for_replacement = sample_proportional(violations_per_point, N, rng);
            
            // Skip if this is a fixed point
            if (is_point_fixed[chosen_for_replacement]) {
                // Try again with another point if this one is fixed
                continue;
            }
            
            // first evaluation regarding the chosen point
            // local evaluation only looks at constaints involving the chosen point.
            evaluate(points, N, constraints, constraint_count,
                constraints_per_point, MIN_DIST,
                &violations_chosen, violations_per_point_relative, &max_violation_relative, 
                &min_dist_relative, chosen_for_replacement, constraints_per_point_count[chosen_for_replacement]);
    
            // create copy 
            memcpy(test_pts, points, N * sizeof(Point));
            
            // update the chosen point in the copy
            test_pts[chosen_for_replacement] = random_point_in_ball(points[chosen_for_replacement],
                fmax(MIN_RADIUS, final_radius / pow(2, sub_it)), rng);
            

            // evaluate the updated points
            int total_violations_with_test, temp_violations_per_point[N], temp_max_violation_point;
            double temp_min_dist;
            

            evaluate(test_pts, N, constraints, constraint_count,
                constraints_per_point, MIN_DIST,
                &total_violations_with_test, temp_violations_per_point, &temp_max_violation_point, &temp_min_dist, chosen_for_replacement, constraints_per_point_count[chosen_for_replacement]);
                
          
            int local_violations = violations_per_point_relative[chosen_for_replacement];
            int local_violations_with_test = temp_violations_per_point[chosen_for_replacement];
  
            if (local_violations_with_test <= local_violations) {
                // update points
                points[chosen_for_replacement] = test_pts[chosen_for_replacement];
      
                // update violations
                for (int i = 0; i < N; i++) {
                    violations_per_point[i] += temp_violations_per_point[i] - violations_per_point_relative[i];
                }
                 
                // update check point if there is a strict improvement
                if (local_violations_with_test < local_violations) {
                    total_violations += local_violations_with_test - local_violations;                 
                    sync_broadcast_new_solution(sync, points, total_violations);
                    its_since_checkpoint = 0; 
                    break;
                }

            }
            
        }
        
        it++;
        its_since_checkpoint++;
    }
    
    if(sync_set_stop(sync)) { // only print and save if no other thread has done so first.
    
        // final solution check.
        evaluate(points, N, constraints, constraint_count, constraints_per_point, MIN_DIST,
        &total_violations, violations_per_point, &point_with_max_violations, &min_distance, -1, -1);

        assert(total_violations == 0);

        double time_elapsed = elapsed_time_sec(start_time, get_time());

        color_printf(GREEN, "\n====================  SOLVED  ====================\n\n");
        color_printf(YELLOW, "Time");  printf(": %.3f seconds\n", time_elapsed);
        color_printf(YELLOW, "Total iterations"); printf(": %lld\n", it);
        color_printf(YELLOW, "Minimum distance"); printf(": %.3f\n", min_distance);
        color_printf(YELLOW, "Thread number"); printf(": %d\n", thread_id);
        color_printf(GREEN, "\nSolution:\n");

        for (int i = 0; i < N; i++) {
            printf("\t\t Point %d: (%.6f, %.6f)\n", i + 1, points[i].x, points[i].y);
        }

        // for (int i = 0; i < constraint_count; i++) {
        //     Constraint constraint = constraints[i];
        //     int pi = constraint.i - 1;
        //     int pj = constraint.j - 1;
        //     int pk = constraint.k - 1;
        //     double determinant = det(points[pi], points[pj], points[pk]);
        //     if ((constraint.sign == 1 && determinant <= EPSILON) ||
        //         (constraint.sign == -1 && determinant >= -EPSILON)) {
        //         printf("Constraint %d violated\n", i + 1, " determinant: %.6f\n", determinant);
        //     }
        // }
        printf("\n");

        serialize_solution(N, points, output_file);
        color_printf(YELLOW, "Solution saved to %s\n", output_file);
    }
}
