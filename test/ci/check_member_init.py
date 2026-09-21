#!/usr/bin/env python3
"""Every pointer or arithmetic data member under src/ has a default member
initializer.

Issue #466: `AsyncServer* _tcpListener;` in src/arduino/wifi.hpp was never
initialised. 2.1.0 added a read of it (`if (_tcpListener != nullptr)`) before
anything had assigned it, the check passed on whatever the stack held, and
the `delete` that followed crashed the node at start-up. The compiler cannot
warn about this: the class has no constructor, so there is no constructor to
leave the member out of, and the header only builds on the device, where
neither clang-tidy nor cppcheck runs in this repository's CI.

So the rule is checked as text, and it is deliberately stricter than "some
constructor sets it": every raw-pointer member and every member of a
fundamental arithmetic type must carry a default member initializer
(`= nullptr`, `= 0`, `= false`, ...). A default holds under every
constructor the class has, and every one it gains later, which is exactly
the property a constructor-based rule cannot give.

One consequence to know when adding a default: under gnu++11, which the
ESP32 Arduino 2.x core still builds with, a struct with a default member
initializer is no longer an aggregate, so `= {a, b, c}` and
`push_back({a, b})` stop compiling for it. Give such a struct a default
constructor and one taking its fields, as BridgeCoordinationState and
MeshChannelCandidate do; the desktop build is C++14 and will not tell you.

Not checked, on purpose, and worth knowing:

  * arrays (`char str[200];`) -- an uninitialised buffer written before it
    is read is the normal shape of a formatting scratch area;
  * enums and class types -- an enum's type cannot be told from a class
    name by regex, and class types construct themselves;
  * references -- a reference member cannot exist uninitialised.

The parser is a line scanner that tracks braces after stripping comments
and string literals: a data member is a declaration that begins at the
class body's own depth. A declaration deeper than that is a local variable
in a member function and is left alone. It is a heuristic, and the test
beside it (test_check_member_init.sh) pins down what it must and must not
report.

Usage: check_member_init.py [PATH ...]   (default: src)
Exit 0 when clean, 1 when any member is reported, 2 on a usage error.
"""

import pathlib
import re
import sys

ARITHMETIC = (
    r"(?:(?:unsigned|signed)\s+)?"
    r"(?:long\s+long|long\s+int|long|short|int|char|bool|float|double|"
    r"size_t|ssize_t|u?int(?:8|16|32|64|ptr)?_t|time_t)"
)
# Anything followed by `*` is a pointer, whatever it points at.
POINTER_TYPE = r"[A-Za-z_][\w:]*(?:\s*<[^;{}]*?>)?"

QUALIFIERS = r"(?:(?:mutable|volatile|const)\s+)*"
DECLARATOR = re.compile(
    r"^\s*(?P<stars>\*+\s*)?(?P<name>[A-Za-z_]\w*)\s*(?P<array>(?:\[[^\]]*\]\s*)+)?(?P<init>=|\{)?"
)
SKIP_WORDS = re.compile(
    r"\b(?:static|constexpr|typedef|using|friend|extern|return|operator|template|"
    r"virtual|explicit|inline|throw|delete|new|goto|case|default)\b"
)
CLASS_OPEN = re.compile(
    r"\b(?P<kind>class|struct|union)\s+(?:[A-Za-z_]\w*\s+)*?(?P<name>[A-Za-z_]\w*)\s*"
    r"(?:final\s*)?(?::[^{;]*)?\{"
)
CLASS_HEAD = re.compile(
    r"\b(?P<kind>class|struct|union)\s+(?:[A-Za-z_]\w*\s+)*?(?P<name>[A-Za-z_]\w*)\s*"
    r"(?:final\s*)?(?::[^{;]*)?$"
)


def strip_comments_and_strings(text):
    """Comments become spaces (newlines kept), string and char literals become
    empty quotes, so nothing inside them can look like a declaration or a
    brace."""
    out = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if text.startswith("//", i):
            j = text.find("\n", i)
            j = n if j == -1 else j
            out.append(" " * (j - i))
            i = j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n if j == -1 else j + 2
            out.append("".join("\n" if ch == "\n" else " " for ch in text[i:j]))
            i = j
        elif c in "\"'":
            quote = c
            j = i + 1
            while j < n and text[j] != quote:
                if text[j] == "\\":
                    j += 1
                if text[j] == "\n":
                    break
                j += 1
            out.append(quote + quote)
            i = j + 1
        else:
            out.append(c)
            i += 1
    return "".join(out)


class Finding:
    def __init__(self, path, line, cls, member, kind):
        self.path, self.line, self.cls, self.member, self.kind = path, line, cls, member, kind

    def __str__(self):
        return (f"{self.path}:{self.line}: {self.cls}::{self.member} ({self.kind}) "
                f"has no default member initializer")


