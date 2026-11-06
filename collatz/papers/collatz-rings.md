# Invariants of Collatz maps across rings of integers

*Draft, 6 November 2026. Code, logs and Lean proofs: `collatz/research/`. Results and their status: `collatz/catalog.md`. Citations are markers; the bibliography is written last, from a review of the finished text.*

## Abstract

*(Placeholder: written last.)*

## 1. Introduction

Start with a positive whole number. If it is even, halve it. If it is odd, multiply it by 3 and add 1. Then do the same to the result, and keep going. From 3 the numbers run

$$3 \to 10 \to 5 \to 16 \to 8 \to 4 \to 2 \to 1,$$

and from 1 they cycle, $1 \to 4 \to 2 \to 1$. The **Collatz conjecture** says that every positive whole number eventually reaches 1 [Lagarias 1985]. It has been checked beyond $10^{20}$, and it is not proved.

An instruction of this kind, which divides even numbers by 2 and sends an odd number $n$ to $mn + d$ for a fixed multiplier $m$ and a digit $d$ chosen by the remainder of $n$, is a **map**. The one above is the $3x+1$ map.

### 1.1 Negative numbers

Nothing in this instruction needs the number to be positive. Applied to $-5$ it gives

$$-5 \to -14 \to -7 \to -20 \to -10 \to -5,$$

a second cycle, and there are two more, through $-1$ and through $-17$ [Lagarias 1985]. So on all the integers the $3x+1$ map does something different from what it does on the positive ones. The reason is visible in the first step. From $5$ the map goes to $16$, and from $-5$ it goes to $-14$. The orbit of $-5$ is not the orbit of $5$ with the signs flipped.

Other maps behave better. Barina's map [Barina 2018] halves even numbers, and sends an odd $n$ to $7n + 1$ if $n$ leaves remainder 1 on division by 4, and to $7n - 1$ if it leaves remainder 3. From $5$ and from $-5$ it gives

$$5 \to 36 \to 18 \to 9 \to 64 \to \cdots \to 1, \qquad -5 \to -36 \to -18 \to -9 \to -64 \to \cdots \to -1.$$

Here the negative orbit is the positive one with the signs flipped, for every starting number. Barina checked that every positive number up to $10^{15}$ reaches 1, and noted that the negative numbers then take care of themselves.

The positive integers can be thought of as the nonzero integers with each number glued to its negative: $5$ and $-5$ are one object, and the positive number $5$ is the name we give it. Barina's map respects the gluing. It sends $5$ and $-5$ to $36$ and $-36$, which are glued to each other, so it makes sense as a map on the glued objects, and that map is the one on the positive integers. The $3x+1$ map does not respect the gluing, since $16$ and $-14$ are not glued.

### 1.2 Two kinds of map

The two maps choose what to add in different ways. Barina's map looks at the number's remainder on division by 4. A map of this kind is defined on all the integers at once, and when it respects the gluing it is a map on the positive integers too. We call such a map a **Barina-type map**.

The $3x+1$ map cannot be written this way. On the positive integers it adds 1 to every odd number. To extend it to all the integers so that it respects the gluing, it has to add $-1$ to the negative odd numbers, $3z + 1$ for $z > 0$ and $3z - 1$ for $z < 0$. What it adds then depends on the sign of $z$, and no remainder, on division by 4 or by any power of 2, tells the sign. What the $3x+1$ map actually does is pick one member of each glued pair, the positive one, and apply a formula to it. We call a map of this kind a **Collatz-type map**.

On the integers the difference is easy to miss, because there is only one sensible choice of member, the positive one. It becomes visible when there are more than two numbers to glue.

### 1.3 Other number systems

The **Gaussian integers** are the numbers $a + bi$ with $a$ and $b$ whole numbers and $i^2 = -1$. They form a square grid in the plane. Four of them have inverses, $1, i, -1, -i$; these are the **units**, the counterparts of $1$ and $-1$ in the integers. Multiplying by $i$ turns a number a quarter turn about $0$, so gluing each number to its unit multiples puts four numbers together. The class of $2 + i$ is

