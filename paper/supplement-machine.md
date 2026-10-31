# Supplement: arithmetic at the machine level

*Draft. Chapters 3–5 (addition, subtraction, multiplication) are to come. Chapter 6 (division) is written. Every routine in Appendix A is checked by [`verify-machine/`](verify-machine/README.md).*

A computer does arithmetic on fixed-size groups of bits. This supplement explains that arithmetic from the ground up: first the mathematics it rests on, then how a machine carries it out, and then each operation. Its routines are given in pseudocode, and each is translated into machine code in Appendix A.

## 1. The mathematics

### 1.1 True and false

A **bit** holds one of two values: false or true, written 0 and 1. Four operations combine bits:

| $x$ | $y$ | NOT $x$ | $x$ AND $y$ | $x$ OR $y$ | $x$ XOR $y$ |
| --- | --- | --- | --- | --- | --- |
| 0 | 0 | 1 | 0 | 0 | 0 |
| 0 | 1 | 1 | 0 | 1 | 1 |
| 1 | 0 | 0 | 0 | 1 | 1 |
| 1 | 1 | 0 | 1 | 1 | 0 |

AND is true when both are true; OR when at least one is; XOR ("exclusive or") when exactly one is. Read as numbers, AND is multiplication and XOR is addition with the carry dropped: $1 + 1 = 2$, whose last bit is 0.

A machine applies these operations to whole groups of bits at once, bit by bit. For example, $1100$ AND $1010 = 1000$, and $1100$ XOR $1010 = 0110$.

### 1.2 Binary

A group of bits is read as a number in base 2. Each position is worth twice the one to its right: from the right, 1, 2, 4, 8, and so on. So $1101$ is $8 + 4 + 0 + 1 = 13$. In general, the bits $b_{n-1} \ldots b_1 b_0$ stand for

$$b_{n-1} \cdot 2^{n-1} + \cdots + b_1 \cdot 2 + b_0.$$

A **byte** is 8 bits, so it holds $2^8 = 256$ values: 0 to 255. A **word** is the group a machine works on in one step. On current machines a word is 64 bits, which holds the values 0 to $2^{64} - 1$, about $1.8 \times 10^{19}$. A word of $w$ bits holds $2^w$ values.

### 1.3 Powers of 2

Powers of 2 are to binary what powers of 10 are to decimal.

- **Shifting multiplies and divides.** Moving every bit $k$ places left multiplies by $2^k$. Moving every bit $k$ places right divides by $2^k$ and drops the remainder: $1101$ shifted right by 2 is $11$, and $13 \div 4 = 3$ remainder 1.
- **The low bits are a remainder.** The lowest $k$ bits of $n$ are the remainder of $n$ divided by $2^k$. They can be picked out with AND: $n$ AND $(2^k - 1)$, since $2^k - 1$ is $k$ ones.
- **Every positive integer is a power of 2 times an odd number.** Write $n = 2^s \cdot o$ with $o$ odd. Then $s$ is the number of zeros at the right end of $n$ (its **trailing zeros**), and $o$ is $n$ shifted right by $s$. For example, $12 = 1100 = 2^2 \cdot 3$.

### 1.4 Modular arithmetic

Dividing $a$ by $m$ gives a quotient $q$ and a remainder $r$ with $a = q \cdot m + r$ and $0 \leq r < m$. The remainder is written $a \bmod m$. Two numbers are **congruent mod $m$**, written $a \equiv b \pmod m$, when they have the same remainder, which is the same as saying $m$ divides $a - b$.

Sums and products respect congruence: if $a \equiv a'$ and $b \equiv b' \pmod m$, then $a + b \equiv a' + b'$ and $a \cdot b \equiv a' \cdot b' \pmod m$.

> *Proof.* Write $a' = a + jm$ and $b' = b + km$. Then $a' + b' = a + b + (j + k)m$, and $a' b' = ab + (ak + bj + jkm)m$. Each differs from the original by a multiple of $m$. ∎

So a long calculation can be reduced mod $m$ at every step, and the result is still right mod $m$.

This is the arithmetic of a machine word. A word keeps only $w$ bits. When a sum or product needs more, the bits above the top are lost, and what remains is the result mod $2^w$. For example, in a byte, $200 + 100 = 300 = 256 + 44$ leaves 44, and $16 \cdot 20 = 320$ leaves 64. By the rule above, a chain of additions and multiplications on words gives the true result mod $2^w$.

