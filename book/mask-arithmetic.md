# Mask arithmetic

The examples use the 4-bit clock, where every number is a 4-bit word from 0000 to 1111 and arithmetic is mod 16.

A **flag** $k$ is 0 or 1. Its **mask** is

$$m = 0 - k = \begin{cases} 0000 & k = 0 \\ 1111 & k = 1 \end{cases}$$

since $0 - 1$ on the clock is 15, which is 1111. Every bit of the mask equals $k$, so the two always go together: the flag says yes or no, and the mask carries that answer to every bit of a word.

## XOR

XOR with a mask leaves a word alone or flips every bit of it:

$$x \text{ XOR } m = \begin{cases} x & m = 0000 \\ 15 - x & m = 1111 \end{cases}$$

For example, with $x = 0110$ (6):

$$0110 \text{ XOR } m = \begin{cases} 0110 & m = 0000 \\ 1001 & m = 1111 \end{cases}$$

Flipping every bit of 6 gives 9, which is $15 - 6$.

**Conditional negation.** A flip is one short of a negation: $-x$ on the clock is $16 - x$, not $15 - x$. Adding the flag supplies the missing 1:

$$(x \text{ XOR } m) + k = \begin{cases} x & m = 0000,\ k = 0 \\ 16 - x & m = 1111,\ k = 1 \end{cases}$$

For example, take $x = 0110$ (6). With $k = 1$, the mask is $m = 1111$, and the word is negated:

$$\begin{aligned}
(x \text{ XOR } m) + k &= (0110 \text{ XOR } 1111) + 1 \\
&= 1001 + 1 \\
&= 1010 \\
&= 10 \\
&= 16 - 6 \\
&\equiv -6 \pmod{16}
\end{aligned}$$

With $k = 0$, the mask is $m = 0000$, and the word comes back unchanged:

$$\begin{aligned}
(x \text{ XOR } m) + k &= (0110 \text{ XOR } 0000) + 0 \\
&= 0110 + 0 \\
&= 0110 \\
&= 6
\end{aligned}$$

## AND

AND with a mask keeps a word or deletes it:

$$x \text{ AND } m = \begin{cases} 0 & m = 0000 \\ x & m = 1111 \end{cases}$$

For example, with $x = 0110$ (6):

$$0110 \text{ AND } m = \begin{cases} 0000 & m = 0000 \\ 0110 & m = 1111 \end{cases}$$

**Conditional deletion.** AND with the mask is the whole formula: one instruction keeps $x$ when $k = 1$ and turns it into 0 when $k = 0$.

## Arithmetic operations

*(Placeholder.)*
