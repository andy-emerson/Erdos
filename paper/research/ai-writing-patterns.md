# Recognizable patterns of AI-generated prose

Survey compiled 1 November 2026.

## Introduction

This note collects the habits of style that readers, editors and researchers now associate with text written by large language models. It is meant as raw material for a set of prose rules for a book on exact arithmetic, so it concentrates on patterns a careful human writer could fall into by accident, and says little about artifacts that only appear when chatbot output is pasted unedited (leftover citation tags, placeholder brackets, tracking parameters in links).

Sources were chosen in this order of preference. First, quantitative studies: corpus work on word-frequency shifts in scientific abstracts and peer reviews, a grammatical comparison of human and model text published in PNAS, and alignment research on why models flatter and pad. Second, the Wikipedia editors' guide "Signs of AI writing", maintained by WikiProject AI Cleanup, which is the most detailed field guide available and which other outlets now cite as the reference. Third, statements by model makers that acknowledge a habit, such as Anthropic's published system prompts and OpenAI's comments on em dashes, sycophancy and "contrastive phrasing". Fourth, journalism and practitioner catalogues, used mainly to confirm that a pattern is widely noticed and to name ones the studies do not cover. Most material dates from 2024 to 2026. Some pages (the Wikipedia guide, the tropes.fyi list) are living documents, so what they say may have changed since they were read.

Two cautions apply throughout. Nearly every pattern here also occurs in competent human writing; the guides agree that one instance proves nothing and that density and clustering are what give text away. And the patterns drift: the Wikipedia guide notes that "delve" has largely faded while newer models favour other words, so a list of banned words will age. Rules about structure and habit are likely to last longer than word lists.

Each pattern is marked with how well it is attested:

- strong: at least one quantitative study, or several independent serious sources including a model maker's own acknowledgment;
- moderate: documented in a curated guide such as Wikipedia's and confirmed by at least one other independent source, but not measured;
- anecdotal: practitioner observation or a single catalogue, or (in one case) the author's own observation with no independent source found.

## 1. Vocabulary

### 1.1 Overused "AI vocabulary" (strong)

A small set of words, mostly verbs and adjectives, appears far more often in model output than in human writing of the same kind.

Examples: "This chapter delves into the intricate interplay of rounding and representation." "The result underscores the crucial role of exact arithmetic."

Kobak and colleagues measured word frequencies in over 15 million PubMed abstracts and found that 2024 had far more "excess" words than any earlier year, including the Covid years. The Covid excess was almost entirely nouns about content; the 2024 excess was mostly style words, about two thirds of them verbs. The sharpest jumps were "delves" (about 28 times its expected frequency), "underscores" (about 14 times) and "showcasing" (about 11 times), with "potential", "findings", "crucial", "additionally", "comprehensive", "enhancing", "notably" and "insights" also on the list. They estimate at least 13.5% of 2024 abstracts were processed by a model. Juzek and Ward isolated 21 "focal words" overused by ChatGPT in abstracts (delve, showcase, boast, underscore, intricate, realm, groundbreaking, garnered, aligns, surpass and others) and argue that preference tuning is the likeliest cause, though their evidence for that is mixed. Liang and colleagues found the same effect in peer reviews for machine-learning conferences: "meticulous" became about 35 times more likely to appear in a review sentence, "intricate" about 11 times, "commendable" about 10 times. The PNAS study by Reinhart and colleagues found ChatGPT versions using "tapestry" and "camaraderie" around 150 times as often as human writers. Wikipedia's guide gives a longer list and a rough timeline by model generation: "delve" for GPT-4 era text, "align with", "fostering" and "highlighting" later, and words such as "leverage", "multifaceted" and "prioritize" in 2026.

Why it reads as AI: the words are not wrong individually; they are improbably dense. Wikipedia calls a cluster of them "one of the strongest tells". Russell, Karpinska and Iyyer found that people who use ChatGPT often can spot model text very accurately, and that such "AI vocabulary" was one of the main cues they relied on.

What a human writer does instead: picks the plain word that says what happened ("examines", "shows", "has"), and uses a word like "crucial" only when something would actually fail without the thing described.

Sources: Kobak et al. 2025; Juzek and Ward 2025; Liang et al. 2024; Reinhart et al. 2025; Wikipedia, Signs of AI writing; Russell et al. 2025.

### 1.2 Inflated significance and legacy (strong)

Ordinary facts are presented as milestones, testaments or turning points.

Examples: "Gauss's method stands as a testament to the enduring power of modular thinking." "This marked a pivotal moment in the evolving landscape of numerical computing."