### 1.5 Counting cost below big-O

Big-O describes how the cost of a calculation grows as its inputs grow. The main paper uses it this way: long division of two $n$-digit numbers takes $O(n^2)$ steps, so doubling the digits roughly quadruples the work. Big-O keeps only the shape of the growth and drops constant factors, so $3n^2$ and $50n^2$ steps are both $O(n^2)$.

At the machine level the inputs don't grow. Every number is one word of 64 bits, so every routine in this supplement takes $O(1)$ time. Big-O can't tell them apart: the constant it drops is the whole difference between them.

So this supplement counts cost directly. A routine is a list of machine instructions, and its cost is built up from what each instruction costs, counted in the machine's own steps (chapter 2 says what those are). This is the constant that big-O sets aside. It comes back into big-O for numbers of many words: long division of $n$-word numbers is still $O(n^2)$ word operations, and the cost of each word operation, counted here, is the constant in front.

## 2. Machine code

A program in a language such as Nim, C or Rust is translated by a compiler into machine code: a list of instructions the chip carries out directly. Machine code works differently from those languages. There are no named variables, no expressions and no loops. There are a few places to hold numbers, a few dozen simple instructions, and jumps.

### 2.1 Registers, memory and flags

- **Registers** are the chip's own places for numbers. An x86-64 chip has 16 general registers, each one word, with names such as `rax`, `rdx` and `r8`. An instruction reads and writes registers, and a register's value changes whenever an instruction writes to it. The low 32 bits of a register have their own names (`eax` is the low half of `rax`) for 32-bit instructions.
- **Memory** is a long row of bytes, numbered. An instruction can load a value from a numbered place in memory into a register.
- **Flags** are single bits that most arithmetic instructions set as a side effect. The **carry flag** holds the bit that fell off the top of an addition or a shift, or the borrow of a subtraction. The **zero flag** says whether the result was 0.

### 2.2 Instructions

These are the instructions this supplement uses. All arithmetic is mod $2^{64}$, as in section 1.4.

| Instruction | Effect |
| --- | --- |
| `mov` | copy a value into a register, or between a register and memory |
| `add`, `sub`, `neg` | add, subtract, negate |
| `and`, `or`, `xor` | the operations of section 1.1, bit by bit |
| `shl`, `shr` | shift left or right (section 1.3) |
| `rcl` | shift left by one, bringing the carry flag in at the bottom |
| `imul` | multiply, keeping the low word of the product |
| `div` | divide (section 2.3) |
| `tzcnt` | count trailing zeros |
| `lea` | add, with small multiples: `lea rax, [r8+r8*2]` sets `rax` to $3 \cdot$ `r8` |
| `movzx` | load one byte from memory into a register, filling the rest with zeros |
| `cmp` | subtract without keeping the result, only setting the flags |
| `jb`, `jc`, `jnz`, `jmp` | jump if below, if carry, if not zero, or always |

### 2.3 The divide instruction

`div r` divides a two-word number by the register `r`. The high word of the dividend must be in `rdx` and the low word in `rax`. It puts the quotient in `rax` and the remainder in `rdx`. To divide one word by another, a program first sets `rdx` to 0. There is also a 32-bit form, which divides `edx:eax` by a 32-bit register.

The division itself is not done by instructions. It is done by a circuit inside the chip, the **divider**, which carries out long division (section 6.1).

### 2.4 How long instructions take

A chip runs on a clock, and its time is counted in **cycles**. It has several units that do the work: adders, a multiplier, a divider, and units that load from memory. Two facts about them matter here.

- **Independent instructions overlap.** If an instruction doesn't need the result of an earlier one, the chip may start it before the earlier one finishes, on another unit or on the same unit if that unit can take new work while it is still busy. So when a program does many divisions that don't depend on each other, what limits its speed is how often the busiest unit can start a new instruction.
- **The multiplier overlaps with itself, and the divider doesn't.** A product is a sum of shifted copies of one factor, one copy for each 1 bit of the other. All the copies are known at the start, so a circuit can add them in pairs, then the pairs in pairs, like a tournament. That takes a few stages, and each stage can work on a different multiplication, so a new multiplication can start every cycle. Long division is different: each step needs the remainder from the step before, so the steps form a chain. The divider is busy with one division until the chain is done.

