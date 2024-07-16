#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include "utils.c"

#define EPSILON 1e-6

// Function prototypes
void min_dist(Point* points, int n, double* min_distance, int* m1, int* m2);
void evaluate(const Point* points, int n, const Constraint* constraints, int constraint_count, const int** constraints_per_point, double MIN_DIST,
    int* total_violations, int* violations_per_point, int* point_with_max_violations, double* min_distance, int given_point, int constraint_count_given_point);

void min_dist(Point* points, int n, double* min_distance, int* m1, int* m2) {
    *min_distance = DBL_MAX;
    *m1 = -1;
    *m2 = -1;

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            double dist = fabs(points[i].x - points[j].x) + fabs(points[i].y - points[j].y);
            if (dist < *min_distance) {
                *min_distance = dist;
                *m1 = i;
                *m2 = j;
            }
        }
    }
}

void evaluate(const Point* points, int n, const Constraint* constraints, int constraint_count, const int** constraints_per_point,  double MIN_DIST,
    int* total_violations, int* violations_per_point, int* point_with_max_violations, double* min_distance, int given_point, int constraint_count_given_point) {
    *total_violations = 0;
    int max_violations = 0;
    *point_with_max_violations = -1;

    for (int i = 0; i < n; ++i) {
        violations_per_point[i] = 0;
    }

    int n_constraints = given_point == -1 ? constraint_count : constraint_count_given_point;
    for (int i = 0; i < n_constraints; i++) {
        Constraint constraint =  given_point == -1 ? constraints[i] : constraints[constraints_per_point[given_point][i]];

        int pi = constraint.i - 1;
        int pj = constraint.j - 1;
        int pk = constraint.k - 1;
        double determinant = det(points[pi], points[pj], points[pk]);

        if ((constraint.sign == 1 && determinant <= EPSILON) ||
            (constraint.sign == -1 && determinant >= -EPSILON)) {
            (*total_violations)++;
            int points_to_update[] = { pi, pj, pk };
            for (int j = 0; j < 3; j++) {
                int p = points_to_update[j];
                violations_per_point[p]++;
                if (violations_per_point[p] >= max_violations) {
                    max_violations = violations_per_point[p];
                    *point_with_max_violations = p;
                }
            }
        }
        
    }
    
    // int m1, m2;
    // min_dist(points, n, min_distance, &m1, &m2);

    // if (*min_distance < MIN_DIST) {
    //     (*total_violations)++;
    //     violations_per_point[m1]++;
    //     violations_per_point[m2]++;
    //     if (violations_per_point[m1] >= max_violations) {
    //         *point_with_max_violations = m1;
    //     }
    //     if (violations_per_point[m2] >= max_violations) {
    //         *point_with_max_violations = m2;
    //     }
    // }
}