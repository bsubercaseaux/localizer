#include <pthread.h>
#include <stdbool.h>
#include <assert.h>
#include "utils.c"


#ifndef THREADING_H
#define THREADING_H


#define K_TOP 5

// Mutex and condition variable to signal the first thread to finish
typedef struct {
    Solution top_k_solutions[K_TOP];
    bool stop_flag; 
    pthread_mutex_t stop_mutex;
    
    pthread_mutex_t top_k_mutex;
    pthread_mutex_t print_mutex;
} synchronization_t;

void sync_init(synchronization_t* sync) {
    sync->stop_flag = false;
    
    for(int i = 0; i < K_TOP; ++i) {
        solution_init(&sync->top_k_solutions[i]);
    }

    int rc1 = pthread_mutex_init(&sync->stop_mutex, NULL);
    assert(rc1 == 0);
    int rc2 = pthread_mutex_init(&sync->top_k_mutex, NULL);
    assert(rc2 == 0);
    int rc3 = pthread_mutex_init(&sync->print_mutex, NULL);
    assert(rc3 == 0);
}

void sync_destroy(synchronization_t* sync) {
    pthread_mutex_destroy(&sync->stop_mutex);
    pthread_mutex_destroy(&sync->top_k_mutex);
    pthread_mutex_destroy(&sync->print_mutex);
}

bool sync_should_stop(synchronization_t* sync) {
    pthread_mutex_lock(&sync->stop_mutex);
    bool should_stop = sync->stop_flag;
    pthread_mutex_unlock(&sync->stop_mutex);
    return should_stop;
}

bool sync_set_stop(synchronization_t* sync) {
    // returns whether it was the first thread to set the stop flag (i.e., it was false before)
    pthread_mutex_lock(&sync->stop_mutex);
    bool ans = !sync->stop_flag;
    sync->stop_flag = true;
    pthread_mutex_unlock(&sync->stop_mutex);
    return ans;
}

void sync_broadcast_new_solution(synchronization_t* sync, Point* points, int violations) {
    pthread_mutex_lock(&sync->top_k_mutex);
    for(int i = 0; i < K_TOP; ++i) {
        if(violations < sync->top_k_solutions[i].violations) {
            for(int j = i+1; j < K_TOP; ++j) {
                sync->top_k_solutions[j].violations = sync->top_k_solutions[j-1].violations;
                for(int p = 0; p < MAX_POINTS; ++p) {
                    sync->top_k_solutions[j].points[p].x =  sync->top_k_solutions[j-1].points[p].x;
                    sync->top_k_solutions[j].points[p].y =  sync->top_k_solutions[j-1].points[p].y;
                }
            }
            sync->top_k_solutions[i].violations = violations;
            for(int p = 0; p < MAX_POINTS; ++p) {
                sync->top_k_solutions[i].points[p].x = points[p].x;
                sync->top_k_solutions[i].points[p].y = points[p].y;
            }
            break;
        }
    }
    pthread_mutex_unlock(&sync->top_k_mutex);
}

void sync_get_best_solution(synchronization_t* sync, Point* points, int* violations, rng_t* rng) {
    pthread_mutex_lock(&sync->top_k_mutex);
    
    int scores[K_TOP];
    for(int i = 0; i < K_TOP; ++i) {
        scores[i] = MAX_CONSTRAINTS - sync->top_k_solutions[i].violations;
    }
    int idx = sample_proportional(scores, K_TOP, rng);
    
    *violations = sync->top_k_solutions[idx].violations;
    
    for(int i = 0; i < MAX_POINTS; ++i) {
        points[i] = random_point_in_ball(sync->top_k_solutions[idx].points[i], 1.0, rng);
    }
    
    pthread_mutex_unlock(&sync->top_k_mutex);
}

void sync_color_printf(synchronization_t* sync, Color color, const char* format, ...) {
    pthread_mutex_lock(&sync->print_mutex);
    va_list args;

    // Print the color code
    printf("%s", get_color_code(color));

    // Initialize variadic arguments
    va_start(args, format);

    // Print the formatted string
    vprintf(format, args);

    // Reset to default color
    printf("%s", get_color_code(RESET));
    pthread_mutex_unlock(&sync->print_mutex);

    // Clean up variadic arguments
    va_end(args);
}

void sync_printf(synchronization_t* sync, const char* format, ...) {
    va_list args;
    
    va_start(args, format);
    
    pthread_mutex_lock(&sync->print_mutex);
    vprintf(format, args);
    pthread_mutex_unlock(&sync->print_mutex);

    va_end(args);
}



#endif // THREADING_H