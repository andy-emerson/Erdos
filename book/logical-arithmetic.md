# Logical arithmetic

The four logical operators, AND, OR, XOR and NOT, work on words bit by bit. Each section below starts with what its operator does, then its uses without a mask, then its uses with one.

The examples use the 4-bit clock, where every number is a 4-bit word from 0000 to 1111 and arithmetic is mod 16.

Shifts move bits. **shl** moves them left by $j$ places and fills with zeros, **shr** moves them right and fills with zeros, and **sar** moves them right and fills with copies of the top bit:

$$x \text{ shl } j = 2^j x \bmod 16 \qquad x \text{ shr } j = \lfloor x / 2^j \rfloor \qquad x \text{ sar } j = \lfloor x / 2^j \rfloor \text{ for } x \text{ read as signed}$$

A **flag** $k$ is 0 or 1. Its **mask** is

$$m = 0 - k = \begin{cases} 0000 & k = 0 \\ 1111 & k = 1 \end{cases}$$

since $0 - 1$ on the clock is 15, which is 1111. Every bit of the mask equals $k$, so the two always go together: the flag says yes or no, and the mask carries that answer to every bit of a word.

Instructions: 1

A mask can also come straight from the top bit of a word, with no flag. Shifting by 3 with sar pushes out every bit except the top one and fills the word with it:

$$x \text{ sar } 3 = \begin{cases} 0000 & \text{top bit of } x = 0 \\ 1111 & \text{top bit of } x = 1 \end{cases}$$

Instructions: 1

For example, $1010 \text{ sar } 3 = 1111$ and $0110 \text{ sar } 3 = 0000$. The top bit can be any tag kept there, such as a sign.

## XOR

XOR compares two words bit by bit: each bit of the result is 1 where the words differ and 0 where they agree. So it is 0 exactly when they are equal:

$$a \text{ XOR } b = \begin{cases} 0000 & a = b \\ \text{not } 0000 & a \ne b \end{cases}$$

Instructions: 1

### Toggling chosen bits

XOR with a constant $c$ toggles the bits of $x$ where $c$ has a 1 and keeps the rest. Bit by bit:

$$\text{bit of } (x \text{ XOR } c) = \begin{cases} \text{bit of } x & \text{bit of } c = 0 \\ \text{bit of } x \text{ toggled} & \text{bit of } c = 1 \end{cases}$$

Instructions: 1

Toggling the same bits twice restores them: $(x \text{ XOR } c) \text{ XOR } c = x$.

### With a mask

Against a mask, the comparison becomes a flip. Compared with 0000, a word differs exactly where it has a 1, so it comes back unchanged. Compared with 1111, it differs exactly where it has a 0, so every bit flips. So XOR with a mask leaves a word alone or flips every bit of it:

$$x \text{ XOR } m = \begin{cases} x & m = 0000 \\ 15 - x & m = 1111 \end{cases}$$

Instructions: 1

For example, with $x = 0110$ (6):

$$0110 \text{ XOR } m = \begin{cases} 0110 & m = 0000 \\ 1001 & m = 1111 \end{cases}$$

Flipping every bit of 6 gives 9, which is $15 - 6$.

### Conditional negation

A flip is one short of a negation: $-x$ on the clock is $16 - x$, not $15 - x$. Adding the flag supplies the missing 1:

$$(x \text{ XOR } m) + k = \begin{cases} x & m = 0000,\ k = 0 \\ 16 - x & m = 1111,\ k = 1 \end{cases}$$

Instructions: 2

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

Instructions: 2

## AND

AND keeps bits: each bit of the result is 1 only where both words have a 1. So AND with a word $c$ keeps the bits of $x$ where $c$ has a 1 and clears the rest. Bit by bit:

$$\text{bit of } (x \text{ AND } c) = \begin{cases} 0 & \text{bit of } c = 0 \\ \text{bit of } x & \text{bit of } c = 1 \end{cases}$$

