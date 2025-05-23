import argparse


argparser = argparse.ArgumentParser()
argparser.add_argument("-f", "--file", type=str, required=True, help="Path to the input file")

filename = argparser.parse_args().file

new_content = []
with open(filename, "r") as f:
    for line in f:
        if line[-1] != ')':
            line = line[:-1]
        tokens = line.split('_')
        print(tokens)
        ttokens = eval(tokens[1])
        a,b,c = ttokens
        if max(a,b,c) > 16:
            continue
        new_content.append(line)

with open("reduced.or", "w") as f:
    f.write(''.join(new_content))