$$\{\, 2 + i,\ -1 + 2i,\ -2 - i,\ 1 - 2i \,\},$$

one number in each quarter of the plane. Numbers glued in this way are called **associates**, and the glued classes play the part that the positive integers play for the integers.

A Collatz-type map needs one member of each class. There are now two natural ways to choose. A geometric choice takes the member in a fixed quarter of the plane, for example the one whose angle lies between $-45^\circ$ and $45^\circ$, which for the class above is $2 + i$. An arithmetic choice takes the member with a fixed remainder, for example the one that leaves remainder 1 on division by $2 + 2i$, which for the class above is $-1 + 2i$. Barina-type maps behave like the arithmetic choice, because they read remainders.

The two choices give different answers to a simple question: what is the smallest multiplier for which a map of this kind appears to send every nonzero number to a unit? With the geometric choice the answer is $2 + i$, whose size, measured by the **norm** $a^2 + b^2$, is $5$. With the arithmetic choice, and for Barina-type maps, it is $5 + 2i$, of norm $29$. Both answers rest on computer checks of every start up to a large norm; that nothing smaller works is proved for Barina-type maps. Seen from the Gaussian integers, the difference between the $3x+1$ map and Barina's map is the difference between a geometric and an arithmetic way of naming the glued classes. It was there in the integers all along.

The same constructions work in other number systems of the same kind, the **rings of integers** of number theory, each with its own units and its own way of gluing. They differ in a few quantities: how many units there are, how close the units come to 1, how the number 2 breaks into primes, and how many ways there are to measure size. This paper asks which of these quantities control the behaviour of Collatz-type and Barina-type maps, and which stop mattering as the ring grows. A quantity that keeps controlling the maps from ring to ring is an **invariant** of the problem.

### 1.4 Earlier work

Maps that multiply on each remainder class and then divide go back to Hasse, and their growth was studied by Heppner, by Matthews and Watts, and by Leigh [Heppner 1978; Matthews and Watts 1985; Leigh 1986]. Their criterion for a map that grows in the worst case and shrinks on average, rewritten with norms, decides which multipliers are worth studying in every ring of this paper. Matthews treated such maps on rings of integers and suggested a systematic study [Matthews 2010, Sections 10 and 11]. On the Gaussian integers, Joseph, Kucinski and a 2017 research group studied particular maps [Joseph 1998; Kucinski 2000; Alvarado 2025].

Gluing by a symmetry has one clear precedent. Monks and Yazinski found a hidden symmetry of the $3x+1$ map on the 2-adic numbers, and restated the conjecture on the classes it glues together [Monks and Yazinski 2004]. We do the same with a simpler symmetry, multiplication by the units, which every ring has.

### 1.5 This paper

The paper follows the order in which a reader meets the ideas. Part I stays in the integers. It moves from the positive integers to all the integers, records what changes, and proves each observation for the integers only. Some of these facts look heavier than they need to be when there are only two units. Part II states them once, for any ring of integers, where they earn their weight. Part III tests the general statements: first in the Gaussian integers, then in two rings chosen because they differ from the Gaussian integers in the ways the statements care about, one with infinitely many units and one in which 2 splits into two different primes.

The results come in two groups, because cycles come in two kinds. A cycle that passes through a unit is **trivial**, like $1 \to 4 \to 2 \to 1$. Every other cycle, like the one through $-5$, misses the units. A map is a **Collatz map** if every nonzero starting number eventually falls into a trivial cycle.

For cycles through the units the gluing does most of the work. A map on the ring that respects the gluing is a Collatz map exactly when the map it induces on the glued classes sends every class to the class of 1. A map has at most as many trivial cycles as the ring has units, and if it commutes with multiplication by the units their number divides the number of units. Turning the multiplier by a unit, or reflecting the map, gives a family of maps that are Collatz maps together or not at all.

