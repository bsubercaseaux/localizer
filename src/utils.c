#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdarg.h>
#include <stdint.h>
#include "rng.c"

#define __STDC_LIMIT_MACROS
#define MAX_LINE_LENGTH 256


#define MAX_POINTS 81
#define MAX_CONSTRAINTS (MAX_POINTS*(MAX_POINTS-1)*(MAX_POINTS-2))/6

#ifndef UTILS_H
#define UTILS_H


#define WEIGHT_ADJUSTMENT 5


// Structure to represent a constraint
typedef struct {
    int i, j, k;
    int sign;
} Constraint;

// Structure to represent a point
typedef struct {
    double x, y;
} Point;

typedef struct {
    Point points[MAX_POINTS];
    int violations;
} Solution;

// Structure to represent a symmetry
typedef struct {
    int cycles[MAX_POINTS][MAX_POINTS];
    int cycle_lengths[MAX_POINTS];
    // int leads[MAX_POINTS];
    int num_cycles;
} Symmetry;

void solution_init(Solution* sol) {
    for(int i = 0; i < MAX_POINTS; ++i) {
        sol->points[i].x = 0.0;
        sol->points[i].y = 0.0;
        sol->violations = INT32_MAX;
    }
}

// Function prototypes
void generate_random_assignment(int N, Point* points, rng_t* rng);
int sample_proportional(int* weights, int count, rng_t* rng);
Point random_point_in_ball(Point p, double r, rng_t* rng);
double det(Point pa, Point pb, Point pc);
void parse_fixed_points(const char* fixed_points_file, int N, Point* fixed_points, bool* is_point_fixed);

// Parse constraints from file
void parse_constraints(const char* orientation_file, 
                        int* N, 
                        Constraint* constraints, 
                        int* constraint_count,
                        int** constraints_per_point,
                        int* constraints_per_point_count) {
                        
    FILE* file = fopen(orientation_file, "r");
    if (file == NULL) {
        printf("Error opening file\n");
        exit(1);
    }

    char line[MAX_LINE_LENGTH*sizeof(char)];
    *N = 0;
    *constraint_count = 0;
    for(int i = 0; i < MAX_POINTS; i++) {
        constraints_per_point_count[i] = 0;
    }

    while (fgets(line, sizeof(line), file)) {
        char orientation;
        int i, j, k;
        sscanf(line, "%c_(%d, %d, %d)", &orientation, &i, &j, &k);

        if (i > *N) *N = i;
        if (j > *N) *N = j;
        if (k > *N) *N = k;

        constraints[*constraint_count].i = i;
        constraints[*constraint_count].j = j;
        constraints[*constraint_count].k = k;
        
        constraints_per_point[i-1][constraints_per_point_count[i-1]++] = *constraint_count;
        constraints_per_point[j-1][constraints_per_point_count[j-1]++] = *constraint_count;
        constraints_per_point[k-1][constraints_per_point_count[k-1]++] = *constraint_count;

        switch (orientation) {
        case 'A': constraints[*constraint_count].sign = 1; break;
        case 'B': constraints[*constraint_count].sign = -1; break;
        case 'C': constraints[*constraint_count].sign = 0; break;
        }
        
        (*constraint_count)++;
        if (*constraint_count >= MAX_CONSTRAINTS) {
            printf("ERROR: Too many constraints\n");
            exit(1);
        }
    }

    fclose(file);
}

// Generate random assignment of coordinates
void generate_random_assignment(int N, Point* points, rng_t* rng) {
    for (int i = 0; i < N; i++) {
        points[i].x = rng_float(rng) * 10;
        points[i].y = rng_float(rng) * 10;
    }
}

int update_max_violations(int* violations_per_point, int N, int updated_point, int updated_violations) {
    int mx = -1;
    int imx = 0;
    for(int i = 0; i < N; i++) {
        int v = (i == updated_point) ? updated_violations : violations_per_point[i];
        if(v > mx) {
            mx = v;
            imx = i;
        }
    }
    return imx;
}


Point rotate(Point p, double angle) {
    Point rotated;
    rotated.x = p.x * cos(angle) - p.y * sin(angle);
    rotated.y = p.x * sin(angle) + p.y * cos(angle);
    return rotated;
}

// rotate r times in a k-fold symmetry
// so by 2*pi*r/k degrees
Point rotate_r_k(Point p, int r, int k) {
    double angle = 2 * M_PI * r / k;
    return rotate(p, angle);
}

void enforce_symmetry(const Symmetry* symmetry, Point* points) {
    for(int i = 0; i < symmetry->num_cycles; ++i) {
        Point lead = points[symmetry->cycles[i][0]];
        for(int j = 0; j < symmetry->cycle_lengths[i]; ++j) {
            points[symmetry->cycles[i][j]] = rotate_r_k(lead, j, symmetry->cycle_lengths[i]);
        }
    }
}



// Sample an index proportionally to its weight
int sample_proportional(int* weights, int count, rng_t* rng) {

    int adjusted_weights[count];
    
    int total_violations = 0;
    for (int i = 0; i < count; i++) {
        // weight adjustment makes sure all weights are > 0, and hence every element has a chance to be selected
        // adjusted_weights[i] = WEIGHT_ADJUSTMENT * pow(weights[i], 1.5) + 1;
        adjusted_weights[i] = WEIGHT_ADJUSTMENT * weights[i] + 1;
        total_violations += adjusted_weights[i];
    }

    double r = rng_float(rng) * total_violations;
    int cumulative = 0;
    for (int i = 0; i < count; i++) {
        // printf("i: %d, weight: %d\n", i, weights[i]);
        cumulative += adjusted_weights[i];
        if (r <= cumulative) {
            return i;
        }
    }

    return count - 1;  // Fallback
}

