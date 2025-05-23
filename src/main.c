#include <stdio.h>
#include <float.h>
#include <stdbool.h> 
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#include "utils.c"
#include "solver.c"
#include "threading.c"
#include "rng.c"

int GLOBAL_SEED = 42;

synchronization_t _sync; // Global synchronization variable, keeping the best solutions.
// it's global to allow for signal handling
int _N = 0; // number of points, global for signal handling

char* output_file = NULL;

// Struct to hold thread parameters
typedef struct {
    int thread_id;
    int N; 
    Constraint *constraints;
    int constraint_count;
    int** constraints_per_point;
    int* constraints_per_point_count;
    
    bool* is_point_fixed;
    Point* fixed_points;
    Symmetry* symmetry;
    
    int sub_iterations;
    double MIN_DIST; 
    Point *points;
    char *output_file;
    long long int reset_its;
    rng_t *rng;
    synchronization_t *sync;
} thread_params_t;


void print_usage() {
    color_printf(RED, "Usage: <orientation_file> [-i sub_iterations] [-d min_dist] [-o output_file] [-s random_seed]\n [-r reset_interval] [-t threads] [-f fixed points file] [-c symmetry file]\n");
}

void sigint_handler(int sig_num)
{
    printf("\nInterrupt signal (%d) received.\n", sig_num);
    pthread_mutex_lock(&_sync.top_k_mutex);
    // Print information about the object if it exists
  
    color_printf(GREEN, "Best solution is:\n");
    Point* points = _sync.top_k_solutions[0].points;
    for (int i = 0; i < _N; i++) {
        printf("\t\t Point %d: (%.6f, %.6f)\n", i + 1, points[i].x, points[i].y);
    }
        
    printf("Violations: %d\n", _sync.top_k_solutions[0].violations);
    printf("\n");

    serialize_solution(_N, points, output_file);
    color_printf(YELLOW, "Solution saved to %s\n", output_file);
 
    pthread_mutex_unlock(&_sync.top_k_mutex);
    sync_destroy(&_sync);
    exit(0);
}


void* thread_solve(void* arg) {
    
    thread_params_t* params = (thread_params_t*)arg;
    
    solve(params->N, 
        params->constraints, 
        params->constraint_count, 
        (const int**) params->constraints_per_point, 
        params->constraints_per_point_count, 
        params->sub_iterations, 
        params->MIN_DIST, 
        params->points, 
        params->output_file, 
        params->reset_its,
        params->thread_id,
        params->sync,
        params->rng,
        params->is_point_fixed,
        params->fixed_points,
        params->symmetry);
    
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

        
    signal(SIGINT, sigint_handler);
    
    char* orientation_file = argv[1];

    // default values
    int sub_iterations = 10;
    int NUM_THREADS = 1;
    double min_dist = -1.0; // negative -> turned off
    long long int reset_its = 100000;
    
        output_file = malloc(256 * sizeof(char));
    if (output_file != NULL) {
        strcpy(output_file, "output.txt");
    }
    
    char* fixed_points_file = malloc(256 * sizeof(char));
    char* symmetry_file = malloc(256 * sizeof(char));

    // Parse optional arguments
    int opt;

    while ((opt = getopt(argc - 1, argv + 1, "i:s:d:o:r:t:f:c:")) != -1) {
        switch (opt) {
            case 'i':
                sub_iterations = atoi(optarg);
                break;
            case 's':
                GLOBAL_SEED = atoi(optarg);
                break;
            case 'd':
                min_dist = atof(optarg);
                break;
            case 'o':
                strcpy(output_file, optarg); //, sizeof(output_file) - 1);
                break;
            case 'r':
                reset_its = atoi(optarg);
                break;
            case 't':
                NUM_THREADS = atoi(optarg);
                break;
            case 'f':
                strcpy(fixed_points_file, optarg);
                // some_fixed_points = true;
                break;
            case 'c':
                strcpy(symmetry_file, optarg);
                break;
            default:
                print_usage();
                return 1;
        }
    }
    
    pthread_t threads[NUM_THREADS];
        
    int N, constraint_count;
    Constraint constraints[MAX_CONSTRAINTS];
    int** constraints_per_point = calloc(MAX_POINTS, sizeof(int*));
    for(int i = 0; i < MAX_POINTS; ++i) {
        constraints_per_point[i] = calloc(MAX_CONSTRAINTS, sizeof(int));
    }
    int* constraints_per_point_count = calloc(MAX_POINTS, sizeof(int));
    
    parse_constraints(orientation_file, &N, constraints, &constraint_count, constraints_per_point, constraints_per_point_count);

    color_printf(YELLOW, "Parsed %d constraints over %d points\n\n", constraint_count, N);
    
    _N = N;
    
    bool* is_point_fixed = calloc(MAX_POINTS, sizeof(bool));
    Point* fixed_points = calloc(MAX_POINTS, sizeof(Point));
    
    parse_fixed_points(fixed_points_file, N, fixed_points, is_point_fixed);
    
    Symmetry symmetry;
    
    parse_symmetry(symmetry_file, &symmetry);        
   
    // Synchronization mutexes.
    sync_init(&_sync);
    
    
    
    thread_params_t* params = calloc(NUM_THREADS, sizeof(thread_params_t));
    
    // Create threads
    for (int i = 0; i < NUM_THREADS; i++) {
   
        params[i].thread_id = i + 1;
        params[i].N = N;
        params[i].constraints = constraints;
        params[i].constraint_count = constraint_count;
        params[i].constraints_per_point =  constraints_per_point;
        params[i].constraints_per_point_count = constraints_per_point_count;
        params[i].is_point_fixed = is_point_fixed;
        params[i].fixed_points = fixed_points;
        params[i].symmetry = &symmetry;
        params[i].sub_iterations = sub_iterations;
        params[i].MIN_DIST = min_dist;
        params[i].points = calloc(MAX_POINTS, sizeof(Point));
        params[i].output_file = output_file;
        params[i].reset_its = reset_its;
        params[i].sync = &_sync;
        params[i].rng = malloc(sizeof(rng_t));
        rng_init(params[i].rng, GLOBAL_SEED + i);

        if (pthread_create(&threads[i], NULL, thread_solve, &params[i]) != 0) {
            perror("Failed to create thread");
            return 1;
        }
    }
    
    color_printf(CYAN, "=========================================================\n");
    color_printf(CYAN, "========================  STARTING  =====================\n");
    color_printf(CYAN, "=========================================================\n\n");
    
       // Join all threads
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Failed to join thread");
            return 1;
        }

        free(params[i].points);
        free(params[i].rng);
        
    }
    
    free(constraints_per_point_count);
    for(int i = 0; i < MAX_POINTS; ++i) {
        free(constraints_per_point[i]);
    }
    
    sync_destroy(&_sync);
    
    free(params);
    free(output_file);

    return 0;
}
