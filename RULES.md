# Rules

The rules the work follows, in three lists. A rule holds until it is discussed; a lesson learned holds where it was tested.

**The rule for this file:** only the author writes or changes it. Claude may recommend a rule, but never adds or changes one without the author's say.

## Algorithm design

1. **Exact.** Arithmetic is exact, never approximated. The project's point is to make exact arithmetic fast, so a departure from exactness is out of scope.
2. **General before special.** Start from the textbook solution, with no special cases. Then name each special case, a condition inside an operation under which a faster path applies, as an opportunity to optimize. Each special case gets its own row and its own explanation.
3. **Prefer solutions shared across operations.** Operations do different jobs and will have some different optimizations, but they share much. Where one solution works for several, prefer it to letting them drift apart.
4. **One structure each.** Each object (a rational, a matrix) is allowed one stored structure, each operation one temporary structure, and each function one local structure. None is required; more than one needs discussion first. Structures buy speed at the cost of memory, and this rule caps the loss.
   - *Stored:* part of an object, kept as long as the object is (e.g. a matrix's packed list).
   - *Temporary:* lives across one operation and is freed at its end (e.g. the build hash table).
   - *Local:* created and freed inside one function, never returned or stored (e.g. mul's decoded row).
5. **Self-contained.** No outside libraries.

### Lessons learned (not rules)

- **Lowest terms** saves memory, and sometimes computation, but it is an optimization technique, not a principle: the decimal type keeps $10^k$ denominators and is not in lowest terms.
- **Fixed size beats growing** for the matrix build table. Elsewhere growing may win.
- **Immutable stored values** have paid off in many places, but changing a value in place is not ruled out where it is better.

## Optimization approach

1. **Schoolbook first, then measure.** Build the textbook version, measure where the cost goes, and optimize the measured weak points, one at a time.
2. **Solve the problem in front of us.** A problem outside the current question is exposed, not quietly solved: overflow is reported as an error when overflow isn't what's being studied. Don't solve problems that don't exist yet.
3. **Match the control.** An optimized model gives the same values as the control, value for value.
4. **Timed runs alone.** Timed runs need a quiet machine, with all other work paused.

6. **Portable pseudocode.** *(Draft, Nov 1, 2026: agreed in discussion; wording awaits the author's approval.)* Machine-level pseudocode uses only steps that x86-64, ARM64 and RISC-V all provide in their common current form. A step one family lacks, such as the carry flag or the 128-by-64 divide, is not used.

## Reviews

1. **A spec is reviewed against the algorithm design rules** above, before anything is built.
2. **A build is reviewed against its spec**: it does what was specified, and only that.

## Prose

1. **Standard terms.** Use the field's standard term; define the paper's own words where they are introduced.
2. **No references, benchmarks or prior art** in the paper.
3. **Proofs hold only proof** and never use a later fact.
4. **LaTeX** uses `\text{}` only.

### Draft prose rules (Nov 1, 2026; for the author's approval)

Drawn from the author's edits to the machine-level supplement. Rules on AI writing patterns will be added from the survey in [`paper/research/ai-writing-patterns.md`](paper/research/ai-writing-patterns.md).

5. **One new idea at a time.** A passage finishes one idea before it starts the next.
6. **An idea comes just before its first use.** Introduced earlier, it is forgotten by the time it is needed; a short pause inside a section is better.
7. **Easy before hard.** Order follows difficulty, and the text never calls anything hard.
8. **Familiar before abstract.** Start from what the reader already knows, such as a clock before modular arithmetic or decimal before binary.
9. **Like things, like treatment.** If one item of a kind gets prose and a formula, every item of that kind does.
10. **Reasons, not measurements.** Explain why a method is faster; readers can run the code to measure it.
11. **Answer the informed reader's question where it arises.** Where the text departs from what an expert expects, it says why at that point.
12. **Meaning over syntax; general over brand names.** Teach what an operation does rather than how a tool spells it, and name a language, product or chip only where the point needs it.

Open conflicts with the rules above: rule 2 (no references) and the supplement's citation of published instruction costs; rule 4 (`\text{}` only) and one use of `\big(` in the supplement.