// Generate a random point in a ball around a given point
Point random_point_in_ball(Point p, double r, rng_t* rng) {
    double theta = rng_float(rng) * 2 * M_PI;
    double random_r = r * sqrt(rng_float(rng));

    Point new_point;
    new_point.x = p.x + random_r * cos(theta);
    new_point.y = p.y + random_r * sin(theta);

    return new_point;
}

// Calculate determinant of a 2D triangle
double det(Point pa, Point pb, Point pc) {
    return (pc.y - pa.y) * (pb.x - pa.x) - (pc.x - pa.x) * (pb.y - pa.y);
}

// for pretty printing
typedef enum {
    BLACK,
    RED,
    GREEN,
    YELLOW,
    BLUE,
    MAGENTA,
    CYAN,
    WHITE,
    RESET
} Color;

const char* get_color_code(Color color) {
    switch (color) {
    case BLACK:   return "\033[0;30m";
    case RED:     return "\033[0;31m";
    case GREEN:   return "\033[0;32m";
    case YELLOW:  return "\033[0;33m";
    case BLUE:    return "\033[0;34m";
    case MAGENTA: return "\033[0;35m";
    case CYAN:    return "\033[0;36m";
    case WHITE:   return "\033[0;37m";
    case RESET:   return "\033[0m";
    default:      return "\033[0m";
    }
}

void color_printf(Color color, const char* format, ...) {
    va_list args;

    // Print the color code
    printf("%s", get_color_code(color));

    // Initialize variadic arguments
    va_start(args, format);

    // Print the formatted string
    vprintf(format, args);

    // Reset to default color
    printf("%s", get_color_code(RESET));

    // Clean up variadic arguments
    va_end(args);
}

void serialize_solution(int N, Point* points, const char* output_file) {
    FILE* file = fopen(output_file, "w");
    if (file == NULL) {
        printf("Error opening file\n");
        exit(1);
    }

    for (int i = 0; i < N; i++) {
        fprintf(file, "%d %.8f %.8f\n", i + 1, points[i].x, points[i].y);
    }

    fclose(file);
}

struct timespec get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts;
}

double elapsed_time_sec(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

// Parse fixed points from file
void parse_fixed_points(const char* fixed_points_file, int N, Point* fixed_points, bool* is_point_fixed) {
    // If no file is provided, return without fixing any points
    if (fixed_points_file == NULL || strlen(fixed_points_file) == 0) {
        return;
    }

    FILE* file = fopen(fixed_points_file, "r");
    if (file == NULL) {
        printf("Error opening fixed points file\n");
        return;
    }

    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        int point_idx;
        double x, y;
        
        // Parse line in format "idx:x,y"
        if (sscanf(line, "%d:%lf,%lf", &point_idx, &x, &y) == 3) {
            if (point_idx > 0 && point_idx <= N) {
                // Adjust to 0-based indexing
                int idx = point_idx - 1;
                fixed_points[idx].x = x;
                fixed_points[idx].y = y;
                is_point_fixed[idx] = true;
                
                color_printf(GREEN, "Fixed point %d at (%.2f, %.2f)\n", point_idx, x, y);
            } else {
                color_printf(RED, "Invalid point index %d in fixed points file\n", point_idx);
            }
        } else {
            color_printf(RED, "Invalid format in fixed points file: %s\n", line);
        }
    }

    fclose(file);
}

void parse_symmetry(const char* symmetry_file, Symmetry* symmetry) {
    // If no file is provided, return without fixing any points
    if (symmetry_file == NULL || strlen(symmetry_file) == 0) {
        symmetry->num_cycles = 0;
        return;
    }

    FILE* file = fopen(symmetry_file, "r");
    if (file == NULL) {
        printf("Error opening symmetry file\n");
        exit(1);
    }

    color_printf(GREEN, "Parsing symmetry file: %s\n", symmetry_file);
    color_printf(GREEN, "--------------------------------\n");
    char line[MAX_LINE_LENGTH];
    int cycle_count = 0;
    while (fgets(line, sizeof(line), file)) {
        char* ptr = line;
        int num;
        int cnt = 0;
        int chr_cnt = 0;
        while (ptr && *ptr && sscanf(ptr, "%d%n", &num, &chr_cnt) == 1) {
            symmetry->cycles[cycle_count][cnt++] = num-1;
            ptr += chr_cnt;
            // Skip any whitespace
            while (*ptr && (*ptr == ' ' || *ptr == '\t')) {
                ptr++;
            }
        }
        

        symmetry->cycle_lengths[cycle_count] = cnt;

        cycle_count++;
        
    }
    symmetry->num_cycles = cycle_count;
    for(int i = 0; i < symmetry->num_cycles; ++i) {
        color_printf(YELLOW, "Cycle %d, ", i);
 
        
        for(int j = 0; j < symmetry->cycle_lengths[i]; ++j) {
            int point = symmetry->cycles[i][j];
            printf("%d->", point + 1);   
        }
        printf("%d", symmetry->cycles[i][0] + 1);
        printf("\n");
    }
    fclose(file);
    color_printf(GREEN, "--------------------------------\n");
    color_printf(GREEN, "Parsed %d cycles\n\n", symmetry->num_cycles);
}

#endif // UTILS_H