Wikipedia lists this first among its content signs, with stock phrases such as "stands as", "is a testament to", "pivotal moment", "evolving landscape", "indelible mark" and "setting the stage for". The guide explains it as the statistical pull toward generic, important-sounding language, reinforced by training that rewards broadly pleasing answers. The tropes.fyi catalogue lists the same thing as "grandiose stakes inflation". The word-frequency studies in 1.1 measure the vocabulary that carries this habit.

What a human writer does instead: states the fact and lets the reader judge its weight; if the significance matters, gives the concrete consequence ("after this, error bounds could be checked by machine").

Sources: Wikipedia, Signs of AI writing; tropes.fyi; Kobak et al. 2025.

### 1.3 Promotional and evaluative adjectives (moderate)

Neutral description drifts into brochure language.

Examples: "a rich and vibrant body of work", "this elegant, groundbreaking algorithm".

Wikipedia notes that models drift toward travel-guide or press-release prose even when told to be encyclopedic, and that newer models do it more subtly than GPT-4 did. "Groundbreaking", "commendable" and "innovative" appear in the frequency studies above.

What a human writer does instead: describes what the algorithm does and how fast; leaves praise to the reader or to a cited authority.

Sources: Wikipedia, Signs of AI writing; Juzek and Ward 2025; Liang et al. 2024.

### 1.4 Avoiding "is" (moderate)

Simple copulas are replaced by "serves as", "stands as", "functions as", "represents" or "boasts".

Examples: "The continued fraction serves as a compact representation of the ratio." "The library boasts support for arbitrary precision."

Wikipedia lists this under language signs and reports a study finding a drop of over 10% in "is" and "are" in academic writing in 2023. tropes.fyi calls it the "serves as" dodge.

What a human writer does instead: writes "is" and "has".

Sources: Wikipedia, Signs of AI writing; tropes.fyi.

### 1.5 Connective filler and empty intensifiers (moderate)

Sentences open with "Additionally", "Notably", "Importantly", "Interestingly" or "It's worth noting that", which announce a point instead of making it.

Examples: "Notably, the algorithm terminates." "It's worth noting that the denominator can grow quickly."

"Additionally" (especially at the start of a sentence) heads Wikipedia's vocabulary list; "additionally" and "notably" are among Kobak's excess style words; tropes.fyi and aismells both list "it's worth noting" as filler. A related habit is the "magic adverb" ("quietly", "fundamentally") attached to make an ordinary claim sound deep.

What a human writer does instead: deletes the connective. If the point is worth noting, stating it is enough.

Sources: Wikipedia, Signs of AI writing; Kobak et al. 2025; tropes.fyi; aismells.

### 1.6 Elegant variation (moderate)

The writer keeps renaming the same thing to avoid repeating a word.

Example: a paragraph that calls one object "the fraction", then "the rational quantity", then "this ratio", then "the quotient".

Wikipedia attributes this to the repetition penalties used when sampling text, which discourage reusing tokens. In technical writing it is worse than a stylistic flaw, because a reader reasonably assumes a new term names a new thing.

What a human writer does instead: picks one name and keeps it.

Sources: Wikipedia, Signs of AI writing.

### 1.7 Vague attribution and vague connection (moderate)

Claims are credited to unnamed "experts", "observers" or "several sources", and relations are blurred into "associated with" or "in connection with".

Examples: "Experts have long argued that floating point is unsuitable here." "He was associated with the development of the standard."

Wikipedia lists both under content signs and notes that models inflate one or two sources into a consensus. tropes.fyi lists vague attribution too. In a technical book this pattern overlaps with plain inaccuracy.

What a human writer does instead: names the person, paper and year, or drops the claim; states the actual role ("he chaired the committee").

Sources: Wikipedia, Signs of AI writing; tropes.fyi.

### 1.8 Noun-heavy, nominalized style (strong)

Actions are turned into nouns, producing dense, abstract sentences.

Example: "The utilization of exact representation enables the elimination of accumulation of rounding error" instead of "Exact representation means rounding errors cannot accumulate."

Reinhart and colleagues (PNAS, 2025) compared instruction-tuned models with human writers across genres and found nominalizations at 1.5 to 2 times the human rate, and concluded that the models are trained toward an "informationally dense, noun-heavy" style that they do not adjust to context.

What a human writer does instead: puts the action in a verb and the actor in the subject.

Sources: Reinhart et al. 2025.

## 2. Sentence-level patterns

### 2.1 Em dashes used for everything (strong as a perceived tell; frequency not formally measured)

