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

Two lists: what to do, and what not to do. Rules marked *(draft)* await the author's approval. The draft rules come from the author's edits to the machine-level supplement (Nov 1, 2026) and from the survey in [`book/research/ai-writing-patterns.md`](book/research/ai-writing-patterns.md). One instance of a pattern is not a fault; a habit of it is.

### Do

1. **Standard terms.** Use the field's standard term; define the paper's own words where they are introduced.
2. **Proofs hold only proof** and use only earlier facts.
3. **LaTeX** uses `\text{}` only.
4. **One new idea at a time.** A passage finishes one idea before it starts the next. *(draft)*
5. **An idea comes just before its first use.** Introduced earlier, it is forgotten by the time it is needed; a short pause inside a section is better. *(draft)*
6. **Easy before hard.** Order follows difficulty. *(draft)*
7. **Familiar before abstract.** Start from what the reader already knows, such as a clock before modular arithmetic or decimal before binary. *(draft)*
8. **Like things, like treatment.** If one item of a kind gets prose and a formula, every item of that kind does. *(draft)*
9. **Reasons, not measurements.** Explain why a method is faster; readers can run the code to measure it. *(draft)*
10. **Answer the informed reader's question where it arises.** Where the text departs from what an expert expects, it says why at that point. *(draft)*
11. **Meaning over syntax.** Teach what an operation does rather than how a tool spells it. *(draft)*
12. **Plain words.** Prefer the plain verb ("is", "has", "uses"). *(draft)*
13. **Count before listing.** A list has as many items as there are. *(draft)*
14. **Paragraphs by default.** A list is for separable, parallel items such as the steps of an algorithm. *(draft)*
15. **End on content.** A section ends on its last point. *(draft)*
16. **Hedge only real uncertainty,** and say what the uncertainty is. *(draft)*
17. **Cut any sentence a reader would not miss.** *(draft)*

### Don't

1. **No references, benchmarks or prior art** in the paper.
2. **No brand names without need.** Name a language, product or chip only where the point needs it. *(draft)*
3. **Don't call anything hard.** *(draft)*
4. **No reflexive three.** Never three items because three sounds complete. *(draft)*
5. **No inflated words,** such as crucial, pivotal, delve, underscore, showcase, intricate and meticulous. The list drifts and is reviewed now and then. *(draft)*
6. **Dashes rarely.** No dash where a comma, parentheses, a colon or a full stop will do. *(draft)*
7. **No "not X but Y"** unless X is something readers actually believe. *(draft)*
8. **No trailing "-ing" comments.** A sentence does not end with a clause that interprets it (", ensuring...", ", highlighting..."); a reason gets its own sentence or is cut. *(draft)*
9. **No bold except where a term is defined.** *(draft)*
10. **No recap** of the section just read, and **no offer** of more. *(draft)*
11. **No labelled verdict at the start** of a passage ("Short answer:", "Yes, with one caveat"); a condition belongs inside the sentence it limits. *(draft)*
12. **No drumrolls.** No "Here's the thing", and no rhetorical question answered in the next fragment. A colon introduces a definition, list, equation or consequence, not a punchline. *(draft)*

Open conflicts: Don't 1 (no references) and the supplement's citation of published instruction costs; Do 3 (`\text{}` only) and one use of `\big(` in the supplement.
