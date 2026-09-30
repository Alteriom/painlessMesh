"""Rebuild .github/icm/icm.source.json and each pipeline's stage table from the stage contracts.

Usage, from anywhere in the repository: python3 .github/icm/_system/generate.py [--check]
(--check changes nothing and exits 1 if either output is out of date.)

Each stage's inputs, outputs and human check are read from its contract, so the contract
is the only place they are written by hand. A fact file under _shared/ names the commit
it was verified at in its frontmatter; the repository files it cites (a bare workflow file
name is looked up in .github/workflows/) become its evidence.
"""

import json
import re
import sys
from datetime import date, timedelta
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
ICM = ".github/icm"
SOURCE = f"{ICM}/icm.source.json"
REPOSITORY = "Alteriom/painlessMesh"
PROJECT = "painlessmesh"
REVIEW_DAYS = 90
ENTRY, POINTER = "CLAUDE.md", "AGENTS.md"
TABLE_HEADER = "| Stage | Job | Input | Output | Human check |\n|---|---|---|---|---|\n"

CODE = re.compile(r"`([^`\n]+)`")
PATHLIKE = re.compile(r"^[\w.-]+(/[\w.-]+)*/?$")
LINK = re.compile(r"\]\(\s*<?([^)\s>]+)>?\s*\)")
INPUT = re.compile(r"^- (Working|Reference) \(([^)]*)\): (.*)$")
OUTPUT = re.compile(r"^- (\S+) → output/")
FRONT = re.compile(r"\A---\n(.*?)\n---\n", re.S)


def fail(message):
    sys.exit(f"generate.py: {message}")


def read(rel):
    return (ROOT / rel).read_text(encoding="utf-8")


def exists(rel):
    return bool(PATHLIKE.match(rel)) and (ROOT / rel.rstrip("/")).exists()


def section(text, name, where):
    match = re.search(rf"^## {name}\n(.*?)(?=^## |\Z)", text, re.M | re.S)
    if not match:
        fail(f"{where} has no '## {name}' section")
    return match.group(1).strip()


def frontmatter(text):
    match = FRONT.match(text)
    if not match:
        return {}
    return dict(line.split(": ", 1) for line in match.group(1).splitlines() if ": " in line)


def node_id(path):
    rel = path[len(ICM) + 1 :] if path.startswith(ICM + "/") else path
    return re.sub(r"[^a-zA-Z0-9._-]", "-", rel).strip("-._")[:120]


def parse_contract(pipeline, folder):
    contract = f"{pipeline}/stages/{folder}/CONTEXT.md"
    text = read(contract)
    title = text.splitlines()[0]
    if " — " not in title:
        fail(f"{contract}: the title is '# {folder} — <the job>'")
    stage = {"contract": contract, "job": title.split(" — ", 1)[1].strip(),
             "working": [], "labels": [], "reference": [], "optional": [], "do_not_load": []}
    for line in section(text, "Inputs", contract).splitlines():
        match = INPUT.match(line)
        if match:
            kind, when, value = match.groups()
            path = CODE.match(value).group(1) if value.startswith("`") else None
            if kind == "Working":
                if path:
                    stage["working"].append(path)
                    stage["labels"].append(path)
                else:
                    stage["labels"].append(value.split(" — ", 1)[0].strip())
                continue
            if not path or not exists(path):
                fail(f"{contract}: reference input {value!r} is not a file in the repository")
            stage["optional" if when.startswith("only") else "reference"].append(path)
        elif line.startswith("Do NOT load:"):
            stage["do_not_load"] = [p for p in CODE.findall(line) if exists(p)]
    outputs = [m.group(1) for m in map(OUTPUT.match, section(text, "Outputs", contract).splitlines()) if m]
    if len(outputs) != 1:
        fail(f"{contract}: '## Outputs' names {len(outputs)} files; a stage writes one")
    stage["output"] = f"{pipeline}/stages/{folder}/output/{outputs[0]}"
    check = " ".join(section(text, "Human check", contract).split())
    if not 1 <= len(check) <= 500:
        fail(f"{contract}: the human check is {len(check)} characters (1-500)")
    stage["human_check"] = check
    return stage


def input_cell(pipeline, stage):
    cells = []
    for label in stage["labels"]:
        prefix = f"{pipeline}/stages/"
        if label.startswith(prefix) and "/output/" in label:
            folder, name = label[len(prefix) :].split("/output/", 1)
            cells.append(f"{folder}'s `{name}`")
        else:
            cells.append(label)
    return " and ".join(cells)


