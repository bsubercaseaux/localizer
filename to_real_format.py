import sys

def vmap(a, b, c):
    return a  + ((b-2)*(b-1))//2 + ((c-3)*(c-2)*(c-1))//6

input_filename = sys.argv[1]
M = {}
N = 50
for a in range(1, N+1):
    for b in range(a+1, N+1):
        for c in range(b+1, N+1):
            v = vmap(a, b, c) 
            M[v] = (a, b, c)

def to_var(i):
    ai = abs(i)
    triple = M[ai]
    if i < 0:
        return f"B_{triple}"
    else:
        return f"A_{triple}"

with open(input_filename, 'r') as F:
    mapped_vars = []
    for i, line in enumerate(F):
        tokens = line[:-1].split(' ')
        valid_starts = ['v -123456789']
        if tokens[0] not in valid_starts:
            print("Skipping line", i, "as it does not start with 'v -123456789'")
            continue
        tokens = tokens[1:-1]
        int_tokens = list(map(int, tokens))
        mapped_vars.extend(list(map(to_var, int_tokens)))
    print(mapped_vars)
    with open(input_filename + '.or', 'w') as W:
        for mv in mapped_vars:
            W.write(mv + '\n')
