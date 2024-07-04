#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdarg.h>

#define MAX_LINE_LENGTH 256


#define MAX_POINTS 40
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


// Function prototypes
void generate_random_assignment(int N, Point* points);
int sample_proportional(int* weights, int count);
Point random_point_in_ball(Point p, double r);
double det(Point pa, Point pb, Point pc);

// Parse constraints from file
void parse_constraints(const char* orientation_file, 
                        int* N, 
                        Constraint* constraints, 
                        int* constraint_count,
                        int constraints_per_point[MAX_POINTS][MAX_CONSTRAINTS],
                        int* constraints_per_point_count) {
                        
    FILE* file = fopen(orientation_file, "r");
    if (file == NULL) {
        printf("Error opening file\n");
        exit(1);
    }

    char line[MAX_LINE_LENGTH];
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
void generate_random_assignment(int N, Point* points) {
    for (int i = 0; i < N; i++) {
        points[i].x = (double)rand() / RAND_MAX * 10;
        points[i].y = (double)rand() / RAND_MAX * 10;
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



// Sample an index proportionally to its weight
int sample_proportional(int* weights, int count) {

    int adjusted_weights[count];
    
    int total_violations = 0;
    for (int i = 0; i < count; i++) {
        // weight adjustment makes sure all weights are > 0, and hence every element has a chance to be selected
        adjusted_weights[i] = WEIGHT_ADJUSTMENT * weights[i] + 1;
        total_violations += adjusted_weights[i];
    }

    double r = (double)rand() / RAND_MAX * total_violations;
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
Point random_point_in_ball(Point p, double r) {
    double theta = (double)rand() / RAND_MAX * 2 * M_PI;
    double random_r = r * sqrt((double)rand() / RAND_MAX);

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
        fprintf(file, "%d %.6f %.6f\n", i + 1, points[i].x, points[i].y);
    }

    fclose(file);
}


#endif // UTILS_H