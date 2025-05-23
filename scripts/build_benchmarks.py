import os
import random
import itertools

def generate_random_points(N):
    points = []
    for i in range(N):
        x = random.random()*50
        y = random.random()*50
        points.append((x, y))
    return points
    
def det(a, b, c):
    return (c[1]-a[1])*(b[0]-a[0]) - (b[1]-a[1])*(c[0]-a[0])
    
    
def generate_constraints(points):
    n = len(points)
    for triple in itertools.combinations(range(n), 3):
        a, b, c = triple
        pa, pb, pc = points[a], points[b], points[c]
        det_ = det(pa, pb, pc)
        if det_ == 0:
            raise ValueError('Collinear points')
        if det_ > 0:
            yield ('A', a, b, c)
        else:
            yield ('B', a, b, c)
        
        
def build_benchmarks(L, R, n, seed=42):
    random.seed(seed)

    for N in range(L, R+1):
        # create folder based on N
        # with name `benchmarks/N`
        os.makedirs(f'benchmarks/{N}', exist_ok=True)
        for i in range(n):
            # create file based on N and i
            # with name `benchmarks/N/i.txt
            with open(f'benchmarks/{N}/{i}.txt', 'w') as f:
                points = generate_random_points(N)
                constraints = list(generate_constraints(points))
                for constraint in constraints:
                    s, a, b, c = constraint
                    f.write(f'{s}_{a+1, b+1, c+1}\n')
                