Em dashes stand in for commas, parentheses, colons and full stops, often several to a paragraph and often to "punch up" a clause.

Examples: "The proof is short — almost suspiciously so." "Exact arithmetic has a cost — memory — and that cost grows."

Wikipedia says models use em dashes more than non-professional human writers in the same genre, often where a human would use a comma, parenthesis or colon, and in a formulaic, sales-copy way; it also says the dash is a tell only alongside other signs. Sean Goedecke's analysis (October 2025) reports that GPT-4o used roughly ten times as many as GPT-3.5 and that models from other labs share the habit, and argues, while admitting it is speculative, that the cause is training on digitized older books. In November 2025 Sam Altman announced that ChatGPT would "finally" obey a custom instruction not to use em dashes, which is an acknowledgment from the model maker that the habit was strong enough to resist instructions. Many writers have objected that the em dash is a legitimate mark used heavily by good human writers; that is true, and is the reason the rule should be about function, not about the character itself. I found no peer-reviewed study measuring em dash rates in model output.

What a human writer does instead: uses commas for light asides, parentheses for true digressions, a colon before an explanation or list, and a new sentence when the thought is new. A dash is kept for the rare break that none of those can carry.

Sources: Wikipedia, Signs of AI writing; Goedecke 2025; AOL/Business Insider on Altman 2025; Plagiarism Today 2025.

### 2.2 The rule of three by reflex (moderate)

Adjectives, phrases and examples come in threes regardless of how many there really are.

Examples: "fast, reliable and elegant"; "The library offers exact results, predictable performance and a clean interface."

Wikipedia lists this as a language sign and says triads make shallow analysis look comprehensive. tropes.fyi lists "tricolon abuse", especially triads stacked back to back. I found no quantitative study of triad frequency in model output; the attestation rests on curated guides and wide practitioner agreement.

What a human writer does instead: lists the items that exist. Two is often the honest number; four is fine. One precise adjective usually beats three loose ones.

Sources: Wikipedia, Signs of AI writing; tropes.fyi.

### 2.3 "It's not X, it's Y" (negative parallelism) (strong)

A claim is introduced by denying something nobody asserted, then supplying the "real" answer.

Examples: "This isn't a bug, it's a design decision." "Exactness isn't a luxury. It's a requirement."

Wikipedia lists "not only ... but ..." and "it's not ..., it's ..." as stereotypical signs that "clear up" misconceptions nobody raised. Will Oremus's Atlantic article (July 2026) calls it perhaps the best-known tic of AI writing, reports an estimate from the detection company Pangram that such sentences are about three times as common in AI writing as in human writing, cites Barron's finding that the construction more than quadrupled in corporate communications between 2023 and 2025, and quotes an OpenAI model-behavior manager conceding that ChatGPT "turns to it too often". Explanations offered include rater preference (it "gives the impression of nuance and insight") and simple next-word probability.

What a human writer does instead: states Y directly. The contrast is kept only when a reader really would assume X, and then X is attributed ("A common belief is that ...").

Sources: Wikipedia, Signs of AI writing; Oremus, The Atlantic, 2026; tropes.fyi.

### 2.4 The countdown: "No X. No Y. Just Z." (moderate)

Options are dismissed in a row of fragments before the payoff.

Example: "No rounding. No approximation. Just the exact answer."

Wikipedia lists "no ..., no ..., just ..." with negative parallelism; the Atlantic piece discusses it as a variant; tropes.fyi and aismells list it as the "countdown".

What a human writer does instead: writes one sentence that says what the method gives.

Sources: Wikipedia, Signs of AI writing; Oremus 2026; tropes.fyi; aismells.

### 2.5 Trailing "-ing" clauses that assert significance (strong)

A factual sentence ends with a participial phrase that interprets it: "highlighting ...", "underscoring ...", "reflecting ...", "ensuring ...".

Example: "The algorithm returns a reduced fraction, ensuring consistency across all subsequent operations."

Reinhart and colleagues found present participial clauses at 2 to 5 times the human rate in instruction-tuned model output, the largest grammatical difference they measured. Wikipedia lists the same construction as "superficial analysis" and notes the attached claim is usually unsupported.

What a human writer does instead: either drops the clause or makes its claim a separate sentence with its reason ("Because every result is reduced, equality can be tested by comparing numerators and denominators").

Sources: Reinhart et al. 2025; Wikipedia, Signs of AI writing.

### 2.6 False ranges (moderate)

"From X to Y" phrasing that implies a spectrum between unrelated things.