def members_without_initializer(segment):
    """The (name, kind) pairs in one declaration statement that lack an
    initializer, or [] when the statement is not a data member declaration
    this checker cares about."""
    s = segment.strip()
    if not s or s.startswith("#") or "(" in s or SKIP_WORDS.search(s):
        return []
    # An access specifier shares the statement with the first member after it
    # (`protected:` has no `;` of its own).
    s = re.sub(r"^(?:public|protected|private)\s*:\s*", "", s)
    m = re.match(rf"^{QUALIFIERS}(?P<type>{ARITHMETIC})\b(?P<rest>.*)$", s) or \
        re.match(rf"^{QUALIFIERS}(?P<type>{POINTER_TYPE})\s*(?P<rest>\*.*)$", s)
    if not m:
        return []
    # A pointer to a class type is only a pointer if a `*` precedes the first
    # name; an arithmetic type may be pointer or value.
    rest = m.group("rest")
    is_pointer_type_match = m.group("type") and not re.match(rf"^{ARITHMETIC}$", m.group("type"))
    if is_pointer_type_match and not rest.lstrip().startswith("*"):
        return []
    found = []
    for declarator in split_declarators(rest):
        d = DECLARATOR.match(declarator)
        if not d or d.group("array") or d.group("init"):
            continue
        kind = "pointer" if d.group("stars") or is_pointer_type_match else "arithmetic"
        if is_pointer_type_match and not d.group("stars"):
            continue
        found.append((d.group("name"), kind))
    return found


def split_declarators(rest):
    """`a = 0, *b, c` -> ['a = 0', '*b', 'c'], not splitting inside brackets."""
    parts, depth, current = [], 0, []
    for ch in rest:
        if ch in "<([{":
            depth += 1
        elif ch in ">)]}":
            depth -= 1
        if ch == "," and depth == 0:
            parts.append("".join(current))
            current = []
        else:
            current.append(ch)
    parts.append("".join(current))
    return parts


def check_file(path):
    text = strip_comments_and_strings(path.read_text(encoding="utf-8", errors="replace"))
    findings = []
    depth = 0
    scopes = []          # (name, member_depth)
    pending = None       # a class head whose `{` is on a later line
    statement = []       # lines of a statement not yet ended by `;` at member depth
    statement_line = 0
    for lineno, raw in enumerate(text.split("\n"), 1):
        line = raw.rstrip()
        if line.lstrip().startswith("#"):
            continue
        depth_at_start = depth

        if pending and line.lstrip().startswith("{"):
            scopes.append((pending, depth_at_start + 1))
            pending = None
        opener = None if re.search(r"\benum\s+(?:class|struct)\b", line) else CLASS_OPEN.search(line)
        if opener:
            before = line[: opener.end() - 1]
            scopes.append((opener.group("name"), depth_at_start + before.count("{") - before.count("}") + 1))
        elif CLASS_HEAD.search(line) and not re.search(r"\benum\b|;|\btemplate\b|\bfriend\b", line):
            pending = CLASS_HEAD.search(line).group("name")

        if scopes and depth_at_start == scopes[-1][1] and not opener:
            # An access specifier on a line of its own is not the start of
            # the declaration that follows it: a finding is reported on the
            # declaration's own line.
            if re.match(r"^\s*(?:public|protected|private)\s*:\s*$", line):
                statement = []
                depth += line.count("{") - line.count("}")
                continue
            if not statement:
                statement_line = lineno
            statement.append(line)
            # A line that opens or closes a block at member depth is a
            # function body, a nested type or a braced default, never the
            # start of a declaration continued on the next line.
            if ";" not in line and ("{" in line or "}" in line):
                statement = []
            elif ";" in line:
                text_stmt = " ".join(statement)
                statement = []
                for seg in text_stmt.split(";"):
                    for name, kind in members_without_initializer(seg):
                        findings.append(Finding(path, statement_line, scopes[-1][0], name, kind))
                statement_line = 0
        else:
            statement = []

        depth += line.count("{") - line.count("}")
        while scopes and depth < scopes[-1][1]:
            scopes.pop()
    return findings


def main(argv):
    roots = [pathlib.Path(a) for a in argv[1:]] or [pathlib.Path("src")]
    files = []
    for root in roots:
        if root.is_file():
            files.append(root)
        elif root.is_dir():
            files.extend(sorted(p for p in root.rglob("*") if p.suffix in (".hpp", ".h", ".hxx")))
        else:
            print(f"check_member_init: no such path: {root}", file=sys.stderr)
            return 2
    findings = []
    for f in files:
        findings.extend(check_file(f))
    for finding in findings:
        print(finding)
    if findings:
        print(f"\n{len(findings)} member(s) without a default member initializer. "
              "Give each one a default (= nullptr, = 0, = false): it holds under every "
              "constructor the class has or gains later. See test/ci/check_member_init.py.")
        return 1
    print(f"check_member_init: {len(files)} header(s), every pointer and arithmetic member has a default")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
