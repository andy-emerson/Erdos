| Design | Operation | Path | x86 slots | Latency | Throughput bound |
| --- | --- | --- | --- | --- | --- |
| Control | int add | one | 4 | 6 | 0.67 |
| Control | int sub | one | 4 | 6 | 0.67 |
| Control | int neg | one | 3 | 6 | 0.5 |
| Control | int mul | one | 4 | 8 | 1 |
| Control | int div | one | 7 | 19 | 10 |
| Control | cx add | one | 8 | 6 | 1.33 |
| Control | cx sub | one | 8 | 6 | 1.33 |
| Control | cx neg | one | 6 | 6 | 1.0 |
| Control | cx mul | one | 14 | 9 | 4 |
| Control | cx div | real divisor | 16 | 19 | 20 |
| Control | cx div | imag divisor | 16 | 20 | 20 |
| A | int add (notes) | same | 11 | 6 | 1.83 |
| A | int add (notes) | opp, no borrow | 13 | 6 | 2.17 |
| A | int add (notes) | opp, borrow | 16 | 7 | 2.67 |
| A | int sub (notes) | same | 12 | 6 | 2.0 |
| A | int sub (notes) | opp, no borrow | 14 | 6 | 2.33 |
| A | int sub (notes) | opp, borrow | 17 | 8 | 2.83 |
| A | int add (perArm) | same | 11 | 6 | 1.83 |
| A | int add (perArm) | opp, no borrow | 13 | 6 | 2.17 |
| A | int add (perArm) | opp, borrow | 14 | 7 | 2.33 |
| A | int sub (perArm) | same | 12 | 6 | 2.0 |
| A | int sub (perArm) | opp, no borrow | 14 | 6 | 2.33 |
| A | int sub (perArm) | opp, borrow | 15 | 8 | 2.5 |
| A | cx add (notes) | same | 11 | 6 | 1.83 |
| A | cx add (notes) | opp, no borrow | 15 | 6 | 2.5 |
| A | cx add (notes) | opp, borrow | 18 | 7 | 3.0 |
| A | cx add (notes) | cross, x = 0 | 15 | 7 | 2.5 |
| A | cx add (notes) | cross, y = 0 | 15 | 6 | 2.5 |
| A | cx add (notes) | cross, expression | 12 | 0 | 2.0 |
| A | cx sub (notes) | same | 12 | 6 | 2.0 |
| A | cx sub (notes) | opp, no borrow | 16 | 6 | 2.67 |
| A | cx sub (notes) | opp, borrow | 19 | 8 | 3.17 |
| A | cx sub (notes) | cross, x = 0 | 16 | 8 | 2.67 |
| A | cx sub (notes) | cross, y = 0 | 16 | 6 | 2.67 |
| A | cx sub (notes) | cross, expression | 13 | 0 | 2.17 |
| A | cx add (perArm) | same | 11 | 6 | 1.83 |
| A | cx add (perArm) | opp, no borrow | 15 | 6 | 2.5 |
| A | cx add (perArm) | opp, borrow | 16 | 7 | 2.67 |
| A | cx add (perArm) | cross, x = 0 | 15 | 7 | 2.5 |
| A | cx add (perArm) | cross, y = 0 | 15 | 6 | 2.5 |
| A | cx add (perArm) | cross, expression | 12 | 0 | 2.0 |
| A | cx sub (perArm) | same | 12 | 6 | 2.0 |
| A | cx sub (perArm) | opp, no borrow | 16 | 6 | 2.67 |
| A | cx sub (perArm) | opp, borrow | 17 | 8 | 2.83 |
| A | cx sub (perArm) | cross, x = 0 | 16 | 8 | 2.67 |
| A | cx sub (perArm) | cross, y = 0 | 16 | 6 | 2.67 |
| A | cx sub (perArm) | cross, expression | 13 | 0 | 2.17 |
| A | neg | one | 5 | 6 | 1.0 |
| A | mul | one | 9 | 8 | 1.5 |
| A | div | one | 12 | 19 | 10 |
| A-bf | int add | one | 27 | 15 | 4.5 |
| A-bf | int sub | one | 28 | 16 | 4.67 |
| A-bf | cx add | one | 47 | 17 | 8.2 |
| A-bf | cx sub | one | 48 | 18 | 8.4 |
| B | int add | one | 8 | 6 | 1.33 |
| B | int sub | one | 8 | 6 | 1.33 |
| B | int neg | one | 4 | 5 | 1.0 |
| B | int mul | one | 14 | 9 | 4 |
| B | int collapse (notes) | borrow 0 | 7 | 6 | 1.17 |
| B | int collapse (notes) | borrow 1 | 9 | 7 | 1.5 |
| B | int collapse (perArm) | borrow 0 | 7 | 6 | 1.17 |
| B | int collapse (perArm) | borrow 1 | 6 | 6 | 1.0 |
| B | int div (notes) | borrows x 0, y 0 | 24 | 20 | 10 |
| B | int div (notes) | borrows x 0, y 1 | 25 | 21 | 10 |
| B | int div (notes) | borrows x 1, y 0 | 25 | 21 | 10 |
| B | int div (notes) | borrows x 1, y 1 | 26 | 21 | 10 |
| B | int div (perArm) | borrows x 0, y 0 | 26 | 20 | 10 |
| B | int div (perArm) | borrows x 0, y 1 | 25 | 20 | 10 |
| B | int div (perArm) | borrows x 1, y 0 | 25 | 20 | 10 |
| B | int div (perArm) | borrows x 1, y 1 | 24 | 20 | 10 |
| B | cx add | one | 16 | 6 | 2.67 |
| B | cx sub | one | 16 | 6 | 2.67 |
| B | cx neg | one | 8 | 5 | 2.0 |
| B | cx mul | one | 52 | 11 | 16 |
| B | cx collapse (notes) | borrows re 0, im 0 | 14 | 6 | 2.33 |
| B | cx collapse (notes) | borrows re 0, im 1 | 16 | 7 | 2.67 |
| B | cx collapse (notes) | borrows re 1, im 0 | 16 | 7 | 2.67 |
| B | cx collapse (notes) | borrows re 1, im 1 | 18 | 7 | 3.0 |
| B | cx collapse (perArm) | borrows re 0, im 0 | 14 | 6 | 2.33 |
| B | cx collapse (perArm) | borrows re 0, im 1 | 13 | 6 | 2.17 |
| B | cx collapse (perArm) | borrows re 1, im 0 | 13 | 6 | 2.17 |
| B | cx collapse (perArm) | borrows re 1, im 1 | 12 | 6 | 2.0 |
| B | cx div (notes) | borrows x 00, y 00 | 54 | 21 | 10 |
| B | cx div (notes) | borrows x 00, y 01 | 55 | 22 | 10 |
| B | cx div (notes) | borrows x 00, y 10 | 55 | 22 | 10 |
| B | cx div (notes) | borrows x 01, y 00 | 55 | 22 | 10 |
| B | cx div (notes) | borrows x 01, y 01 | 56 | 22 | 10 |
| B | cx div (notes) | borrows x 01, y 10 | 56 | 22 | 10 |
| B | cx div (notes) | borrows x 10, y 00 | 55 | 22 | 10 |
| B | cx div (notes) | borrows x 10, y 01 | 56 | 22 | 10 |
| B | cx div (notes) | borrows x 10, y 10 | 56 | 22 | 10 |
| B | cx div (perArm) | borrows x 00, y 00 | 58 | 21 | 10 |
| B | cx div (perArm) | borrows x 00, y 01 | 57 | 21 | 10 |
| B | cx div (perArm) | borrows x 00, y 10 | 57 | 21 | 10 |
| B | cx div (perArm) | borrows x 01, y 00 | 57 | 21 | 10 |
| B | cx div (perArm) | borrows x 01, y 01 | 56 | 21 | 10 |
| B | cx div (perArm) | borrows x 01, y 10 | 56 | 21 | 10 |
| B | cx div (perArm) | borrows x 10, y 00 | 57 | 21 | 10 |
| B | cx div (perArm) | borrows x 10, y 01 | 56 | 21 | 10 |
| B | cx div (perArm) | borrows x 10, y 10 | 56 | 21 | 10 |
| C | add | same axis | 9 | 7 | 1.5 |
| C | add | cross, y = 0 | 10 | 6 | 1.67 |
| C | add | cross, x = 0 | 13 | 6 | 2.17 |
| C | add | cross, expression | 13 | 0 | 2.17 |
| C | sub | same axis | 9 | 7 | 1.5 |
| C | sub | cross, y = 0 | 10 | 6 | 1.67 |
| C | sub | cross, x = 0 | 16 | 8 | 2.67 |
| C | sub | cross, expression | 16 | 0 | 2.67 |
| C | neg | one | 6 | 8 | 1.0 |
| C | mul | one | 17 | 12 | 2.83 |
| C | div | one | 18 | 24 | 10 |