Example: "from the ancient Babylonians to modern cryptography".

Wikipedia lists it and traces it to persuasive writing in training data; tropes.fyi lists it too. Genuine ranges ("from 2 to 64 bits") are fine.

What a human writer does instead: names the specific cases and says what connects them.

Sources: Wikipedia, Signs of AI writing; tropes.fyi.

### 2.7 Colon and question reveals (moderate)

A short set-up ends in a colon or a rhetorical question, then the "answer" arrives as a fragment.

Examples: "The result? A fraction with a 400-digit denominator." "The problem is simple: precision."

tropes.fyi lists "The X? A Y." as a trope, and aismells lists the "self-posed question" ("The takeaway? Simple."). This is the same suspense device as 4.5 compressed into one sentence.

What a human writer does instead: writes the statement as a statement. A colon is for what genuinely follows from the clause before it, such as a definition, a list or an equation.

Sources: tropes.fyi; aismells.

### 2.8 Symmetric parallelism and anaphora (anecdotal)

Consecutive sentences or clauses share the same opening or the same shape, for rhythm rather than meaning.

Example: "It is exact. It is predictable. It is portable."

tropes.fyi lists "anaphora abuse" and back-to-back tricolons. Wikipedia's negative-parallelism section covers part of this. No measurement found.

What a human writer does instead: uses parallel form only for things that really are parallel, such as cases of a definition, and otherwise lets sentence shapes follow the content.

Sources: tropes.fyi; Wikipedia, Signs of AI writing.

### 2.9 Uniform rhythm, or manufactured punch (moderate)

Two opposite rhythm problems are both reported. One is monotony: sentences of similar length and structure, paragraph after paragraph. The other is the dramatic one-line paragraph or fragment inserted for emphasis.

Examples: four consecutive sentences each about 18 words with a subordinate clause up front; a paragraph that consists only of "And that changes everything."

GPTZero's early detector was built on "burstiness", the observation that human writing varies more through a document (GPTZero says it no longer relies on this measure). The Wikipedia guide's notes on 2026 models mention sentences mostly of 8 to 20 words. tropes.fyi lists "short punchy fragments". An Atlantic essay (May 2026) argues that a uniform, polished register across very different situations is itself a tell.

What a human writer does instead: lets length follow content, with long sentences for careful qualification and short ones for plain facts, and saves one-line paragraphs for the rare point that needs them.

Sources: GPTZero support article; Wikipedia, Signs of AI writing; tropes.fyi; The Atlantic, "The Biggest Tell That Something Was Written by AI", 2026.

## 3. Structure and formatting

### 3.1 Headers and bullets everywhere (strong)

Short answers and continuous arguments are broken into headed sections and bullet lists.

Example: a two-paragraph explanation of why 0.1 cannot be represented in binary, delivered as a header, five bullets and a "Key takeaway".

Anthropic's own published system prompts for Claude (2025 and January 2026 versions) tell the model to avoid "over-formatting responses with elements like bold emphasis, headers, lists, and bullet points", and to write reports and explanations as prose. That instruction exists because the default behaviour is the opposite. Wikipedia lists Markdown formatting among the most common signs, and user complaints about over-formatted ChatGPT answers are widespread. tropes.fyi adds "listicle in a trench coat", where prose is really a numbered list ("The first ... The second ...").

What a human writer does instead: writes connected paragraphs where the ideas depend on each other, and keeps lists for genuinely parallel, separable items such as steps of an algorithm.

Sources: Anthropic system prompts; Wikipedia, Signs of AI writing; tropes.fyi; aiproductivity.ai.

### 3.2 Bolded lead-ins on list items (strong)

Every bullet starts with a bold phrase and a colon, then a sentence restating it.

Example: "**Precision:** Exact arithmetic provides full precision."

Wikipedia calls these "inline-header vertical lists" and separately flags mechanical boldface in a "key takeaways" style. tropes.fyi lists "bold-first bullets". Anthropic's system prompt forbids "excessive bolded text" in prose.

What a human writer does instead: if a list is needed, lets each item be a plain sentence; uses bold rarely, for a term at the point where it is defined.

Sources: Wikipedia, Signs of AI writing; tropes.fyi; Anthropic system prompts.

### 3.3 Title Case Headings and decorative symbols (moderate)

Headings capitalize every major word; headings or bullets carry emoji, arrows or other Unicode decoration.

Examples: "Applications In Modern Cryptography"; "→ Why it matters".

