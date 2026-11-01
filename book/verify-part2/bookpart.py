"""The Part 2 portion of the book: the paper's checks read only the text after the "# Part 2" divider."""


def part2_text(path):
    text = path.read_text(encoding="utf-8")
    marker = "\n# Part 2. The library\n"
    return text.split(marker, 1)[1] if marker in text else text