Instructions: 1

### Congruence

The low $j$ bits of $x$ are its remainder mod $2^j$:

$$x \text{ AND } (2^j - 1) \equiv x \pmod{2^j}$$

Instructions: 1, or 3 when $j$ varies

The bits above them are multiples of $2^j$, and the AND clears them.

For a constant modulus, the mask is a constant in the program (0011 for mod 4). Otherwise it is $(1 \text{ shl } j) - 1$.

For example, $13 \bmod 4$:

$$\begin{aligned}
1101 \text{ AND } (2^2 - 1) &= 1101 \text{ AND } 0011 \\
&= 0001 \\
&= 1
\end{aligned}$$

### With a mask

Against a mask, keeping some bits becomes keeping all or none. 1111 has a 1 in every place, so it keeps every bit of the word, and 0000 has none, so it clears them all. So AND with a mask keeps a word or deletes it:

$$x \text{ AND } m = \begin{cases} 0 & m = 0000 \\ x & m = 1111 \end{cases}$$

Instructions: 1

For example, with $x = 0110$ (6):

$$0110 \text{ AND } m = \begin{cases} 0000 & m = 0000 \\ 0110 & m = 1111 \end{cases}$$

### Conditional deletion

AND with the mask is the whole formula, one instruction:

$$x \text{ AND } m = \begin{cases} 0 & m = 0000,\ k = 0 \\ x & m = 1111,\ k = 1 \end{cases}$$

Instructions: 1

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

Instructions: 2

For example, take $x = 0110$ (6) and $y = 0011$ (3). With $k = 1$, the mask is $m = 1111$, and $y$ is added:

$$\begin{aligned}
x + (y \text{ AND } m) &= 0110 + (0011 \text{ AND } 1111) \\
&= 0110 + 0011 \\
&= 1001 \\
&= 9
\end{aligned}$$

With $k = 0$, the mask is $m = 0000$, and nothing is added:

$$\begin{aligned}
x + (y \text{ AND } m) &= 0110 + (0011 \text{ AND } 0000) \\
&= 0110 + 0000 \\
&= 0110 \\
&= 6
\end{aligned}$$

## OR

OR sets bits: each bit of the result is 1 where either word has a 1. So OR with a word $c$ sets the bits of $x$ where $c$ has a 1 and keeps the rest. Bit by bit:

$$\text{bit of } (x \text{ OR } c) = \begin{cases} \text{bit of } x & \text{bit of } c = 0 \\ 1 & \text{bit of } c = 1 \end{cases}$$

Instructions: 1

### Setting chosen bits

OR with a constant sets those bits, as above. When the bits are known to be 0 beforehand, it is also an add: $x \text{ OR } c = x + c$.

Instructions: 1

### Combining flags

On flags, OR means "either":

$$k_1 \text{ OR } k_2 = \begin{cases} 0 & k_1 = 0,\ k_2 = 0 \\ 1 & \text{otherwise} \end{cases}$$

Instructions: 1

A flag OR-ed with each new flag stays 1 once any of them is 1.

### With a mask

Against a mask, setting some bits becomes setting all or none. So OR with a mask keeps a word or fills it:

$$x \text{ OR } m = \begin{cases} x & m = 0000,\ k = 0 \\ 1111 & m = 1111,\ k = 1 \end{cases}$$

Instructions: 1

## NOT

NOT toggles every bit of a word, the same as XOR with 1111:

$$\text{NOT } x = 15 - x$$

Instructions: 1

### Negation

Toggling every bit is one short of negation, so adding 1 completes it:

$$-x = (\text{NOT } x) + 1$$

Instructions: 2

### The opposite mask

NOT turns a mask into the mask of the opposite flag:

$$\text{NOT } m = \begin{cases} 1111 & m = 0000,\ k = 0 \\ 0000 & m = 1111,\ k = 1 \end{cases}$$

Instructions: 1

### Conditional keeping