Wikipedia lists title case, emoji and curly quotes, noting that curly quotes are a weak signal on their own. tropes.fyi lists arrows and similar "Unicode decoration". These matter less for a typeset book, where the house style decides.

What a human writer does instead: follows one heading style throughout (sentence case is common in technical books) and uses no decoration.

Sources: Wikipedia, Signs of AI writing; tropes.fyi.

### 3.4 Closing summaries and fractal summaries (strong)

Sections end by restating what they just said, often introduced by "In summary", "In conclusion" or "Overall"; in long pieces, summaries appear at every level.

Example: a three-paragraph section ending "In summary, exact arithmetic trades speed for correctness, which makes it valuable in many contexts."

Wikipedia lists summaries and conclusions as a sign. tropes.fyi lists both "the signposted conclusion" and "fractal summaries", and also "one-point dilution", a single idea restated many ways. Research on preference tuning gives a plausible mechanism: Singhal and colleagues found that much of the measured gain from RLHF can be reproduced by rewarding length alone, so padding is rewarded.

What a human writer does instead: ends a section when its point is made. A summary is written only where a reader needs to carry results forward, and it then adds structure (which results, in which order) instead of repeating sentences.

Sources: Wikipedia, Signs of AI writing; tropes.fyi; Singhal et al. 2023.

### 3.5 The "challenges and future outlook" formula (moderate)

A piece ends with a section acknowledging difficulties, dismissing them, and closing on optimism.

Example: "Despite these challenges, exact arithmetic remains a promising area with exciting future directions."

Wikipedia describes this formula ("Despite its ... faces several challenges", "Future Outlook") and stresses that the formula, not the mention of difficulties, is the problem. tropes.fyi lists it as "Despite its challenges...".

What a human writer does instead: discusses each limitation where it arises, with specifics, and ends on the last real point.

Sources: Wikipedia, Signs of AI writing; tropes.fyi.

### 3.6 Signposting and tour-guide phrases (moderate)

The text announces what it is about to do.

Examples: "Let's break this down." "Let's dive into the details." "In this section, we will explore ..."

tropes.fyi lists "Let's break this down" and "Let's unpack"; aismells lists the "tour guide" voice. Wikipedia's chatbot-correspondence signs ("here is a ...", "more detailed breakdown") overlap. Some signposting is normal in a long technical book; the tell is a signpost at every turn.

What a human writer does instead: starts doing the thing. Signposts are kept for long chapters where the reader needs a map.

Sources: tropes.fyi; aismells; Wikipedia, Signs of AI writing.

### 3.7 Padding and length (strong)

Responses are longer than the content requires: restated questions, preambles, recaps, optional extras.

Singhal and colleagues showed that reward models used in RLHF are strongly biased toward length. The Wikipedia guide and tropes.fyi describe the visible results (restatement, summaries, one-point dilution).

What a human writer does instead: cuts any sentence that a reader would not miss.

Sources: Singhal et al. 2023; Wikipedia, Signs of AI writing; tropes.fyi.

## 4. Stance, tone and conversational habits

### 4.1 Flattery and agreement (strong)

Replies open by praising the question or agreeing with the reader.

Examples: "Great question!" "You're absolutely right."

Sharma and colleagues (Anthropic; ICLR 2024) showed sycophancy across five assistants and traced it partly to human preference data, in which agreeing with the user was among the most predictive features of a preferred answer. In April 2025 OpenAI rolled back a GPT-4o update because it had become "overly supportive but disingenuous", saying it had "focused too much on short-term feedback". Anthropic's Claude 4 system prompts (May 2025) instruct the model never to open by calling a question good, great or fascinating, and to skip "the flattery". Wikipedia lists "You're absolutely right!" among chatbot phrases.

What a human writer does instead: answers. In a book this shows up as praise of the reader ("as the careful reader will have noticed"), which works only when it is earned and rare.

Sources: Sharma et al. 2024; Engadget on OpenAI 2025; ISMG on Claude 4 system prompts 2025; Wikipedia, Signs of AI writing.

### 4.2 Chatbot pleasantries and closing offers (strong)

The text ends with offers or well-wishes addressed to a user: "I hope this helps", "Let me know if you'd like ...", "Would you like me to ...", "If you want, I can also explain:".

Wikipedia lists these under "collaborative communication". TechRadar's AI editor described ChatGPT ending almost every reply with "If you want, I can also explain:" after the 5.3 and 5.4 releases (March 2026), and the phrases are common in other catalogues. They are unlikely in a book manuscript, but the habit of ending with an offer of more has a literary cousin: closing a chapter with a list of further topics the text never needed.