For cycles that miss the units the tool is older. Such a cycle forces a power of the multiplier to come very close to a power of the divisor, and how close it can come bounds the cycle's length from below. In the integers this condition has a size part and a sign part. In the Gaussian integers it has a size part and an angle part, and the two together give a much stronger bound than either alone. For Barina's map a cycle that misses the units needs at least 27,855,251 odd steps. For the map with multiplier $5 + 2i$ it needs at least 2190 odd steps, where the size part alone would give 169 and the angle part alone 161. *(To update after the verification run to norm $1.3 \cdot 10^{11}$.)* Neither map has a cycle of the simplest shape that misses the units, with no computer check needed.

# Part I. From N to Z

## 2. N as Z with x and −x identified

[Placeholder: gluing $x$ to $-x$; when a map on $\mathbb{Z}$ gives a map on $\mathbb{N}$; transfer on $\mathbb{Z}$.]

## 3. Two kinds of map

[Placeholder: $3x+1$ as the canonical Collatz-type map, induced by no residue-class map; $7x \pm 1$ as a Barina-type map inducing itself on $\mathbb{N}$; the window with credit to Matthews and Watts, Heppner and Leigh; drift; forced fixed points ($4 - 3 = 1$).]

## 4. Rotation on Z

[Placeholder: $7x \pm 1$ against $-7x \pm 1$, fixed points against the unit loop $1 \leftrightarrow -1$; mirroring does nothing on $\mathbb{Z}$.]

## 5. Cycles on Z

[Placeholder: trivial and unit cycles, at most two; $3x - 1$ with a unit cycle and other cycles; cycles that miss the units, size and sign conditions, the bound for $7x \pm 1$, averaging, no $1$-cycles; the profile of $7x \pm 1$.]

# Part II. The general statements

## 6. Maps over a ring of integers

[Placeholder: setting and definitions; Collatz-type and Barina-type maps; geometric and residue-class representatives; the definition of a Collatz map.]

## 7. The quotient R/U

[Placeholder: descent, transfer, respects associates iff commutes, lifting cycles.]

## 8. Trivial and unit cycles

[Placeholder: at most $|U|$, divisor form; the unit-cycle theorem; when a variant has a unit cycle; when a rotation has a single trivial cycle.]

## 9. Rotation and mirror

[Placeholder: families of up to $2|U|$ maps; what is the same on $R/U$ and what differs on $R$; automorphisms.]

## 10. Which maps exist

[Placeholder: the threshold $\kappa$; moduli that carry multipliers; the window per archimedean place; below the window; forced fixed points.]

## 11. Cycles that miss the units

[Placeholder: one condition per archimedean place; why the joint bound exceeds the single ones; averaging; $1$-cycles.]

# Part III. The general statements tested

## 12. The Gaussian integers

[Placeholder: $\kappa = 2$ and the first commuting maps at $(1+i)^3$; geometric against residue-class representatives, $2+i$ at norm $5$ and $5+2i$ at norm $29$, and what this says back about $3x+1$ and $7x \pm 1$; the $5+2i$ family and its trivial-cycle counts; trivial cycles divide $4$; the joint bound and $1$-cycles for $5+2i$; profiles of $5+2i$ and $2+i$.]

## 13. Infinitely many units: Z[√2]

[Placeholder: unit walks; the window in each embedding; transfer; residue-class maps send only finitely many units to units.]

## 14. A multiplier without an angle: the split ring

[Placeholder, kept on probation: in $\mathbb{Z}[(1+\sqrt{-7})/2]$ the angle condition decides the cycles of the multiplier $5$.]

## 15. Formal proofs

[Placeholder.]

## 16. Open questions

[Placeholder.]

## References

[Placeholder: written last, from a review of the finished text.]
