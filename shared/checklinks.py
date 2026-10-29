"""Check that every relative Markdown link, and its #anchor, resolves, in every .md file of the repository that git
tracks or would track (ignored files are skipped). Links to web pages (http:, mailto: and so on) are not checked.
Anchors follow GitHub's heading slugs.

Usage: python3 shared/checklinks.py [REPO] [EXCLUDED_TOP_DIR ...]   (exit status 1 if a link is broken)
Example: python3 shared/checklinks.py . 09-interning
"""
import re
import subprocess
import sys
from pathlib import Path

repo = Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
excl = set(sys.argv[2:])
files = subprocess.run(["git", "ls-files", "-co", "--exclude-standard", "*.md"], cwd=repo,
                       capture_output=True, text=True, check=True).stdout.split()
files = [repo / f for f in files if f.split("/")[0] not in excl and (repo / f).exists()]

LINK = re.compile(r"(?<!\!)\[(?:[^\[\]]|\[[^\]]*\])*\]\(\s*<?([^)\s>]+)>?(?:\s+\"[^\"]*\")?\s*\)")
IMG = re.compile(r"!\[[^\]]*\]\(\s*([^)\s]+)\s*\)")

def slug(h):
    h = h.strip().lower()
    h = re.sub(r"[^\w\- ]", "", h)          # GitHub: drop punctuation, keep word chars, - and space
    return h.replace(" ", "-")

anchors_cache = {}
def anchors(p):
    if p not in anchors_cache:
        seen, out = {}, set()
        inside = False
        for line in p.read_text(encoding="utf-8").splitlines():
            if line.lstrip().startswith("```"):
                inside = not inside; continue
            m = re.match(r"^(#{1,6})\s+(.*?)\s*#*\s*$", line)
            if m and not inside:
                s = slug(m.group(2))
                n = seen.get(s, 0); seen[s] = n + 1
                out.add(s if n == 0 else f"{s}-{n}")
            for a in re.findall(r'<a\s+(?:name|id)="([^"]+)"', line):
                out.add(a)
        anchors_cache[p] = out
    return anchors_cache[p]

bad = 0; total = 0
for f in files:
    text = f.read_text(encoding="utf-8")
    text_nocode = re.sub(r"```.*?```", "", text, flags=re.S)
    text_nocode = re.sub(r"`[^`\n]*`", lambda m: "" if "](" not in m.group(0) else m.group(0), text_nocode)
    for m in list(LINK.finditer(text_nocode)) + list(IMG.finditer(text_nocode)):
        url = m.group(1)
        if re.match(r"^[a-z][a-z0-9+.-]*:", url, re.I):
            continue                             # http:, mailto:, etc.
        total += 1
        path, _, frag = url.partition("#")
        target = (f.parent / path).resolve() if path else f
        problem = None
        if not target.exists():
            problem = "missing file"
        elif frag and target.suffix == ".md" and frag not in anchors(target):
            problem = f"missing anchor #{frag}"
        if problem:
            bad += 1
            print(f"{f.relative_to(repo)}: {url}: {problem}")
print(f"{total} links checked in {len(files)} files; {bad} broken")
sys.exit(1 if bad else 0)