Published measurements of each instruction show this ([uops.info](https://uops.info)). For two families of current chips, here is how often each unit can start a new instruction:

| Instruction | Intel (Ice Lake, Tiger Lake, Rocket Lake, Alder Lake) | AMD (Zen 3, Zen 4) |
| --- | --- | --- |
| 64-bit `imul` | every cycle | every cycle |
| 64-bit `div` | every 10 cycles | every 7 cycles |
| 32-bit `div` | every 6 cycles | every 6 cycles |

A multiplication takes 3 cycles from start to result on both. The simple instructions (`add`, `shr`, `mov` and so on) take one cycle, and several units can run them side by side.

### 2.5 Pseudocode for machine code

The main paper's pseudocode gives each name a value once. Machine code doesn't work that way, so this supplement uses a variation:

- `ROUTINE name(inputs)` starts a routine. Its inputs arrive in registers.
- `x ← e` sets the register `x` to the value `e`. Unlike `LET` in the main paper, a register can be set again; its value is the last one set.
- All arithmetic is mod $2^{64}$: `x·y` is the low word of the product.
- `SHIFT LEFT`, `SHIFT RIGHT`, `AND`, `OR`, `XOR` and `TRAILING ZEROS` are the operations of section 1.
- `DIVIDE(a, b)` is the 64-bit divide instruction of section 2.3, and `DIVIDE32(a, b)` is the 32-bit one.
- `TABLE[i]` loads byte `i` of a table kept in memory.
- `IF condition THEN GO TO label` jumps to the line marked `label:` when the condition holds; otherwise the next line runs.
- `REPEAT n TIMES` runs the indented lines under it $n$ times.
- `RETURN` ends the routine with its results.

Each line becomes one to five instructions. Appendix A gives them line by line.

## 3. Addition

*To come.*

## 4. Subtraction

*To come.*

## 5. Multiplication

*To come.*

## 6. Division

The divisions here are **exact**: the divisor goes into the dividend with no remainder. This is the division that reduces a fraction to lowest terms, where the numerator and the denominator are both divided by their gcd. So each routine divides two dividends, $a$ and $a_2$, by the same divisor $b$, and returns both quotients.

The section starts with the standard way, which is what compiled programs do. Three ideas improve on it, each compared with the standard way. Last comes the combination this work uses, compared with the best of the other ideas.

### 6.1 The standard way: long division

The standard routine uses the divide instruction twice:

```text
ROUTINE divide_pair(a, a2, b)
    q  ← DIVIDE(a, b)
    q2 ← DIVIDE(a2, b)
    RETURN q, q2
```

Inside the instruction, the divider does long division in base 2. It takes the dividend's bits from the top, one at a time. Each time, it brings the next bit down into the remainder, and if the remainder has reached the divisor, it subtracts the divisor and sets the next quotient bit to 1:

```text
ROUTINE long_division(a, b)
    q ← 0
    r ← 0
    REPEAT 64 TIMES
        r ← 2·r + (top bit of a)
        a ← a SHIFT LEFT 1
        q ← 2·q
        IF r ≥ b THEN
            r ← r − b
            q ← q + 1
    RETURN q
```

Here `r` can need one bit more than a word, since it can reach almost $2b$. The machine code in Appendix A keeps that extra bit in the carry flag.

**Cost.** Each division is a chain of 64 steps, and each step needs the remainder from the one before. A real divider settles several quotient bits per cycle rather than one, but it still works through the chain, and it can't start the next division until it finishes. A 64-bit division keeps the divider busy for 7 to 10 cycles (section 2.4). A pair of divisions costs 14 to 20 cycles of divider time.

### 6.2 First idea: the 32-bit divide

**The math.** A number below $2^{32}$ has only zeros in its top 32 bits. If the dividend is below $2^{32}$, long division spends its first 32 steps bringing down zeros, and those steps produce only zero quotient bits. So a 32-step long division gives the same quotient. The 32-bit divide instruction does exactly that.

The test is one OR. The OR of several numbers has a 1 in some position exactly when at least one of them does, so $a$ OR $a_2$ OR $b$ is below $2^{32}$ exactly when all three are.

The divisor has to be in the test. When the dividends are not 0, an exact divisor is no larger than they are, so it is below $2^{32}$ too. But when both dividends are 0, the divisor can be any size. The 32-bit divide would use only its low 32 bits, and if those are all 0 (as for $b = 2^{32}$), it would divide by zero.

```text
ROUTINE divide_pair_32(a, a2, b)
    t ← (a OR a2 OR b) SHIFT RIGHT 32
    IF t ≠ 0 THEN GO TO wide
    q  ← DIVIDE32(a, b)
    q2 ← DIVIDE32(a2, b)
    RETURN q, q2
wide:
    q  ← DIVIDE(a, b)
    q2 ← DIVIDE(a2, b)
    RETURN q, q2
```

**How it differs from the standard way.** It adds one test, and when the test passes it uses the 32-bit divide in place of the 64-bit one.

**Compared with the standard way.** When all three numbers are below $2^{32}$, the pair costs 12 cycles of divider time instead of 20 on the Intel chips, and 12 instead of 14 on the AMD chips. Above $2^{32}$ the test fails, and the cost is the standard way's plus the test, which is a few simple instructions that run alongside it.

**Where it helps.** Only when the dividends and the divisor are all below $2^{32}$. Above that, it saves nothing.

### 6.3 Second idea: reusing the inverse

**The math.** In ordinary arithmetic, dividing by 3 is multiplying by $\frac{1}{3}$. Mod $2^{64}$ there are no fractions, but some numbers still have an **inverse**: a number $x$ with $o \cdot x \equiv 1 \pmod{2^{64}}$. For example, mod $2^8$ the inverse of 3 is 171, since $3 \cdot 171 = 513 = 2 \cdot 256 + 1$.

- **Odd numbers have inverses, and even numbers don't.** An even number times anything is even, so it can't be congruent to 1. The steps below construct an inverse for every odd number.
- **Newton's step doubles the correct bits.** Suppose $o \cdot x \equiv 1 \pmod{2^k}$, and let $x' = x \cdot (2 - o \cdot x)$. Then $o \cdot x' \equiv 1 \pmod{2^{2k}}$.

  > *Proof.* Let $e = 1 - o \cdot x$. By assumption, $2^k$ divides $e$. Then $o \cdot x' = o \cdot x \cdot (2 - o \cdot x) = (1 - e)(1 + e) = 1 - e^2$, and $2^{2k}$ divides $e^2$. ∎

  For any odd $o$, $x = 1$ is right mod 2, since $o \cdot 1$ is odd. So six steps reach $2^{64}$ (1, 2, 4, 8, 16, 32, 64 bits), and every odd number has an inverse.
- **A better start saves steps.** The start $x = (3 \cdot o)$ XOR $2$ is already right mod $2^5$, for every odd $o$ (a check of the 16 odd numbers below 32 proves it). From 5 correct bits, four steps reach 64: 10, 20, 40, then 80, which covers all 64.
- **Exact division is multiplication by the inverse.** Write the divisor as $b = 2^s \cdot o$ with $o$ odd (section 1.3). If $a = b \cdot q$, then $a = 2^s \cdot o \cdot q$, so shifting $a$ right by $s$ loses nothing and gives $o \cdot q$. Multiplying by the inverse $x$ of $o$ gives $o \cdot q \cdot x \equiv q \pmod{2^{64}}$. Since $q$ fits in a word, the result is $q$ itself.

Working out the inverse takes four Newton steps. Once it is known, each dividend takes one shift and one multiplication. For two dividends with the same divisor, the inverse is worked out once and used twice:

```text
ROUTINE divide_pair_reuse(a, a2, b)
    s ← TRAILING ZEROS(b)
    o ← b SHIFT RIGHT s
    x ← (3·o) XOR 2                     right mod 2^5
    x ← x·(2 − o·x)                     2^10
    x ← x·(2 − o·x)                     2^20
    x ← x·(2 − o·x)                     2^40
    x ← x·(2 − o·x)                     2^64
    q  ← (a SHIFT RIGHT s)·x
    q2 ← (a2 SHIFT RIGHT s)·x
    RETURN q, q2
```

**How it differs from the standard way.** There is no divide instruction at all. In its place are one trailing-zero count, four Newton steps of two multiplications each, and a shift and a multiplication for each dividend.

**Compared with the standard way.** The routine uses 10 multiplications and no divisions. The multiplier can start one every cycle, so a pair costs about 10 cycles of multiplier time, against 14 to 20 cycles of divider time for the standard way. Its other instructions are simple ones that run alongside.

**Where it helps.**
- **Exact division only.** If the divisor doesn't go into the dividend, the result is not the quotient or anything near it. Mod $2^8$, "$7 \div 3$" by this method is $7 \cdot 171 = 1197$, which leaves 173. This is the case this work needs, since reducing a fraction by its gcd always divides exactly.
- **The same divisor more than once.** Reuse pays off when one divisor divides several numbers, as a gcd divides a numerator and a denominator. With one dividend, the inverse costs 9 multiplications for a single quotient.

It uses no memory beyond its registers.

### 6.4 Third idea: a table to start from

**The math.** Each Newton step doubles the correct bits, so a start with more correct bits saves steps. A start right mod $2^8$ needs three steps (16, 32, 64) instead of four. Such a start can be looked up. The inverse mod $2^8$ depends only on the lowest 8 bits of $o$, which form an odd number from 1 to 255. A table of the 128 inverses, one for each odd number below 256, gives the start. The entry for $o$ is at place $(o$ SHIFT RIGHT $1)$ AND $127$, which is $(o \bmod 256 - 1)/2$.

```text
ROUTINE inverse_from_table(b)
    s ← TRAILING ZEROS(b)
    o ← b SHIFT RIGHT s
    x ← TABLE[(o SHIFT RIGHT 1) AND 127]       right mod 2^8
    x ← x·(2 − o·x)                             2^16
    x ← x·(2 − o·x)                             2^32
    x ← x·(2 − o·x)                             2^64
    RETURN x, s

ROUTINE divide_pair_table(a, a2, b)
    x, s ← inverse_from_table(b)
    q  ← (a SHIFT RIGHT s)·x
    x, s ← inverse_from_table(b)
    q2 ← (a2 SHIFT RIGHT s)·x
    RETURN q, q2
```

Here the inverse is worked out again for each dividend. That isolates this idea: it is division by the inverse, starting from the table, without reuse.

**How it differs from the standard way.** As with reuse, there is no divide instruction. Each dividend takes one table lookup, three Newton steps, a shift and a multiplication.

**Compared with the standard way.** The routine uses 14 multiplications, 7 for each dividend, so a pair costs about 14 cycles of multiplier time against 14 to 20 cycles of divider time. It gains on the Intel chips and about breaks even on the AMD chips. Alone, it is the weakest of the three ideas.

**Where it helps.** Like reuse, it gives the right answer only for exact division. Its cost is memory: the table is 128 bytes, one byte per entry. The table is part of the program. It is made once, when the program is built, and every division reads the same table, so it never grows and is never freed. 128 bytes is two of the 64-byte blocks that a chip's fastest memory (its cache) holds, so lookups are quick.

A bigger table gives little more. A start right mod $2^{16}$ would save one more step, but it needs 32,768 entries of 2 bytes, or 64 KiB. That is larger than the fastest cache on current chips (32 to 48 KiB), so lookups would often wait on slower memory.

### 6.5 The combination: the table and reuse

This work uses the second and third ideas together: the inverse starts from the table and is worked out once, then used for both dividends.

```text
ROUTINE divide_pair_combined(a, a2, b)
    x, s ← inverse_from_table(b)
    q  ← (a SHIFT RIGHT s)·x
    q2 ← (a2 SHIFT RIGHT s)·x
    RETURN q, q2
```

It uses 8 multiplications: 6 for the inverse and one for each dividend. That is fewer than either idea alone (10 for reuse, 14 for the table). It keeps the limits of both: exact division only, it pays most when a divisor is used more than once, and it uses 128 bytes of table.

**Compared with the 32-bit divide.** The 32-bit divide helps only when the dividends and the divisor are all below $2^{32}$, so that is the one case to compare. There:

| Routine | Work for a pair | Busiest unit's time |
| --- | --- | --- |
| 32-bit divide | 2 divisions of 32 steps | 12 cycles of divider time |
| table and reuse | 8 multiplications | about 8 cycles of multiplier time |

The figures are the same on both chip families. The reason is the one in section 2.4. Long division is a chain of steps, each waiting on the last, so the divider handles one division at a time. Multiplication is a sum of parts that are all known at once, so the multiplier starts a new one every cycle. The 32-bit divide shortens the chain to 32 steps, but it is still a chain. The combination replaces the chain with a few multiplications.

So adding the 32-bit divide to the combination wouldn't help: below $2^{32}$ it would replace 8 multiplications with 12 cycles of divider time. Above $2^{32}$ the combination costs the same 8 multiplications, while the standard way costs 14 to 20 cycles.

As section 1.5 explains, big-O can't tell these routines apart, since all of them are $O(1)$. The difference is in the constant, counted here in cycles of the busiest unit.

**One limit of this comparison.** The comparison assumes many divisions that don't depend on each other, so that the chip can overlap them. That is the case in reducing a list or a matrix of fractions. A single division whose result is needed right away is different: then what counts is the time from start to result. The combination's steps form a chain of their own (the lookup, then three Newton steps of two multiplications each), which takes longer from start to result than one divide instruction.

## Appendix A. The machine code

The routines of chapter 6 in x86-64 machine code, Intel syntax. The full file is [`verify-machine/division.s`](verify-machine/division.s), and [`verify-machine/test.c`](verify-machine/test.c) checks every routine against the exact quotient.

The pair routines take $a$ in `rdi`, $a_2$ in `rsi` and $b$ in `rdx`, and write the two quotients to the memory that `rcx` points to. Shifts by a number held in a register must take it from `cl`, the low byte of `rcx`, so routines that shift first move `rcx` to `r10`. The comments name the pseudocode line each instruction carries out.

### A.1 The standard way (section 6.1)

```text
pair_control:
    mov     r8, rdx             # b
    mov     rax, rdi            # low word of the dividend: a
    xor     edx, edx            # high word: 0
    div     r8                  # q ← DIVIDE(a, b), in rax
    mov     [rcx], rax
    mov     rax, rsi            # a2
    xor     edx, edx
    div     r8                  # q2 ← DIVIDE(a2, b)
    mov     [rcx+8], rax
    ret
```

The long division that `div` does inside, written out as instructions. It takes $a$ in `rdi` and $b$ in `rsi` and returns $q$ in `rax`. The remainder is in `rdx`, and its extra bit is in the carry flag:

```text
long_division:
    xor     eax, eax            # q ← 0
    xor     edx, edx            # r ← 0
    mov     ecx, 64             # REPEAT 64 TIMES
1:  shl     rdi, 1              # a ← a SHIFT LEFT 1; its top bit goes to the carry flag
    rcl     rdx, 1              # r ← 2·r + that bit; r's own top bit goes to the carry flag
    jc      2f                  # r is past 2^64, so r ≥ b
    shl     rax, 1              # q ← 2·q
    cmp     rdx, rsi            # IF r ≥ b
    jb      3f
    or      rax, 1              #     q ← q + 1
    sub     rdx, rsi            #     r ← r − b
    jmp     3f
2:  shl     rax, 1              # q ← 2·q
    or      rax, 1              # q ← q + 1
    sub     rdx, rsi            # r ← r − b
3:  dec     ecx
    jnz     1b
    ret
```

### A.2 The 32-bit divide (section 6.2)

```text
pair_32:
    mov     r8, rdx             # b
    mov     rax, rdi
    or      rax, rsi
    or      rax, r8             # a OR a2 OR b
    shr     rax, 32             # t ← (a OR a2 OR b) SHIFT RIGHT 32
    jnz     4f                  # IF t ≠ 0 THEN GO TO wide
    mov     eax, edi
    xor     edx, edx
    div     r8d                 # q ← DIVIDE32(a, b)
    mov     [rcx], rax
    mov     eax, esi
    xor     edx, edx
    div     r8d                 # q2 ← DIVIDE32(a2, b)
    mov     [rcx+8], rax
    ret
4:  mov     rax, rdi            # wide:
    xor     edx, edx
    div     r8                  # q ← DIVIDE(a, b)
    mov     [rcx], rax
    mov     rax, rsi
    xor     edx, edx
    div     r8                  # q2 ← DIVIDE(a2, b)
    mov     [rcx+8], rax
    ret
```

### A.3 Reusing the inverse (section 6.3)

One Newton step, $x \leftarrow x \cdot (2 - o \cdot x)$, with $o$ in `r8` and $x$ in `rax`, is these five instructions. Below, each `newton` stands for them:

```text
    mov     r9, r8
    imul    r9, rax             # o·x
    neg     r9
    add     r9, 2               # 2 − o·x
    imul    rax, r9             # x ← x·(2 − o·x)
```

```text
pair_reuse:
    mov     r10, rcx            # where the quotients go
    tzcnt   rcx, rdx            # s ← TRAILING ZEROS(b)
    mov     r8, rdx
    shr     r8, cl              # o ← b SHIFT RIGHT s
    lea     rax, [r8+r8*2]      # 3·o
    xor     rax, 2              # x ← (3·o) XOR 2
    newton                      # 2^10
    newton                      # 2^20
    newton                      # 2^40
    newton                      # 2^64
    shr     rdi, cl
    imul    rdi, rax            # q ← (a SHIFT RIGHT s)·x
    mov     [r10], rdi
    shr     rsi, cl
    imul    rsi, rax            # q2 ← (a2 SHIFT RIGHT s)·x
    mov     [r10+8], rsi
    ret
```

### A.4 The table (section 6.4)

`inverse_from_table`, with $b$ in `rdx`. It leaves $x$ in `rax` and $s$ in `cl`. Below, `table_inverse` stands for these lines:

```text
    tzcnt   rcx, rdx            # s ← TRAILING ZEROS(b)
    mov     r8, rdx
    shr     r8, cl              # o ← b SHIFT RIGHT s
    mov     r9, r8
    shr     r9, 1
    and     r9, 127             # (o SHIFT RIGHT 1) AND 127
    lea     r11, [rip+inv_table]
    movzx   eax, byte ptr [r11+r9]   # x ← TABLE[...]
    newton                      # 2^16
    newton                      # 2^32
    newton                      # 2^64
```

```text
pair_table:
    mov     r10, rcx
    table_inverse               # x, s ← inverse_from_table(b)
    shr     rdi, cl
    imul    rdi, rax            # q ← (a SHIFT RIGHT s)·x
    mov     [r10], rdi
    table_inverse               # x, s ← inverse_from_table(b), again
    shr     rsi, cl
    imul    rsi, rax            # q2 ← (a2 SHIFT RIGHT s)·x
    mov     [r10+8], rsi
    ret
```

The table: entry $i$ is the inverse of $2i + 1$ mod 256. It is 128 bytes, kept in the program's read-only memory.

```text
inv_table:
    .byte 0x01, 0xab, 0xcd, 0xb7, 0x39, 0xa3, 0xc5, 0xef, 0xf1, 0x1b, 0x3d, 0xa7, 0x29, 0x13, 0x35, 0xdf
    .byte 0xe1, 0x8b, 0xad, 0x97, 0x19, 0x83, 0xa5, 0xcf, 0xd1, 0xfb, 0x1d, 0x87, 0x09, 0xf3, 0x15, 0xbf
    .byte 0xc1, 0x6b, 0x8d, 0x77, 0xf9, 0x63, 0x85, 0xaf, 0xb1, 0xdb, 0xfd, 0x67, 0xe9, 0xd3, 0xf5, 0x9f
    .byte 0xa1, 0x4b, 0x6d, 0x57, 0xd9, 0x43, 0x65, 0x8f, 0x91, 0xbb, 0xdd, 0x47, 0xc9, 0xb3, 0xd5, 0x7f
    .byte 0x81, 0x2b, 0x4d, 0x37, 0xb9, 0x23, 0x45, 0x6f, 0x71, 0x9b, 0xbd, 0x27, 0xa9, 0x93, 0xb5, 0x5f
    .byte 0x61, 0x0b, 0x2d, 0x17, 0x99, 0x03, 0x25, 0x4f, 0x51, 0x7b, 0x9d, 0x07, 0x89, 0x73, 0x95, 0x3f
    .byte 0x41, 0xeb, 0x0d, 0xf7, 0x79, 0xe3, 0x05, 0x2f, 0x31, 0x5b, 0x7d, 0xe7, 0x69, 0x53, 0x75, 0x1f
    .byte 0x21, 0xcb, 0xed, 0xd7, 0x59, 0xc3, 0xe5, 0x0f, 0x11, 0x3b, 0x5d, 0xc7, 0x49, 0x33, 0x55, 0xff
```

### A.5 The table and reuse (section 6.5)

```text
pair_table_reuse:
    mov     r10, rcx
    table_inverse               # x, s ← inverse_from_table(b)
    shr     rdi, cl
    imul    rdi, rax            # q ← (a SHIFT RIGHT s)·x
    mov     [r10], rdi
    shr     rsi, cl
    imul    rsi, rax            # q2 ← (a2 SHIFT RIGHT s)·x
    mov     [r10+8], rsi
    ret
```

## Sources

- [uops.info: DIV (R64)](https://uops.info/html-instr/DIV_R64.html), [DIV (R32)](https://uops.info/html-instr/DIV_R32.html) and [IMUL (R64, R64)](https://uops.info/html-instr/IMUL_R64_R64.html): measured latency and throughput of each instruction on each chip family.
