#ifndef RNG_H
#define RNG_H

typedef struct {
    unsigned long long int state;
} rng_t;

float rng_float(rng_t* rng) {
    rng->state = (rng->state * 1103515245 + 12345) & 0x7fffffff;
    return (float)rng->state / 0x7fffffff;
}

void rng_init(rng_t* rng, unsigned long long int seed) {
    rng->state = seed;
}

#endif