AND with the opposite mask keeps $x$ when $k = 0$ and deletes it when $k = 1$:

$$x \text{ AND NOT } m = \begin{cases} x & m = 0000,\ k = 0 \\ 0 & m = 1111,\ k = 1 \end{cases}$$

Instructions: 2, or 1 with x86-64's `andn` or ARM64's `bic`

## Combinations

### Conditional choice

AND and XOR together choose between two words, $a$ and $c$:

$$a \text{ XOR } \big((a \text{ XOR } c) \text{ AND } m\big) = \begin{cases} a & m = 0000,\ k = 0 \\ c & m = 1111,\ k = 1 \end{cases}$$

Instructions: 3

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

Instructions: 4

### Minimum and maximum

With the flag $k = (x < y)$ and its mask $m$, conditional choice picks the smaller or the larger word:

$$y \text{ XOR } \big((x \text{ XOR } y) \text{ AND } m\big) = \begin{cases} y & m = 0000,\ x \ge y \\ x & m = 1111,\ x < y \end{cases} \qquad \text{(the minimum)}$$

$$x \text{ XOR } \big((x \text{ XOR } y) \text{ AND } m\big) = \begin{cases} x & m = 0000,\ x \ge y \\ y & m = 1111,\ x < y \end{cases} \qquad \text{(the maximum)}$$

Instructions: 3 each, 4 for both; the compare that gives $k$ is one more

## Arithmetic operations

### Addition from XOR and AND

$$a + b = (a \text{ XOR } b) + 2\,(a \text{ AND } b)$$

Instructions: 4

XOR adds each bit without carrying. AND marks the bits that carry, and each carry moves one place left, which doubles it.

### Addition from AND and OR

$$a + b = (a \text{ AND } b) + (a \text{ OR } b)$$

Instructions: 3

### Subtraction from logic

Subtracting is adding the negation:

$$a - b = a + (\text{NOT } b) + 1$$

Instructions: 3, or 1 with the machine's subtraction, which works this way

### Conditional subtraction

With a mask in place of NOT, one line adds or subtracts:

$$a + (b \text{ XOR } m) + k = \begin{cases} a + b & m = 0000,\ k = 0 \\ a - b & m = 1111,\ k = 1 \end{cases}$$

Instructions: 3

OR counts each bit set in either word once, and AND counts again the bits set in both.

## Glossary

- **Absolute value** (XOR): $x$ or $-x$, by the sign's mask. 2 instructions.
- **Addition from AND and OR** (arithmetic): $a + b$ as AND plus OR. 3.
- **Addition from XOR and AND** (arithmetic): $a + b$ as the sum without carries plus the carries. 4.
- **Combining flags** (OR): 1 if either flag is 1. 1.
- **Comparing** (XOR): 0 exactly when two words are equal. 1.
- **Conditional add** (AND): adds $y$ or nothing. 2.
- **Conditional choice** (combinations): $a$ or $c$. 3.
- **Conditional deletion** (AND): $x$ or 0. 1.
- **Conditional keeping** (NOT): $x$ or 0, the opposite way. 2, or 1 with `andn` or `bic`.
- **Conditional negation** (XOR): $x$ or $-x$. 2.
- **Conditional subtraction** (arithmetic): $a + b$ or $a - b$. 3.
- **Conditional swap** (combinations): $(a, b)$ or $(b, a)$. 4.
- **Congruence** (AND): $x \bmod 2^j$. 1, or 3 when $j$ varies.
- **Minimum and maximum** (combinations): the smaller or larger word. 3 each, 4 for both.
- **Negation** (NOT): $-x$. 2.
- **Opposite mask** (NOT): the mask of the other flag. 1.
- **Setting chosen bits** (OR): sets the bits of a constant. 1.
- **Subtraction from logic** (arithmetic): $a - b$ as adding the negation. 3.
- **Toggling chosen bits** (XOR): toggles the bits of a constant. 1.
