# Specs

The specifications being worked on now. A spec defines one or more models of exact arithmetic, and each round of tests measures them against a control. A spec folder moves to `../archive/` when its work is finished.

- [`11-matrix`](11-matrix/): exact rational matrices (Control, Packed, Packed-23, Packed-coprime). **Paused** while overflow is solved at the level of integers.
- [`12-integers`](12-integers/): growing unsigned integers, to replace 64-bit overflow as an error. **Current.**