def build():
    pipelines = sorted(
        f"{ICM}/{p.name}" for p in (ROOT / ICM).iterdir()
        if p.is_dir() and re.match(r"^\d\d-[a-z-]+$", p.name) and (p / "stages").is_dir()
    )
    outputs, stages, contracts, tables, in_place = {}, {}, set(), {}, set()
    for pipeline in pipelines:
        prefix = pipeline.rsplit("/", 1)[1].split("-", 1)[1]
        folders = sorted(p.name for p in (ROOT / pipeline / "stages").iterdir()
                         if p.is_dir() and re.match(r"^\d\d_[a-z-]+$", p.name))
        rows = []
        for order, folder in enumerate(folders, 1):
            stage = parse_contract(pipeline, folder)
            contracts.add(stage["contract"])
            others = [f"{p}/" for p in pipelines if p != pipeline]
            in_place.update(p for p in stage["reference"] + stage["optional"] if not p.startswith(ICM + "/"))
            entry = {
                "order": order,
                "contract": stage["contract"],
                "inputs": {"working": stage["working"], "reference": stage["reference"]},
                "do_not_load": others + [p for p in stage["do_not_load"] if p not in others],
                "outputs": [stage["output"]],
                "human_check": stage["human_check"],
                "required": [node_id(stage["contract"])],
            }
            if stage["optional"]:
                entry["optional"] = [node_id(p) for p in stage["optional"]]
            stages[f"{prefix}-{folder.split('_', 1)[1]}"] = entry
            rows.append(f"| [`{folder}`](stages/{folder}/CONTEXT.md) | {stage['job']} "
                        f"| {input_cell(pipeline, stage)} | `output/{stage['output'].rsplit('/', 1)[1]}` "
                        f"| {stage['human_check']} |")
        path = f"{pipeline}/CONTEXT.md"
        text = read(path)
        if TABLE_HEADER not in text:
            fail(f"{path} has no stage table header")
        start = text.index(TABLE_HEADER) + len(TABLE_HEADER)
        end = text.find("\n\n", start)
        tables[path] = text[:start] + "\n".join(rows) + (text[end:] if end != -1 else "\n")

    # Repository documents named in place: the entry's links, and stage references outside .github/icm.
    for target in LINK.findall(read(ENTRY)):
        if "://" not in target and not target.startswith("#"):
            target = target.split("#", 1)[0]
            if not target.startswith(ICM) and target not in (ENTRY, POINTER) and (ROOT / target).is_file():
                in_place.add(target)
    files = [ENTRY, POINTER, *sorted(in_place)]
    files += sorted(
        p.relative_to(ROOT).as_posix() for p in (ROOT / ICM).rglob("*.md")
        if "_system" not in p.parts and "output" not in p.parts
    )

    facts = {}
    for path in files:
        meta = frontmatter(read(path))
        if "verified_at" in meta:
            facts[path] = meta
    if not facts:
        fail("no fact file carries verified_at")
    newest = max(date.fromisoformat(m["verified_at"]) for m in facts.values())

    nodes = []
    for path in files:
        name = path.rsplit("/", 1)[-1]
        if path in (ENTRY, POINTER):
            kind, layer = "route", "L0"
        elif path in contracts:
            kind, layer = "stage", "L2"
        elif path.startswith(ICM + "/") and name in (ENTRY, "CONTEXT.md"):
            kind, layer = "route", "L1"
        else:
            kind, layer = "fact", "L3"
        meta = facts.get(path, {})
        verified = date.fromisoformat(meta["verified_at"]) if meta else newest
        node = {
            "id": node_id(path),
            "revision": 1,
            "kind": kind,
            "layer": layer,
            "state": "accepted",
            "project_id": PROJECT,
            "verified_at": verified.isoformat(),
            "review_after": (verified + timedelta(days=REVIEW_DAYS)).isoformat(),
            "files": [path],
        }
        if meta.get("verified_commit"):
            text = read(path)
            base = str(Path(path).parent)
            cited = set()
            for span in CODE.findall(text):
                token = span.split()[0].removeprefix("./") if span.strip() else ""
                for candidate in (token, f".github/workflows/{token}"):
                    if PATHLIKE.match(candidate) and (ROOT / candidate).is_file():
                        if not candidate.startswith(ICM):
                            cited.add(candidate)
                        break
            for link in LINK.findall(text):
                resolved = (ROOT / base / link.split("#", 1)[0]).resolve()
                if resolved.is_file() and ROOT in resolved.parents:
                    rel = resolved.relative_to(ROOT).as_posix()
                    if not rel.startswith(ICM):
                        cited.add(rel)
            if not cited:
                fail(f"{path} names verified_commit but cites no repository file")
            node["evidence"] = [{"repo": REPOSITORY, "path": p, "commit": meta["verified_commit"]}
                                for p in sorted(cited)[:50]]
        nodes.append(node)

    source = {
        "workspace_root": "",
        "schema_version": 2,
        "access": "workspace",
        "entrypoint": ENTRY,
        "runtimes": ["claude_code", "codex_cli"],
        "projects": [PROJECT],
        "nodes": nodes,
        "forms": [{"form": "umbrella", "root": "", "pipelines": pipelines}]
        + [{"form": "pipeline", "root": p} for p in pipelines],
        "stages": stages,
    }
    written = {SOURCE: json.dumps(source, indent=2, ensure_ascii=False) + "\n", **tables}
    return written


def main():
    check = "--check" in sys.argv[1:]
    stale = []
    for path, text in build().items():
        current = (ROOT / path).read_text(encoding="utf-8") if (ROOT / path).exists() else None
        if current != text:
            stale.append(path)
            if not check:
                (ROOT / path).write_text(text, encoding="utf-8")
    if check and stale:
        fail("out of date: " + ", ".join(stale) + " (run without --check)")
    print(("up to date" if not stale else "rewrote " + ", ".join(stale)) if not check else "up to date")


if __name__ == "__main__":
    main()
