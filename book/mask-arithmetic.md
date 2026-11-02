# Mask arithmetic

The examples use the 4-bit clock, where every number is a 4-bit word from 0000 to 1111 and arithmetic is mod 16.

A **flag** $k$ is 0 or 1. Its **mask** is

$$m = 0 - k = \begin{cases} 0000 & k = 0 \\ 1111 & k = 1 \end{cases}$$

since $0 - 1$ on the clock is 15, which is 1111. Every bit of the mask equals $k$, so the two always go together: the flag says yes or no, and the mask carries that answer to every bit of a word.

A mask can also come straight from the top bit of a word, with no flag. The arithmetic shift right, **sar**, moves every bit right and fills the empty places at the top with copies of the top bit. Shifting by 3 on 4 bits pushes out every bit except the top one and fills the word with it:

$$x \text{ sar } 3 = \begin{cases} 0000 & \text{top bit of } x = 0 \\ 1111 & \text{top bit of } x = 1 \end{cases}$$

For example, $1010 \text{ sar } 3 = 1111$ and $0110 \text{ sar } 3 = 0000$. The top bit can be any tag kept there, such as a sign.

## XOR

XOR compares two words bit by bit: each bit of the result is 1 where the words differ and 0 where they agree. So it is 0 exactly when they are equal:

$$a \text{ XOR } b = \begin{cases} 0000 & a = b \\ \text{not } 0000 & a \ne b \end{cases}$$

Against a mask, the comparison becomes a flip. Compared with 0000, a word differs exactly where it has a 1, so it comes back unchanged. Compared with 1111, it differs exactly where it has a 0, so every bit flips. So XOR with a mask leaves a word alone or flips every bit of it:

$$x \text{ XOR } m = \begin{cases} x & m = 0000 \\ 15 - x & m = 1111 \end{cases}$$

For example, with $x = 0110$ (6):

$$0110 \text{ XOR } m = \begin{cases} 0110 & m = 0000 \\ 1001 & m = 1111 \end{cases}$$

Flipping every bit of 6 gives 9, which is $15 - 6$.

### Conditional negation

A flip is one short of a negation: $-x$ on the clock is $16 - x$, not $15 - x$. Adding the flag supplies the missing 1:

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

### Absolute value

For a signed word $x$, the top bit is the sign, so sar makes the mask from it:

$$m = x \text{ sar } 3 = \begin{cases} 0000 & x \ge 0 \\ 1111 & x < 0 \end{cases}$$

Subtracting $m$ then does the work of adding the flag, since $-1111 = +1$ on the clock:

$$(x \text{ XOR } m) - m = \begin{cases} x & m = 0000 \\ -x & m = 1111 \end{cases}$$

### Flipping chosen bits

XOR with a constant $c$ flips the bits of $x$ where $c$ has a 1 and keeps the rest. Bit by bit:

$$\text{bit of } (x \text{ XOR } c) = \begin{cases} \text{bit of } x & \text{bit of } c = 0 \\ \text{bit of } x \text{ flipped} & \text{bit of } c = 1 \end{cases}$$

## AND

AND with a mask keeps a word or deletes it:

$$x \text{ AND } m = \begin{cases} 0 & m = 0000 \\ x & m = 1111 \end{cases}$$

For example, with $x = 0110$ (6):

$$0110 \text{ AND } m = \begin{cases} 0000 & m = 0000 \\ 0110 & m = 1111 \end{cases}$$

### Conditional deletion

AND with the mask is the whole formula, one instruction:

$$x \text{ AND } m = \begin{cases} 0 & m = 0000,\ k = 0 \\ x & m = 1111,\ k = 1 \end{cases}$$

For example, take $x = 0110$ (6). With $k = 0$, the mask is $m = 0000$, and the word is deleted:

$$\begin{aligned}
x \text{ AND } m &= 0110 \text{ AND } 0000 \\
&= 0000 \\
&= 0
\end{aligned}$$

With $k = 1$, the mask is $m = 1111$, and the word is kept:

$$\begin{aligned}
x \text{ AND } m &= 0110 \text{ AND } 1111 \\
&= 0110 \\
&= 6
\end{aligned}$$

### Conditional add

$$x + (y \text{ AND } m) = \begin{cases} x & m = 0000,\ k = 0 \\ x + y & m = 1111,\ k = 1 \end{cases}$$

### Keeping chosen bits

AND with a constant $c$ keeps the bits of $x$ where $c$ has a 1 and clears the rest. Bit by bit:

$$\text{bit of } (x \text{ AND } c) = \begin{cases} 0 & \text{bit of } c = 0 \\ \text{bit of } x & \text{bit of } c = 1 \end{cases}$$

### Remainder by a power of two

Keeping the low $j$ bits is the remainder by $2^j$:

$$x \text{ AND } (2^j - 1) = x \bmod 2^j$$

## XOR and AND

### Conditional choice

AND and XOR together choose between two words, $a$ and $c$:

$$a \text{ XOR } \big((a \text{ XOR } c) \text{ AND } m\big) = \begin{cases} a & m = 0000,\ k = 0 \\ c & m = 1111,\ k = 1 \end{cases}$$

$a \text{ XOR } c$ marks the bits where the two words differ. AND with the mask keeps those marks or deletes them, and XOR applies whatever is left to $a$: flipping exactly the bits where $a$ and $c$ differ turns $a$ into $c$, and flipping nothing leaves $a$.

For example, take $a = 0110$ (6) and $c = 0011$ (3). With $k = 1$, the mask is $m = 1111$, and the choice is $c$:

$$\begin{aligned}
a \text{ XOR } \big((a \text{ XOR } c) \text{ AND } m\big) &= 0110 \text{ XOR } \big((0110 \text{ XOR } 0011) \text{ AND } 1111\big) \\
&= 0110 \text{ XOR } (0101 \text{ AND } 1111) \\
&= 0110 \text{ XOR } 0101 \\
&= 0011 \\
&= 3
\end{aligned}$$

With $k = 0$, the mask is $m = 0000$, and the choice is $a$:

$$\begin{aligned}
a \text{ XOR } \big((a \text{ XOR } c) \text{ AND } m\big) &= 0110 \text{ XOR } \big((0110 \text{ XOR } 0011) \text{ AND } 0000\big) \\
&= 0110 \text{ XOR } (0101 \text{ AND } 0000) \\
&= 0110 \text{ XOR } 0000 \\
&= 0110 \\
&= 6
\end{aligned}$$

### Conditional swap

With $d = (a \text{ XOR } b) \text{ AND } m$, XOR-ing $d$ into both words trades them or leaves them:

$$(a \text{ XOR } d,\ b \text{ XOR } d) = \begin{cases} (a, b) & m = 0000,\ k = 0 \\ (b, a) & m = 1111,\ k = 1 \end{cases}$$

### Minimum and maximum

With the flag $k = (x < y)$ and its mask $m$, conditional choice picks the smaller or the larger word:

$$y \text{ XOR } \big((x \text{ XOR } y) \text{ AND } m\big) = \begin{cases} y & m = 0000,\ x \ge y \\ x & m = 1111,\ x < y \end{cases} \qquad \text{(the minimum)}$$

$$x \text{ XOR } \big((x \text{ XOR } y) \text{ AND } m\big) = \begin{cases} x & m = 0000,\ x \ge y \\ y & m = 1111,\ x < y \end{cases} \qquad \text{(the maximum)}$$

## Arithmetic operations

*(Placeholder.)*
