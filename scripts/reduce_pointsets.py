import argparse


argparser = argparse.ArgumentParser()
argparser.add_argument("-f", "--file", type=str, required=True, help="Path to the input file")
argparser.add_argument("-ub", "--upperbound", type=int, required=True, help="consider points until this limit")
argparser.add_argument("-lb", "--lowerbound", type=int, required=True, help="consider points until this limit")
argparser.add_argument("-o", "--output", type=str, default="reduced.or", help="Path to the output file")

filename = argparser.parse_args().file
L = argparser.parse_args().lowerbound
U = argparser.parse_args().upperbound
output = argparser.parse_args().output

new_content = []
with open(filename, "r") as f:
    for line in f:
        tokens = line[:-1].split('_')
        ttokens = eval(tokens[1])
        a,b,c = ttokens
        if max(a,b,c) <= U and min(a,b,c) >= L:
            new_content.append(f"{tokens[0]}_{a-L+1, b-L+1,c-L+1}\n")
        

with open(output, "w") as f:
    f.write(''.join(new_content))