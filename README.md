# Erdos

Making exact rational arithmetic fast. Arithmetic here is always exact; the work is about doing it with less time and memory.

The repository holds two things being worked on, and the rules they follow:

- [`spec`](spec/): the current specifications and their tests. A specification defines one or more models of exact arithmetic (how numbers are stored and how operations work), and each round of tests measures the models against a control: time and memory, on fixed workloads. Each spec has its own folder.
- [`paper`](paper/): a book-length paper that explains the ideas from the ground up (integers, rationals, decimals, matrices) and builds a library from them in pseudocode. The scripts in `paper/verify/` check that pseudocode.
- [`RULES.md`](RULES.md): the rules for algorithm design, optimization, reviews and prose.

[`tools`](tools/) holds small helper scripts, and [`archive`](archive/) holds earlier specifications and earlier versions of the paper.
