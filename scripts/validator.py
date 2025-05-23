import math
from fractions import Fraction

import argparse


def parse_constraints(filename):
    constraints = []
    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            tk = line.split('_')
            sign = 1 if tk[0] == 'A' else -1
            vls = eval(tk[1])
            constraints.append((sign, vls))
    return constraints
        
def parse_points(filename):
    frac_points = []
    float_points = []
    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            tk = line.split(' ')
            frac_points.append((Fraction(float(tk[1])), Fraction(float(tk[2]))))
            float_points.append((float(tk[1]), float(tk[2])))
    return frac_points, float_points
    
def det(a, b, c):
    # return (pc.y - pa.y) * (pb.x - pa.x) - (pc.x - pa.x) * (pb.y - pa.y);
    return (c[1] - a[1]) * (b[0] - a[0]) - (c[0] - a[0]) * (b[1] - a[1])

def validate_constraint(sign, a, b, c):
    if sign == 1:
        return det(a, b, c) > 0
    else:
        return det(a, b, c) < 0
        
def validate(constraint_filename, point_filename):
    constraints = parse_constraints(constraint_filename)
    frac_points, float_points = parse_points(point_filename)
    for sign, vls in constraints:
        a, b, c = frac_points[vls[0]-1], frac_points[vls[1]-1], frac_points[vls[2]-1]
        float_a, float_b, float_c = float_points[vls[0]-1], float_points[vls[1]-1], float_points[vls[2]-1]
        if not validate_constraint(sign, a, b, c):
            print(f"Error in constraint: {'A' if sign == 1 else 'B'}_{vls}")
            print(f"Points: {a}, {b}, {c}")
            print(f"frac det: {det(a, b, c)} ~ {float(det(a, b, c))}")
            print(f"Float Points: {float_a}, {float_b}, {float_c}")
            print(f"float det: {det(float_a, float_b, float_c)}")
            return False
    return True
    
if  __name__ == "__main__":
    argparser = argparse.ArgumentParser()
    argparser.add_argument("-c", "--constraint", type=str, help="Constraint file")
    argparser.add_argument("-p", "--point", type=str, help="Point file")
    
    args = argparser.parse_args()
    print(validate(args.constraint, args.point))