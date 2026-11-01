# Erdos

Making exact rational arithmetic fast. Arithmetic here is always exact; the work is about doing it with less time and memory.

The repository holds two things being worked on, and the rules they follow:

- [`spec`](spec/): the current specifications and their tests. A specification defines one or more models of exact arithmetic (how numbers are stored and how operations work), and each round of tests measures the models against a control: time and memory, on fixed workloads. Each spec has its own folder.
- [`book`](book/): the book, in two parts. Part 1, the machine: the mathematics of words and the routines a machine runs, costed in cycles. Part 2, the library: integers, rationals, decimals and matrices built from the ground up in pseudocode. The scripts in `book/verify-part1/` and `book/verify-part2/` check each part.
- [`RULES.md`](RULES.md): the rules for algorithm design, optimization, reviews and prose.

[`tools`](tools/) holds small helper scripts, and [`archive`](archive/) holds earlier specifications and the paper, which became the book.
