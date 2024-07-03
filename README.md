# Point realizer
Code for realizability of pseudo-points (through CC-systems / signotopes / triple orientations).

## Compilation

```
make
```

## Usage

```
./realizer <orientation file> [sub iterations] [minimum distance] [output filename]
````
(Note that arguments are positional; I should have "keys" soon, to do for instance `-o output filename`).

The orientation file must be in the format described by the following rules:
1. Each line contains an "orientation", which is of the form `<O>(<a>, <b>, <c>)`.
2.  `<O>` must be either "A" (above), "B" (below) or "C" (collinear).
3.  The parameters`<a>`, `<b>`, `<c>` are positive integers that hold `a < b < c`.

For example, a valid orientation is `A(2, 4, 7)` and represents that point `2` is above the directed line `4 -> 7`.

To facilitate things, this repo includes a file that translates SAT assignments into this format, as long as the encoding respects that
the index of the variabel representing whether point `a` is above `b->c` (or equivalently, that `a->b->c` is a counterclockwise turn) is exactly 
```
a  + ((b-2)*(b-1))//2 + ((c-3)*(c-2)*(c-1))//6
```
To use this translator, simply run
```
python3 to_real_format.py <solution_file>
```
This will generate `<solution_file>.or` with the desired format.
