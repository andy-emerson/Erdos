# Notes: signed and complex numbers in a machine word

*Third edition, first draft (Nov 2, 2026): the second edition ([`notes-second-edition.md`](notes-second-edition.md)) with four designs: the signed integer (the machine's), the complex integer (formerly Design C), the header design (formerly Design A) and the complex integer (new). Placeholders mark what is still to be written.*

Machine arithmetic is modular arithmetic. A word of $n$ bits is like an odometer with $n$ binary digits. If you have a 6-digit odometer set to measure kilometers, each kilometer traveled turns the rightmost digit by one, rolling over the next digit every ten kilometers, and so on. The first kilometer traveled takes it from 000000 to 000001. Once you have traveled 999,999 kilometers, there are no more digits to roll over, so one more kilometer turns it back to 000000. Digital clocks work the same way, with the minutes rolling over from 59 to 00. (Hours on a digital clock work this way too, but convention has them run from 1 to 12 instead of 0 to 11.)

The number of positions a counter passes through before it returns to 0 is its **modulus**, $N$. Minutes have modulus 60, and hours modulus 12. The odometer counts in base 10, ten positions per digit, but as a whole it rolls over at $N = 10^6 = 1{,}000{,}000$: the modulus is the base raised to the number of digits. Modular arithmetic works the same for every modulus, which is why an analog clock can draw it as a literal circle, cut into $N$ segments.

The machine counts in base 2, binary, and each binary digit is a bit, so a counter of $n$ bits has modulus $N = 2^n$. A byte is 8 bits, with modulus $2^8 = 256$: it holds every whole number from 0 to 255. On a 64-bit machine a word is 8 bytes, 64 bits, with modulus

$$2^{64} = 18446744073709551616,$$

so a word counts from 0 to 18,446,744,073,709,551,615. For demonstration, 4 bits are enough: the base-2 odometer counts from 0000 to 1111, and the modulus $2^4=16$ clock counts from 0 to 15.

On the number line, 15 hours plus 3 hours is 18 hours. However, on a 16-hour clock 3 hours past 15 displays 2, and so we say that 18 is *congruent* to 2 mod 16.

$$15 + 3 = 18 \equiv 2 \pmod{16}$$

Similarly, if the 16-hour clock reads 4 and we ask what time it was 6 hours ago, it rolls back the other way.

$$4 - 6 = -2 \equiv 14 \pmod{16}$$

So machine arithmetic is modular arithmetic, and modular arithmetic is circular, like the face of an analog clock. The arithmetic we usually want is linear: the number line runs forever in both directions. Translating the line onto the clock is straightforward, as above. Translating the clock back to the line meets two problems, one at each end of the machine's range.

- **Below zero.** The number line has negative numbers, and the clock carries no sign: $3 - 5$ lands on 14, and nothing on the clock says whether 14 means 14 or $-2$.
- **Above $2^n - 1$.** The number line has no largest number, and the clock does: $15 + 1$ lands on 0, not on 16. This is called **overflow**.

The machine has an answer to both problems. We will start with the first.

## The machine's answer: signed integers

Rather than counting up from 0, as we do when we measure a magnitude such as distance on an odometer, the machine can put zero in the middle of its range. A signed word of $n$ bits counts from $-2^{n-1}$ to $2^{n-1} - 1$: with 4 bits, from $-8$ to $7$; with 8 bits, from $-128$ to $127$. There are still $2^n$ values, shifted down by half.

How it does this comes from running the odometer backward. One kilometer back from 000000 shows 999999, and two back shows 999998. The 4-bit odometer does the same: one below 0000 is 1111, and two below is 1110. The machine takes that reading at its word: 1111 is $-1$, 1110 is $-2$, and so on down to 1000, which is $-8$.

| Word | 0000 | 0001 | 0010 | 0011 | 0100 | 0101 | 0110 | 0111 | 1000 | 1001 | 1010 | 1011 | 1100 | 1101 | 1110 | 1111 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Unsigned | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 |
| Signed | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | −8 | −7 | −6 | −5 | −4 | −3 | −2 | −1 |

Notice that every word interpreted as a negative in this way has the top bit equal 1. Therefore, every word whose top bit is 1 is read as negative, and every word whose top bit is 0 is read as zero or positive. Equivalently, the top bit counts $-2^{n-1}$ in place of $+2^{n-1}$, so $1110$ is $-8 + 4 + 2 = -2$. This is **two's complement**.

Nothing about the counter changes, only the reading. $3 - 5$ still lands on 1110, and 1110 read signed is $-2$, the answer on the line. So the machine's own addition and subtraction already give the right result below zero, with no check, as long as the result stays in range. Outside that range is overflow, the second problem, which now starts at half the clock. The range is lopsided: $-2^{n-1}$ has no negation, since $2^{n-1}$ is out of range, and dividing it by $-1$ overflows the same way; on some machines (x86-64) that division stops the program instead of wrapping. This holds for the machine's own signed integers, and for the complex integer below.

*(Placeholder: the operations, each the machine's own: add, subtract, negate, multiply, divide. Then the cost: a bit spent on the sign, and the lopsided range, with $-2^{n-1}$ having no negation and $-2^{n-1} / -1$ overflowing, a trap on x86-64.)*

## Magnitude and direction

Splitting a number into a **magnitude**, how large it is, and a **direction**, which way it points, lets each problem be solved on its own. These notes solve the first: they build arithmetic operations that account for direction, so that results below zero come out right. The second needs a number system for large magnitudes, spec 13's size classes and pools, and is left aside here. In these notes a magnitude is treated as the machine already treats it, as a single word, and the examples keep magnitudes small, which is why the clock has only 16 hours.

**Magnitude.** A magnitude is a whole number, $0, 1, 2, \ldots$. Magnitudes can be added and multiplied, and the result is again a magnitude. They cannot always be subtracted: $3 - 5$ is not a magnitude. What can always be taken is the **absolute difference** $|a - b|$, the larger minus the smaller, which is a magnitude. In this, magnitudes are like modular arithmetic: neither has negative values, and neither has a direction. Adding magnitudes can overflow but never goes below zero. Subtracting them is where the first problem lives.

**Direction on the line.** The number line has two directions, positive and negative. A direction can be split off from a number as a power of $-1$: every number on the line is a magnitude times $(-1)^s$,

$$m \cdot (-1)^s,$$

with $s = 0$ for a positive number and $s = 1$ for a negative one, so $-5 = 5 \cdot (-1)^1$. Only whether $s$ is even or odd matters: $(-1)^s$ is 1 for every even $s$ and $-1$ for every odd $s$. So $s$ can be kept mod 2, as 0 for even and 1 for odd. Multiplying two numbers adds their exponents, $(-1)^{s_1} \cdot (-1)^{s_2} = (-1)^{s_1 + s_2}$, and two negatives make a positive because $1 + 1 \equiv 0 \pmod 2$. Negating adds 1 to $s$.

This is why the split works on a machine. Neither the magnitude nor the exponent is ever negative: the magnitude is kept mod $2^n$ and the exponent mod 2, and both suit modular arithmetic. The $-1$ itself is never computed; it is a fixed base that only says how to read the exponent. Two modular numbers, read together as $m \cdot (-1)^s$, give a value on the line, negative or not.

### A sign in a header

*(Placeholder: the line's version of the header. A number is a magnitude, one full word, and a sign kept outside it. The two facts about addition and the borrow, now in the header design below, are introduced here first, on two directions only. Pros and cons: all $n$ bits for the magnitude and a symmetric range, against an addition that needs the check and the borrow. In rounds 2 and 3 this is the header design restricted to directions 0 and 2.)*

## The plane

**Direction in the plane.** The complex plane has four directions along its two axes, the units $1, i, -1, -i$, and the same trick works with $i$ as the base. Every unit is a power of $i$, and the powers repeat every four steps, since $i^4 = 1$. So the exponent can be kept mod 4, and multiplying two directions adds their exponents:

$$i^{d_1} \cdot i^{d_2} = i^{d_1 + d_2}, \quad \text{with } d_1 + d_2 \text{ taken mod } 4.$$

Stored in a machine, the exponent is two bits, so a direction can be read three ways:

| Binary | 00 | 01 | 10 | 11 |
| --- | --- | --- | --- | --- |
| Exponent $d$ (mod 4) | 0 | 1 | 2 | 3 |
| Direction $i^d$ | 1 | $i$ | $-1$ | $-i$ |

The directions 1 and $-1$ lie on one line, the real axis, and $i$ and $-i$ on another, the imaginary axis. The line's two directions are contained in the plane's four: $d = 0$ and $d = 2$ are the line's $s = 0$ and $s = 1$. Negating is a half turn in both. The line has two directions, so a half turn is one step, adding 1 mod 2; the plane has four, so a half turn is two steps, adding 2 mod 4.

**A number** is then a pair $(m, d)$ of a magnitude $m$ and a direction $d$, with value

$$m \cdot i^d.$$

So $(5, 0)$ is 5, $(5, 2)$ is $-5$ and $(5, 1)$ is $5i$. On each axis, 1 and $i$ are the **positive** directions, and $-1$ and $-i$ the **negative** ones. Two directions are **opposite** when they differ by 2, as 1 and $-1$ do, and lie on **different axes** when they differ by 1, as 1 and $i$ do. A difference of 1 can run either way: 1 and $-i$ differ by $-1$, which is 3 mod 4.

**Zero** is any pair with magnitude 0. Zero is both real and imaginary, since it lies on both axes, and it is neither positive nor negative, since it lies in no direction. So its direction is irrelevant: $(0, d)$ is zero for every $d$. A zero keeps whatever direction the operation that produced it left, and no step sets it. Every number other than zero has exactly one pair.

### The axis in the word: the complex integer

The complex integer keeps the two bits of a direction apart. In the table of directions above, the low bit of $d$ is the **axis**: 0 for 1 and $-1$, 1 for $i$ and $-i$. The high bit is the **sign**: $d = 2s + a$. The complex integer stores the axis as a bit of its own and leaves the sign inside the value, where the machine already handles it.

#### A number

A number is a signed value $v$ and an axis $a$, with value

$$v \cdot i^a.$$

So $(5, 0)$ is 5, $(-5, 0)$ is $-5$ and $(5, 1)$ is $5i$. It is stored as one word,

$$w = 2v + a,$$

the axis in the lowest bit and $v$, in two's complement, in the $n - 1$ bits above it. Both come back out at once: $a$ is the lowest bit, and $v$ is $w$ read signed and halved, rounding down, which the machine does with one shift. On a 16-hour clock $v$ runs from $-4$ to 3; 3 is $0110$, $-2$ is $1100$ and $2i$ is $0101$.

**Zero** is $v = 0$ on either axis, the words 0 and 1. It keeps whatever axis the operation that produced it left.

#### The case check

There is one check, of the axes: the lowest bit of $w_1$ XOR $w_2$. There is no magnitude check, because nothing is subtracted as a magnitude: the sign is in the value, and the clock handles it.

```text
axis check (XOR)
├── same axis      → add the words, the second with its axis bit cleared
└── different axes → zero check
    ├── a zero     → the other number
    └── no zero    → an expression
```

The zero check is one comparison: $v = 0$ exactly when $w$ is 0 or 1, that is, when $w < 2$ read as a magnitude. Integers never reach it.

#### Addition

**Same axis.** Clearing the second word's axis bit and adding gives

$$w_1 + (w_2 - a_2) = 2v_1 + a_1 + 2v_2 = 2(v_1 + v_2) + a_1,$$

the sum's value on the first number's axis, which is also the second's. The signs take care of themselves. For example, $1 + (-3)$ is $0010 + 1010 = 1100$, and $1100$ is $-2$.

**Different axes.** If one value is zero, the sum is the other number, whole. If neither is, the sum, such as $3 + 4i$, is an expression, as in the header design.

#### Negation

Negation negates the value and keeps the axis:

$$-(v, a) = (-v, a), \qquad w \mapsto 2a - w,$$

since $2(-v) + a = 2a - (2v + a)$. Negating zero gives a zero on the same axis. The one value that cannot be negated is $-2^{n-2}$, since $2^{n-2}$ is out of range: that is overflow.

#### Subtraction

Subtraction needs no negation first. On the same axis,

$$w_1 - (w_2 - a_2) = 2(v_1 - v_2) + a_1,$$

the machine's subtraction in place of its addition, after the same check. On different axes, the difference is $w_1$ when $v_2 = 0$, and the negation of $w_2$ when $v_1 = 0$.

#### Multiplication

Multiplication multiplies the values and adds the axes. Two imaginary axes make the real axis with a turn of 2, since $i \cdot i = -1$:

$$(v_1, a_1) \cdot (v_2, a_2) = \big((-1)^{a_1 a_2}\, v_1 v_2,\ a_1 \text{ XOR } a_2\big).$$

In words, $v_1$ (the first word shifted) times $2v_2$ (the second word with its axis bit cleared) is $2 v_1 v_2$, negated when both axes are 1, with the axis bit $a_1$ XOR $a_2$ put back. For integers both axes are 0, and only the product remains.

#### Exact division

Exact division divides the values and subtracts the axes. Dividing by $i$ is multiplying by $-i$, so a real number divided by an imaginary one turns by 2:

$$(v_1, a_1) / (v_2, a_2) = \big((-1)^{(1 - a_1) a_2}\, v_1 / v_2,\ a_1 \text{ XOR } a_2\big),$$

when $v_2$ is not 0 and divides $v_1$. On words, both axis bits are cleared and the words divided, $2v_1 / 2v_2 = v_1 / v_2$, and the quotient is doubled back by a shift. Dividing the cleared first word by the shifted second, $2v_1 / v_2$, would save the shift, but it can divide $-2^{n-1}$ by $-1$, which overflows inside the division even when the result, as for $(-2^{n-2}) / (-i)$, is in range. With both words cleared the divisor is even, never $-1$.

#### What the complex integer gives up

Values run from $-2^{n-2}$ to $2^{n-2} - 1$: one bit goes to the axis, and one to the sign, so overflow comes sooner than in the header design, whose magnitudes use all $n$ bits. The complex integer solves the first problem the machine's way, by reading the clock signed, not by splitting the sign from the magnitude.

### The direction in a header: the header design

The header design stores each number as one such pair. Two facts about addition let it add with one rule, in place of a tree of cases.

#### Two facts about addition

Write a direction as two bits, $d = 2s + a$: the high bit $s$ is the **sign** (0 for 1 and $i$, 1 for $-1$ and $-i$), and the low bit $a$ is the **axis**. One XOR of two directions shows where their bits differ:

| $d_1$ \ $d_2$ | 00 | 01 | 10 | 11 |
| --- | --- | --- | --- | --- |
| **00** | 00 same | 01 diff | 10 opp | 11 diff |
| **01** | 01 diff | 00 same | 11 diff | 10 opp |
| **10** | 10 opp | 11 diff | 00 same | 01 diff |
| **11** | 11 diff | 10 opp | 01 diff | 00 same |

A high bit of 1 means the signs differ. A low bit of 1 means the axes differ. Integers only ever give 00 or 10.

**The signs decide add or subtract.** On one axis, the magnitudes add when the signs agree and subtract when they differ, since one number pulls against the other. On different axes, one magnitude must be zero, or the sum is an expression, and with a zero both ways agree: $m_1 + 0 = |m_1 - 0|$. So the axes never need checking to decide:

$$m = \begin{cases} m_1 + m_2 & s_1 = s_2 \\ |m_1 - m_2| & s_1 \ne s_2 \end{cases}$$

**The larger magnitude sets the direction.** In the same direction, both directions are the result's. In opposite directions, the larger number pulls the result its way. On different axes, the nonzero number is the larger. When the magnitudes are equal, either direction will do: the directions are the same, or the result is zero. So

$$d = \begin{cases} d_1 & m_1 \ge m_2 \\ d_2 & m_1 < m_2 \end{cases}$$

**One compare serves both.** The machine's subtraction reports whether it had to wrap below zero, as a schoolbook subtraction would need to borrow past its leftmost column. That report is the **borrow**:

$$b = \begin{cases} 1 & m_1 < m_2 \\ 0 & m_1 \ge m_2 \end{cases}$$

It is the compare that picks the direction, and it also gives the difference's magnitude, from the same subtraction. A borrow means the clock wrapped, and negating on the clock unwraps it:

$$|m_1 - m_2| \equiv (-1)^b \cdot (m_1 - m_2) \pmod{2^n}.$$

Both are modular, the direction mod 4 and the magnitude mod $2^n$, yet together, as $m \cdot i^d$, they give the answer on the line, even when it is below zero. They do nothing for overflow: a result of $2^n$ or more is still the second problem. The examples below are $5 + (-3)$, $4 + (-4)$ and $3 + (-5)$, on a 16-hour clock, so all modular arithmetic in the table is mod 16:

| Subtraction | Borrow $b$ | Direction | Modular magnitude | Linear result |
| --- | --- | --- | --- | --- |
| $5 - 3 \equiv 2$ | 0 | the first, 1 | $(-1)^0 \cdot 2 = 2$ | 2 |
| $4 - 4 \equiv 0$ | 0 | the first, 1 | $(-1)^0 \cdot 0 = 0$ | 0 |
| $3 - 5 \equiv 14$ | 1 | the second, $-1$ | $(-1)^1 \cdot 14 = -14 \equiv 2$ | $-2$ |

**Zero** needs no check. The rules give the right answer for a zero in any direction. The one test left is for an expression: different axes with neither magnitude zero.

**Not an order.** The XOR and the compare can say which of two numbers is larger, but only when they share an axis. For two nonzero numbers on different axes there is no answer. This is the **component order**: real parts are compared with real parts and imaginary with imaginary, and when both pairs differ, nothing is decided. It is not a total order. No total order can agree with multiplication either, since such an order makes every nonzero square positive, and $i^2 = -1$. So these checks are helpers inside addition, not a standalone comparison.

#### Addition

The two facts give one rule:

$$(m_1, d_1) + (m_2, d_2) = \big(m,\ d\big), \quad m = \begin{cases} m_1 + m_2 & s_1 = s_2 \\ |m_1 - m_2| & s_1 \ne s_2 \end{cases}, \quad d = \begin{cases} d_1 & m_1 \ge m_2 \\ d_2 & m_1 < m_2 \end{cases}$$

except that two nonzero numbers on different axes give an expression. For example:

- $3 + (-5) = (3, 0) + (5, 2)$: the signs differ, so $m = |3 - 5| = 2$, and the second is larger, so $d = 2$. The sum is $(2, 2) = -2$.
- $5 + (0, 1)$: the signs agree, so $m = 5 + 0 = 5$, and the first is larger, so $d = 0$. The sum is 5.
- $0 + (5, 3)$: the signs differ, so $m = |0 - 5| = 5$, and the second is larger, so $d = 3$. The sum is $(5, 3) = -5i$.

If neither magnitude is zero on different axes, a sum such as $3 + 4i$ is not a magnitude times a single direction, and the header design holds it as an expression, still to be worked out (see the end of these notes). Integers never reach this case.

The rule is proved in Lean against the true sum, for every pair of magnitudes below $2^{64}$ in all sixteen pairs of directions, along with the two facts it rests on ([`../spec/13-header/lean/Header/LargerRow.lean`](../spec/13-header/lean/Header/LargerRow.lean)).

#### Negation

Negation adds 2 to the direction:

$$-(m, d) = (m,\ d + 2),$$

since $-1 \cdot i^d = i^2 \cdot i^d = i^{d+2}$. The magnitude is untouched. Negating zero gives a zero.

#### Subtraction

Subtraction is addition of the negation: $a - b = a + (-b)$. Negating $b$ turns its direction by 2 and leaves its magnitude alone, so subtraction runs addition with the second direction turned. For example, $3 - 5$ is $(3, 0) + (5, 2)$: the signs differ and the second is larger, which gives $-2$, as in the table.

#### Multiplication

Multiplication multiplies the magnitudes and adds the directions:

$$(m_1, d_1) \cdot (m_2, d_2) = (m_1 m_2,\ d_1 + d_2).$$

This holds because $m_1 i^{d_1} \cdot m_2 i^{d_2} = m_1 m_2 \cdot i^{d_1 + d_2}$. For example, $(-3) \cdot (-5)$ is $(3, 2) \cdot (5, 2) = (15, 0)$, which is 15, since $2 + 2 = 4 \equiv 0$. A product with a zero factor has magnitude 0, so it is zero, whatever its direction. The additions here are the machine's own: of magnitudes, and of directions mod 4. None of them is the addition of numbers above, so multiplication does not depend on it.

#### Exact division

Exact division divides the magnitudes and subtracts the directions, when $m_2$ is not 0 and divides $m_1$:

$$(m_1, d_1) / (m_2, d_2) = (m_1 / m_2,\ d_1 - d_2).$$

### The direction packed in the word: the packed design

*(Placeholder: a magnitude and its 2-bit direction in one word, from the prior-art research (spec 13, prior-art.md). Not yet built or proved.)*

## The trade-off

| | Signed integers | Complex integer | Header | Packed |
| --- | --- | --- | --- | --- |
| A number | one word, the sign inside | one word: an axis bit and a signed value | one magnitude word and a direction in a header | one word: a magnitude and a 2-bit direction |
| Addition | one addition | an axis check; on the same axis, one addition | the sign bits choose add or subtract; one compare gives the direction | to come |
| Multiplication | one product | one product, axes XORed, negated when both are imaginary | one product, directions added | to come |
| Sums across axes | no axes | not held | not held | not held |
| Largest value | $2^{n-1} - 1$ | $2^{n-2} - 1$ | $2^n - 1$ | $2^{n-2} - 1$ |
| A value with no negation | $-2^{n-1}$ | $-2^{n-2}$ | none | none |

*(Placeholder: round 2's proved counts per operation, by chip.)*

## To return to

Points raised (Nov 1, 2026) and set aside for now.

- **The machine's answer to overflow.** The carry and overflow flags, which report when a result passes the end of the range.
- **The lopsided range.** Two's complement has one more negative value than positive, so $-2^{n-1}$ in Control and $-2^{n-2}$ in the complex integer have no negation, and dividing either by $-1$ overflows. The header design has no such value, since its magnitudes are symmetric about zero.

Expressions in the header design:

- **Two 1-bit fields or one 2-bit field.** A direction $d$ can be read as two bits, $d = 2s + a$: an axis bit $a$ (0 real, 1 imaginary) and a sign bit $s$ (0 positive, 1 negative). Multiplying directions is then $a = a_1 \text{ XOR } a_2$ and $s = s_1 \text{ XOR } s_2 \text{ XOR } (a_1 \text{ AND } a_2)$, the rule of the 2002 Clifford coprocessor in the survey. This is separate from storing an expression's terms in fixed slots by axis (slot 0 real, slot 1 imaginary), which the author accepted.
- **A mandatory plain form.** Whether an expression whose real or imaginary term becomes zero must become a plain (non-expression) number, so that each value has one form. The author doubts it fits a design with headers; equality would otherwise have to treat both forms as the same.
- **Don't build an expression only to collapse it.** For example $(3 + 4i)(3 - 4i) = 25$: work out both axis results first, and produce a plain number when one is zero.
- **Keeping the common case fast** (both numbers plain, on the same axis): agreed, method open.
- **Ordering expressions.** Once a number can carry both a real and an imaginary part, two numbers bring four parts to compare. Whether any order is wanted then (the lexicographic one, real part first, is total and agrees with addition but not multiplication) is open.
- **Zero's direction.** The survey's prior art fixes it; the author keeps it as it falls, since the operations here don't need it fixed.

Sources: [`research/expressions-machine-level.md`](research/expressions-machine-level.md).