What a human writer does instead: ends when the content ends.

Sources: Wikipedia, Signs of AI writing; TechRadar 2026.

### 4.3 The stock verdict opener (anecdotal)

Every answer starts with the same frame: a verdict word plus one qualifier, or a labelled short answer.

Examples: "Yes, with one caveat." "No, because of one requirement." "Short answer: it depends on the denominator."

This is the author's own observation. I searched for a dedicated write-up and found none that documents this exact frame, so it is marked anecdotal. It is closely related to better-attested habits: the formulaic opener that Anthropic's prompt suppresses (4.1), the false-suspense reveal (2.7, 4.5), and the uniform register across contexts (2.9). The tell is less the phrase than its repetition: the same opening shape on every reply or every section. In a book, the equivalent is every section opening with a one-line verdict.

What a human writer does instead: starts with whatever the reader needs first, which is sometimes the answer, sometimes the definition, sometimes the example, and varies accordingly. A verdict that needs a caveat gets the caveat in the same sentence, without announcing that there is "one".

Sources: author's observation; related: Anthropic system prompts; tropes.fyi.

### 4.4 Stacked hedges and disclaimers (moderate)

Claims are qualified repeatedly until no claim remains: "it's important to note", "this may potentially", "one could argue", "based on available information", "while specific details are limited".

Example: "In some cases, this approach may potentially offer certain advantages, depending on the context."

Wikipedia lists knowledge-cutoff and "details are limited" disclaimers, and says they often precede unsupported speculation. A practitioner essay by Valentin Monteiro (DEV Community) catalogues hedging phrases and attributes them to training that rewards safe answers; tropes.fyi lists "false vulnerability" as a polished, risk-free admission. I found no corpus study measuring hedge density in model prose, so this is moderate.

What a human writer does instead: states the claim with its actual condition ("this is faster when denominators stay below 64 bits") and hedges only where there is real uncertainty, saying what the uncertainty is.

Sources: Wikipedia, Signs of AI writing; Monteiro, DEV Community; tropes.fyi.

### 4.5 False suspense: "Here's the thing" (moderate)

A drumroll phrase introduces an ordinary point.

Examples: "Here's the thing: floating point is an approximation." "Here's where it gets interesting."

tropes.fyi lists "Here's the kicker" and "Here's the thing"; aismells lists the "false drumroll". It shares a mechanism with the colon reveal (2.7).

What a human writer does instead: states the point; if it is surprising, the surprise will show.

Sources: tropes.fyi; aismells.

### 4.6 False balance and returning the decision (moderate)

Every question gets "both sides" and a closing "it depends", even when the evidence favours one answer.

Example: "Rational arithmetic and floating point each have strengths and weaknesses; the right choice depends on your use case."

Monteiro's essay describes models listing the strengths of each option and handing the decision back. Wikipedia's "Despite these challenges" formula and its note on presenting one or two views as widely held are related. Anthropic's published prompt shows the behaviour is partly deliberate in some contexts (it asks the model to present opposing perspectives on contested topics), which explains why it spreads to questions that are not contested.

What a human writer does instead: gives the recommendation and the condition under which it would change.

Sources: Monteiro, DEV Community; Wikipedia, Signs of AI writing; Anthropic system prompts.

### 4.7 Uniform, over-polished register (moderate)

The prose has the same smooth, composed tone whatever the subject or situation.

An Atlantic essay (May 2026) argues that writing that is too polished for its circumstances, and identical in register across very different ones, is the most reliable tell. Reinhart and colleagues found that instruction-tuned models do not shift style across genres the way humans do. Russell and colleagues report that expert readers judged formality and originality, not only vocabulary.

What a human writer does instead: lets the register follow the material: brisker in a worked example, slower in a subtle proof, personal where a personal remark helps.

Sources: The Atlantic 2026; Reinhart et al. 2025; Russell et al. 2025.

### 4.8 Invented labels, stock analogies and stacked precedents (anecdotal)

Several smaller habits appear in practitioner catalogues but not in studies: coining an official-sounding name for an ordinary idea ("the precision trap"); the patronizing "Think of it as ..." analogy that explains less than the original; a rapid list of historical precedents to borrow authority; one metaphor repeated until it is worn out.

What a human writer does instead: uses an established term or a plain description; uses an analogy only where it carries real structure; cites a precedent when it is evidence.

Sources: tropes.fyi.

## 5. Candidate rules

These turn the patterns into rules the author might adopt. They are options, not decisions, and some will not suit a mathematical book (a definition-by-cases is legitimately parallel; a proof outline is legitimately a list).

