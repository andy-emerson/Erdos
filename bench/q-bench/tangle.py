#!/usr/bin/env python3
"""Tangle noweb-style Markdown into runnable Python (and C).

Chunk syntax inside the Markdown sources:

    ```python {file=build/foo.py}     a root chunk, written to that path
    ```python {#chunk name}            a named chunk, referenced elsewhere

A line consisting of optional indentation followed by <<chunk name>> is
replaced by that chunk's body, re-indented to match. Chunks that share a
name are concatenated in document order. Undefined references, cycles and
unused named chunks are errors.
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
SOURCES = sorted((ROOT / "lit").glob("*.md"))

FENCE_OPEN = re.compile(r"^```(?:python|c)\s*\{(file=(?P<file>[^}\s]+)|#(?P<name>[^}]+))\}\s*$")
FENCE_CLOSE = re.compile(r"^```\s*$")
REFERENCE = re.compile(r"^(?P<indent>[ \t]*)<<(?P<name>[^>]+)>>\s*$")


def parse(paths):
    named, roots = {}, {}
    for path in paths:
        lines = path.read_text().splitlines()
        i = 0
        while i < len(lines):
            m = FENCE_OPEN.match(lines[i])
            if not m:
                i += 1
                continue
            start = i + 1
            i += 1
            while i < len(lines) and not FENCE_CLOSE.match(lines[i]):
                i += 1
            if i == len(lines):
                sys.exit(f"{path.name}:{start}: unterminated chunk")
            body = lines[start:i]
            if m.group("file"):
                roots.setdefault(m.group("file"), []).extend(body)
            else:
                named.setdefault(m.group("name").strip(), []).extend(body)
            i += 1
    return named, roots


def expand(lines, named, used, stack):
    out = []
    for line in lines:
        m = REFERENCE.match(line)
        if not m:
            out.append(line)
            continue
        name, indent = m.group("name").strip(), m.group("indent")
        if name not in named:
            sys.exit(f"undefined chunk <<{name}>> (referenced from {' -> '.join(stack)})")
        if name in stack:
            sys.exit(f"cyclic chunk reference: {' -> '.join(stack + [name])}")
        used.add(name)
        for inner in expand(named[name], named, used, stack + [name]):
            out.append(indent + inner if inner.strip() else "")
    return out


def main():
    named, roots = parse(SOURCES)
    if not roots:
        sys.exit("no root chunks found")
    used = set()
    for target, body in roots.items():
        text = "\n".join(expand(body, named, used, [target])).rstrip() + "\n"
        path = ROOT / target
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)
        print(f"wrote {target}")
    unused = sorted(set(named) - used)
    if unused:
        sys.exit(f"unused chunks: {', '.join(unused)}")


if __name__ == "__main__":
    main()