Vocabulary

- Prefer the plain verb. Use "is" and "has" rather than "serves as" and "boasts".
- Keep a short list of banned words (for example delve, underscore, showcase, tapestry, landscape, testament, pivotal, crucial, intricate, meticulous, realm, foster, leverage, multifaceted) and review it every year or so, since the list drifts.
- Do not call anything pivotal, crucial or groundbreaking; show the consequence instead.
- Delete sentence-initial "Additionally", "Notably", "Importantly", "Interestingly" and "It's worth noting that".
- Name one thing with one term throughout a passage.
- Attribute every claim to a named source or drop it.
- Put actions in verbs, not in nouns ending in -tion or -ment.

Sentences

- Use a dash only where a comma, parentheses, a colon or a full stop will not do. Consider a cap per page or per chapter.
- Count before listing. Give two items or four if that is how many there are; do not reach for three.
- Do not deny a claim nobody made. Write "Y", not "not X but Y", unless X is a belief readers actually hold and is attributed.
- No "No X. No Y. Just Z."
- Do not end a sentence with "-ing" clauses that interpret it ("highlighting", "ensuring", "reflecting"). Give the reason in its own sentence or cut it.
- Write "from X to Y" only for a real scale.
- No rhetorical question answered in the next fragment. A colon introduces a definition, list, equation or consequence, not a punchline.
- Use parallel sentence shapes only for parallel content.
- Let sentence length follow content. Keep one-sentence paragraphs rare.

Structure

- Default to paragraphs. Use a list only for separable, parallel items such as algorithm steps.
- Bold a term only where it is defined. No bold lead-ins on list items.
- One heading style, applied consistently, with no decoration.
- No section-ending recap of the section just read. Summaries only where results must be carried forward, and then in a form that adds structure.
- No "challenges and outlook" ending. Deal with limitations where they arise.
- Signpost only when the reader needs a map of a long chapter; otherwise begin.
- Cut any sentence a reader would not miss.

Stance

- Never praise the reader or the question.
- End sections and chapters on content, not on offers of more.
- Vary how sections begin. Do not open with a labelled verdict ("Short answer:", "Yes, with one caveat"); if there is a caveat, put it in the sentence.
- State claims with their real conditions. Hedge only real uncertainty, and say what it is.
- No drumroll phrases ("Here's the thing", "Here's where it gets interesting").
- When one option is better, say so and say when it would stop being better.
- Let the register fit the passage.
- Use established terms rather than coined labels; use analogies only when they carry structure.

## Sources

Studies and papers

- [Delving into LLM-assisted writing in biomedical publications through excess vocabulary](https://arxiv.org/abs/2406.07016v5), Kobak, González-Márquez, Horvát, Lause. arXiv June 2024, v5 July 2025; published in Science Advances, 2025 ([full text v3](https://arxiv.org/html/2406.07016v3)).
- [Why Does ChatGPT "Delve" So Much? Exploring the Sources of Lexical Overrepresentation in Large Language Models](https://arxiv.org/html/2412.11385v1), Juzek and Ward. arXiv December 2024; COLING 2025 ([ACL Anthology](https://preview.aclanthology.org/setup/2025.coling-main.426)).
- [Monitoring AI-Modified Content at Scale: A Case Study on the Impact of ChatGPT on AI Conference Peer Reviews](https://arxiv.org/html/2403.07183), Liang et al. arXiv March 2024 (later revisions).
- [Do LLMs write like humans? Variation in grammatical and rhetorical styles](https://arxiv.org/html/2410.16107v2), Reinhart et al. PNAS, 2025; summary in [CMU news, "Is It Human, or Is It AI?"](https://www.cmu.edu/dietrich/news/news-stories/2025/large-language-models-writing-text), 2025.
- [People who frequently use ChatGPT for writing tasks are accurate and robust detectors of AI-generated text](https://arxiv.org/abs/2501.15654), Russell, Karpinska, Iyyer. arXiv January 2025; ACL 2025 ([ACL Anthology](https://aclanthology.org/2025.acl-long.267)).
- [Towards Understanding Sycophancy in Language Models](https://arxiv.org/html/2310.13548v4), Sharma et al. arXiv October 2023, revised May 2025; ICLR 2024 ([proceedings](https://proceedings.iclr.cc/paper_files/paper/2024/hash/0105f7972202c1d4fb817da9f21a9663-Abstract-Conference.html)).
- [A Long Way to Go: Investigating Length Correlations in RLHF](https://arxiv.org/abs/2310.03716v2), Singhal, Goyal, Xu, Durrett. arXiv October 2023, v2 July 2024.
- [Antislop: A Comprehensive Framework for Identifying and Eliminating Repetitive Patterns in Language Models](https://arxiv.org/abs/2510.15061), Paech et al. arXiv October 2025; ICLR 2026 poster. Reports some patterns over 1,000 times more frequent in model output than in human text; consulted for background only.

Editorial guides and model-maker documents

- Wikipedia: Signs of AI writing, WikiProject AI Cleanup. Living document, read November 2026 through mirrors ([reader mirror](https://hn.jakechampion.name/reader/https://en.wikipedia.org/wiki/Wikipedia:WikiProject_AI_Cleanup/AI_catchphrases), [Wikiless mirror](https://wikiless.lunar.icu/wiki/Wikipedia:AIWTW?lang=en)); canonical page at en.wikipedia.org/wiki/Wikipedia:Signs_of_AI_writing.
- [The best guide to spotting AI writing comes from Wikipedia](https://techcrunch.com/2025/11/20/the-best-guide-to-spotting-ai-writing-comes-from-wikipedia/), TechCrunch, 20 November 2025.
- [Claude Sonnet 4.5 system prompts](https://platform.claude.com/docs/de/release-notes/system-prompts/claude-sonnet-4-5.md), Anthropic, versions of 29 September 2025, 19 November 2025 and 18 January 2026 (formatting instructions).
- [A Peek Behind the Claude Curtain](https://www.govinfosecurity.com/peek-behind-claude-curtain-a-28530), ISMG, 28 May 2025 (quotes the Claude 4 instruction to skip flattery).
- [How do I interpret burstiness or perplexity?](https://support.gptzero.me/hc/en-us/articles/15130070230551-How-do-I-interpret-burstiness-or-perplexity), GPTZero support, undated.

Journalism

- [The Most Famous AI Writing Tic Is Also the Most Mysterious](https://tekelili.duckdns.org/the-most-famous-ai-writing-tic-is-also-the-most-mysterious.html), Will Oremus, The Atlantic, July 2026 (republished copy); summary at [AI Weekly](https://aiweekly.co/alerts/atlantic-ais-its-not-x-its-y-tic-proves-hardest-to-shake).
- [The Biggest Tell That Something Was Written by AI](https://aitopics.org/doc/journals:7BEF9DED), The Atlantic, 29 May 2026 (index entry).
- [Sam Altman says OpenAI has a fix for a telltale sign that you used ChatGPT](https://www.aol.com/news/sam-altman-says-openai-fix-133235171.html), AOL / Business Insider, November 2025.
- [OpenAI rolls back update that made ChatGPT an ass-kissing weirdo](https://www.engadget.com/ai/openai-rolls-back-update-that-made-chatgpt-an-ass-kissing-weirdo-203056185.html), Engadget, 29 to 30 April 2025.
- [I got tired of ChatGPT trying to bait me with follow-up questions after every answer](https://www.techradar.com/ai-platforms-assistants/chatgpt/i-got-tired-of-chatgpt-trying-to-bait-me-with-follow-up-questions-after-every-answer-so-i-made-this-one-easy-change), Graham Barlow, TechRadar, 13 March 2026.
- [Em Dashes, Hyphens and Spotting AI Writing](https://www.plagiarismtoday.com/2025/06/26/em-dashes-hyphens-and-spotting-ai-writing/), Plagiarism Today, 26 June 2025.
- [ChatGPT over-formatted responses and user frustration](https://aiproductivity.ai/news/chatgpt-over-formatted-responses-user-frustration/), aiproductivity.ai, undated (contains a dating error about the GPT-4o rollback; used only as evidence of complaints).

Practitioner write-ups

- [Why do AI models use so many em-dashes?](https://seangoedecke.com/em-dashes/), Sean Goedecke, 30 October 2025.
- [AI Writing Tropes to Avoid (tropes.md)](https://gist.github.com/ossa-ma/f3baa9d25154c33095e22272c631f5a1), ossama.is, last active September 2026; entry on [negative parallelism](https://tropes.fyi/tropes/negative-parallelism) at tropes.fyi.
- [Common ChatGPT phrases](https://aismells.pages.dev/common-chatgpt-phrases.html), aismells, undated.
- [AI Writes Like a Scared Intern](https://dev.to/valentin_monteiro/ai-writes-like-a-scared-intern-the-involuntary-ai-watermark-2-2llf), Valentin Monteiro, DEV Community, undated.
