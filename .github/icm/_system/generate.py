#!/usr/bin/env python3
"""The ICM standard, as one script every repository vendors unchanged.

Vendored at ``<home>/_system/generate.py`` (``<home>`` is ``.github/icm`` or ``icm``),
configured by ``<home>/_system/config.json`` beside it, and run from anywhere in the
repository:

    python3 .github/icm/_system/generate.py            # rewrite the generated files
    python3 .github/icm/_system/generate.py --check    # exit 1 if they are out of date or the lint fails
    python3 .github/icm/_system/generate.py lint       # the structural checks alone, any form
    python3 .github/icm/_system/generate.py --check --strict   # also fail on facts past review

Generated: ``<home>/icm.source.json`` and each pipeline's ``CONTEXT.md`` stage table. Each
stage's inputs, outputs and human check are read from its contract, so the contract is
the only place they are written by hand. A fact file names the commit it was verified at
in its frontmatter (``verified_at``, ``verified_commit``); the repository files it cites in
backticks (a bare workflow file name is looked up in ``.github/workflows/``; a ``path:line``
suffix is ignored) or links become its evidence. A node without frontmatter keeps the
dates, revision and evidence it already has in the source file, so adopting this script
loses nothing. Keys it does not derive and the Git sync resolves (``repositories``, ``links``,
``workspaces``) and ``packages`` are carried over from the existing source file unchanged.
``relationships`` and ``inclusions`` belong to knowledge (service) workspaces, declared by
a person in the dashboard; the Git sync does not resolve them, so the lint refuses them.

The lint is what the service's compiler enforces, run before the pull request: entry
under 120 lines, twins byte-equal or one a one-line pointer, every fact dated and cited,
no placeholder left, no secret-looking string, stage contracts parseable. Keep this file
byte-identical to ``app/icm/standard/generate.py`` in memoryrelay/api; the repository's
particulars live in ``config.json``. Python 3.9+, standard library only.
"""

import fnmatch
import json
import re
import subprocess
import sys
from datetime import date, datetime, timedelta, timezone
from pathlib import Path

STANDARD_VERSION = "1.1.0"

HERE = Path(__file__).resolve()
HOME_DIR = HERE.parents[1]  # <root>/<home>: .github/icm or icm


def _root() -> Path:
    for p in HERE.parents:
        if (p / ".git").exists():
            return p
    return HOME_DIR.parents[1] if HOME_DIR.parent.name == ".github" else HOME_DIR.parent


ROOT = _root()
ICM = HOME_DIR.relative_to(ROOT).as_posix()
SOURCE = f"{ICM}/icm.source.json"
CONFIG = HERE.parent / "config.json"

TABLE_HEADER = "| Stage | Job | Input | Output | Human check |\n|---|---|---|---|---|\n"
MAX_ENTRY_LINES, ENTRY_TARGET_LINES = 120, 60
MAX_EVIDENCE = 50  # the compiler's cap on evidence entries per node

CODE = re.compile(r"`([^`\n]+)`")
PATHLIKE = re.compile(r"^[\w.-]+(/[\w.-]+)*/?$")
LINE_SUFFIX = re.compile(r":\d+(-\d+)?$")
LINK = re.compile(r"\]\(\s*<?([^)\s>]+)>?\s*\)")
INPUT = re.compile(r"^- (Working|Reference) \(([^)]*)\): (.*)$")
OUTPUT = re.compile(r"^- (\S+) → output/")
FRONT = re.compile(r"\A---\n(.*?)\n---\n", re.DOTALL)
PLACEHOLDER = re.compile(r"\{…\}|\{\{[^}]*\}\}|\{TBD\}|\{TODO[^}]*\}")
SECRETS = re.compile(
    r"(mem_(?:prod|live|test|dev)_[A-Za-z0-9]{12,}|nr_live_[A-Za-z0-9]{12,}|sk_live_[A-Za-z0-9]{12,}"
    r"|ghp_[A-Za-z0-9]{30,}|github_pat_[A-Za-z0-9_]{30,}|AKIA[0-9A-Z]{16}"
    r"|-----BEGIN [A-Z ]*PRIVATE KEY-----"
    r"|(?i:(?:api[_-]?key|secret|password|token)\s*[:=]\s*['\"][A-Za-z0-9+/_\-]{16,}['\"])"
    r"|postgres(?:ql)?(?:\+\w+)?://[^:\s/]+:[^@\s]+@)"
)

DEFAULTS = {
    "repository": None,
    "project": None,
    "entry": "CLAUDE.md",
    "pointer": "AGENTS.md",
    "review_days": 90,
    "runtimes": ["claude_code", "codex_cli"],
    "sub_entries": [],
    "instruction_files": [],
    "documents_index": "_shared/documents.md",
    "decisions": [],
    "document_id_prefix": "doc-",
    "forms": "umbrella",
}


class Problem(Exception):
    pass


def fail(message):
    sys.exit(f"generate.py: {message}")


def load_config():
    if not CONFIG.is_file():
        fail(
            f"{CONFIG.relative_to(ROOT).as_posix()} is missing; it names the repository and the project"
        )
    cfg = dict(DEFAULTS)
    cfg.update(json.loads(CONFIG.read_text(encoding="utf-8")))
    for key in ("repository", "project"):
        if not cfg.get(key):
            fail(f"config.json needs {key!r}")
    return cfg


def read(rel):
    return (ROOT / rel).read_text(encoding="utf-8")


def exists(rel):
    return bool(PATHLIKE.match(rel)) and (ROOT / rel.rstrip("/")).exists()


def section(text, name, where):
    match = re.search(rf"^## {name}\n(.*?)(?=^## |\Z)", text, re.MULTILINE | re.DOTALL)
    if not match:
        raise Problem(f"{where} has no '## {name}' section")
    return match.group(1).strip()


def frontmatter(text):
    match = FRONT.match(text)
    if not match:
        return {}
    return dict(line.split(": ", 1) for line in match.group(1).splitlines() if ": " in line)


def node_id(path, cfg):
    if path.startswith(ICM + "/"):
        rel = path[len(ICM) + 1 :]
    elif path in (cfg["entry"], cfg["pointer"]) or path in cfg["sub_entries"]:
        rel = path
    else:
        rel = cfg["document_id_prefix"] + path
    return re.sub(r"[^a-zA-Z0-9._-]", "-", rel).strip("-._")[:120]


def linked_files(path, cfg):
    """Repository files a Markdown file links, relative to the root."""
    base = str(Path(path).parent)
    found = set()
    for link in LINK.findall(read(path)):
        if "://" in link or link.startswith("#"):
            continue
        resolved = (ROOT / base / link.split("#", 1)[0]).resolve()
        if resolved.is_file() and ROOT in resolved.parents:
            rel = resolved.relative_to(ROOT).as_posix()
            if not rel.startswith(ICM) and rel not in (cfg["entry"], cfg["pointer"]):
                found.add(rel)
    return found


def cited_files(path, cfg):
    """Repository paths a fact file cites, outside the home: (files, directories).

    Backticked paths (a bare workflow file name is looked up in .github/workflows/, a
    ``path:line`` suffix is ignored) and Markdown links count."""
    files, dirs = set(), set()
    for span in CODE.findall(read(path)):
        token = span.split()[0].removeprefix("./") if span.strip() else ""
        token = LINE_SUFFIX.sub("", token)
        for candidate in (token, f".github/workflows/{token}"):
            if not PATHLIKE.match(candidate) or candidate.startswith(ICM) or candidate == ".git":
                continue
            if candidate in (cfg["entry"], cfg["pointer"]) or candidate in cfg["sub_entries"]:
                continue  # the entry routes; it owns no fact
            target = ROOT / candidate.rstrip("/")
            if target.is_file():
                files.add(candidate)
                break
            if target.is_dir() and "/" in candidate.rstrip("/"):
                dirs.add(candidate.rstrip("/") + "/")
                break
    files |= linked_files(path, cfg)
    return files, dirs


def without_code(text):
    """``text`` with fenced blocks and code spans removed, for prose-only checks."""
    text = re.sub(r"```.*?```", "", text, flags=re.DOTALL)
    return CODE.sub("", text)


# --- stage contracts --------------------------------------------------------------------


def parse_contract(pipeline, folder):
    contract = f"{pipeline}/stages/{folder}/CONTEXT.md"
    text = read(contract)
    title = text.splitlines()[0]
    if " — " not in title:
        raise Problem(f"{contract}: the title is '# {folder} — <the job>'")
    stage = {
        "contract": contract,
        "job": title.split(" — ", 1)[1].strip(),
        "working": [],
        "labels": [],
        "reference": [],
        "optional": [],
        "do_not_load": [],
    }
    for line in section(text, "Inputs", contract).splitlines():
        match = INPUT.match(line)
        if match:
            kind, when, value = match.groups()
            path = CODE.match(value).group(1) if value.startswith("`") else None
            if kind == "Working":
                # A working input is an earlier stage's output; anything else named here
                # (a repository file, a report) is the row's label, not a pinned input.
                if path and "/output/" in path:
                    stage["working"].append(path)
                    stage["labels"].append(path)
                else:
                    stage["labels"].append(value.split(" — ", 1)[0].strip())
                continue
            if not path or not exists(path):
                raise Problem(
                    f"{contract}: reference input {value!r} is not a file in the repository"
                )
            stage["optional" if when.startswith("only") else "reference"].append(path)
        elif line.startswith("Do NOT load:"):
            stage["do_not_load"] = [p for p in CODE.findall(line) if exists(p)]
    outputs = [
        m.group(1) for m in map(OUTPUT.match, section(text, "Outputs", contract).splitlines()) if m
    ]
    if len(outputs) != 1:
        raise Problem(f"{contract}: '## Outputs' names {len(outputs)} files; a stage writes one")
    stage["output"] = f"{pipeline}/stages/{folder}/output/{outputs[0]}"
    check = " ".join(section(text, "Human check", contract).split())
    if not 1 <= len(check) <= 500:
        raise Problem(f"{contract}: the human check is {len(check)} characters (1-500)")
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


def pipelines_of():
    return sorted(
        f"{ICM}/{p.name}"
        for p in HOME_DIR.iterdir()
        if p.is_dir() and re.match(r"^\d\d-[a-z-]+$", p.name) and (p / "stages").is_dir()
    )


# --- build -----------------------------------------------------------------------------


# Keys of the source file this script does not derive: written in a reviewed pull request
# and resolved by the Git sync (or, for packages, read as written). Carried over unchanged.
CARRIED_KEYS = ("repositories", "links", "workspaces", "packages")
# Keys only a knowledge (service) workspace carries, declared by a person in the dashboard.
# A Git sync does not resolve them and the compile fails on them, so the lint refuses them.
SERVICE_ONLY_KEYS = ("relationships", "inclusions")


def previous_source():
    if not (ROOT / SOURCE).is_file():
        return {}
    try:
        data = json.loads(read(SOURCE))
    except ValueError:
        return {}
    return data if isinstance(data, dict) else {}


def existing_nodes():
    data = previous_source()
    return {n["id"]: n for n in data.get("nodes", []) if isinstance(n, dict) and "id" in n}


def build(cfg):
    pipelines = pipelines_of()
    stages, contracts, tables, in_place = {}, set(), {}, set()
    for pipeline in pipelines:
        prefix = pipeline.rsplit("/", 1)[1].split("-", 1)[1]
        folders = sorted(
            p.name
            for p in (ROOT / pipeline / "stages").iterdir()
            if p.is_dir() and re.match(r"^\d\d_[a-z-]+$", p.name)
        )
        rows = []
        for order, folder in enumerate(folders, 1):
            stage = parse_contract(pipeline, folder)
            contracts.add(stage["contract"])
            others = [f"{p}/" for p in pipelines if p != pipeline]
            in_place.update(
                p for p in stage["reference"] + stage["optional"] if not p.startswith(ICM + "/")
            )
            entry = {
                "order": order,
                "contract": stage["contract"],
                "inputs": {"working": stage["working"], "reference": stage["reference"]},
                "do_not_load": others + [p for p in stage["do_not_load"] if p not in others],
                "outputs": [stage["output"]],
                "human_check": stage["human_check"],
                "required": [node_id(stage["contract"], cfg)],
            }
            if stage["optional"]:
                entry["optional"] = [node_id(p, cfg) for p in stage["optional"]]
            stages[f"{prefix}-{folder.split('_', 1)[1]}"] = entry
            rows.append(
                f"| [`{folder}`](stages/{folder}/CONTEXT.md) | {stage['job']} "
                f"| {input_cell(pipeline, stage)} | `output/{stage['output'].rsplit('/', 1)[1]}` "
                f"| {stage['human_check']} |"
            )
        path = f"{pipeline}/CONTEXT.md"
        text = read(path)
        if TABLE_HEADER not in text:
            raise Problem(f"{path} has no stage table header")
        start = text.index(TABLE_HEADER) + len(TABLE_HEADER)
        end = text.find("\n\n", start)
        tables[path] = text[:start] + "\n".join(rows) + (text[end:] if end != -1 else "\n")

    # Repository documents named in place: the entry's links, the documents index's links,
    # stage references outside the home, and the other agents' instruction files.
    in_place |= linked_files(cfg["entry"], cfg)
    index = f"{ICM}/{cfg['documents_index']}" if cfg["documents_index"] else None
    if index and (ROOT / index).is_file():
        in_place |= linked_files(index, cfg)
    for pattern in cfg["instruction_files"]:
        matches = [p for p in ROOT.glob(pattern) if p.is_file()]
        if not matches:
            raise Problem(f"instruction file pattern {pattern!r} matches nothing")
        in_place.update(p.relative_to(ROOT).as_posix() for p in matches)
    for sub in cfg["sub_entries"]:
        if not (ROOT / sub).is_file():
            raise Problem(f"sub_entries names {sub!r}, which is not a file")
    in_place -= set(cfg["sub_entries"])
    files = [cfg["entry"]]
    if (ROOT / cfg["pointer"]).is_file():
        files.append(cfg["pointer"])
    files += [*cfg["sub_entries"], *sorted(in_place)]
    files += sorted(
        p.relative_to(ROOT).as_posix()
        for p in HOME_DIR.rglob("*.md")
        if "_system" not in p.parts and "output" not in p.parts
    )

    facts = {}
    for path in files:
        meta = frontmatter(read(path))
        if "verified_at" in meta:
            facts[path] = meta
    if not facts:
        raise Problem("no fact file carries verified_at")
    newest = max(date.fromisoformat(m["verified_at"]) for m in facts.values())
    previous = existing_nodes()

    nodes = []
    for path in files:
        name = path.rsplit("/", 1)[-1]
        if path in (cfg["entry"], cfg["pointer"]):
            kind, layer = "route", "L0"
        elif path in cfg["sub_entries"]:
            kind, layer = "route", "L1"
        elif path in contracts:
            kind, layer = "stage", "L2"
        elif path.startswith(ICM + "/") and name in ("CLAUDE.md", "AGENTS.md", "CONTEXT.md"):
            kind, layer = "route", "L1"
        elif any(fnmatch.fnmatch(path, g) for g in cfg["decisions"]):
            kind, layer = "decision", "L3"
        else:
            kind, layer = "fact", "L3"
        nid = node_id(path, cfg)
        old = previous.get(nid, {})
        meta = facts.get(path, {})
        if meta:
            verified = date.fromisoformat(meta["verified_at"])
            review_after = (verified + timedelta(days=cfg["review_days"])).isoformat()
        elif old.get("verified_at"):
            verified = date.fromisoformat(old["verified_at"])
            review_after = (
                old.get("review_after")
                or (verified + timedelta(days=cfg["review_days"])).isoformat()
            )
        else:
            verified = newest
            review_after = (verified + timedelta(days=cfg["review_days"])).isoformat()
        node = {
            "id": nid,
            "revision": old.get("revision", 1),
            "kind": kind,
            "layer": layer,
            "state": "accepted",
            "project_id": cfg["project"],
            "verified_at": verified.isoformat(),
            "review_after": review_after,
            "files": [path],
        }
        if meta.get("verified_commit"):
            cited, _ = cited_files(path, cfg)
            if not cited:
                # A fact that cites only folders (or nothing the scan can see) keeps the
                # evidence a person curated; a fact that cites files is authoritative, so a
                # file it stops citing stops being watched and a split fact fits the cap.
                cited = {
                    e["path"]
                    for e in old.get("evidence") or []
                    if e["path"] != ".git" and (ROOT / e["path"]).is_file()
                }
            if not cited:
                raise Problem(f"{path} names verified_commit but cites no repository file")
            node["evidence"] = [
                {"repo": cfg["repository"], "path": p, "commit": meta["verified_commit"]}
                for p in sorted(cited)[:MAX_EVIDENCE]
            ]
        elif old.get("evidence"):
            node["evidence"] = old["evidence"]
        if old.get("dependencies"):
            node["dependencies"] = old["dependencies"]
        if old.get("links"):
            node["links"] = old["links"]
        if old.get("revision") and old.get("files") == node["files"] and _changed(old, node):
            node["revision"] = old["revision"] + 1
        nodes.append(node)

    forms = [{"form": "umbrella", "root": "", "pipelines": pipelines, "unit": "run"}]
    forms += [{"form": "pipeline", "root": p, "unit": "run"} for p in pipelines]
    source = {
        "workspace_root": "",
        "schema_version": 2,
        "access": "workspace",
        "entrypoint": cfg["entry"],
        "runtimes": cfg["runtimes"],
        "projects": [cfg["project"]],
        "nodes": nodes,
        "forms": forms,
        "stages": stages,
    }
    carried = previous_source()
    for key in CARRIED_KEYS:
        if carried.get(key):
            source[key] = carried[key]
    written = {SOURCE: json.dumps(source, indent=2, ensure_ascii=False) + "\n", **tables}
    return written, files, contracts


def _changed(old, new):
    """Whether a node's dates or evidence moved; the revision then advances once."""
    for key in ("verified_at", "review_after"):
        if old.get(key) != new.get(key):
            return True
    return (old.get("evidence") or []) != (new.get("evidence") or [])


# --- lint ------------------------------------------------------------------------------


def git_has_commit(sha):
    """True/False when the repository can answer, None when it cannot (no git, or a shallow
    clone such as a CI checkout with fetch-depth 1, where an older commit is simply absent)."""

    def git(*args):
        return subprocess.run(
            ["git", "-C", str(ROOT), *args], capture_output=True, text=True, timeout=10, check=False
        )

    try:
        shallow = git("rev-parse", "--is-shallow-repository")
        if shallow.returncode != 0 or shallow.stdout.strip() == "true":
            return None
        return git("cat-file", "-e", f"{sha}^{{commit}}").returncode == 0
    except (OSError, subprocess.TimeoutExpired):
        return None


def lint(cfg, strict=False):
    """Structural checks, any form: (errors, warnings)."""
    errors, warnings = [], []
    today = datetime.now(timezone.utc).date()  # noqa: UP017
    entry = cfg["entry"]
    if not (ROOT / entry).is_file():
        return [f"entry {entry!r} is missing"], warnings
    entry_lines = read(entry).count("\n") + 1
    if entry_lines > MAX_ENTRY_LINES:
        errors.append(
            f"{entry} has {entry_lines} lines; the entry routes and holds no payload (max {MAX_ENTRY_LINES})"
        )
    elif entry_lines > ENTRY_TARGET_LINES:
        warnings.append(
            f"{entry} has {entry_lines} lines; the reference targets under {ENTRY_TARGET_LINES}"
        )
    if "## Route by what just happened" not in read(entry):
        errors.append(f"{entry} has no '## Route by what just happened' table")

    # Twins: every folder holding both CLAUDE.md and AGENTS.md.
    folders = {}
    for p in [
        ROOT / "CLAUDE.md",
        ROOT / "AGENTS.md",
        *HOME_DIR.rglob("*.md"),
        *(ROOT / s for s in cfg["sub_entries"]),
        *(ROOT / s for s in _twins_of(cfg["sub_entries"])),
    ]:
        if p.is_file() and p.name in ("CLAUDE.md", "AGENTS.md"):
            folders.setdefault(p.parent, {})[p.name] = p.read_bytes()
    for folder, twins in folders.items():
        if len(twins) == 2 and twins["CLAUDE.md"] != twins["AGENTS.md"]:

            def pointer(text, other):
                lines = [l for l in text.decode("utf-8", "replace").splitlines() if l.strip()]
                return len(lines) == 1 and other in lines[0]

            if not (
                pointer(twins["CLAUDE.md"], "AGENTS.md") or pointer(twins["AGENTS.md"], "CLAUDE.md")
            ):
                where = folder.relative_to(ROOT).as_posix() or "the root"
                errors.append(
                    f"CLAUDE.md and AGENTS.md in {where} differ; make one a one-line pointer"
                )

    # Facts: dated, cited, not stale, evidence commit real.
    home_md = sorted(
        p for p in HOME_DIR.rglob("*.md") if "_system" not in p.parts and "output" not in p.parts
    )
    seen_commits = {}
    for p in home_md:
        rel = p.relative_to(ROOT).as_posix()
        text = p.read_text(encoding="utf-8")
        meta = frontmatter(text)
        is_fact = p.name not in ("CLAUDE.md", "AGENTS.md", "CONTEXT.md") and "/stages/" not in rel
        if is_fact and "_shared" in p.parts:
            if "verified_at" not in meta or "verified_commit" not in meta:
                errors.append(
                    f"{rel}: a fact file carries verified_at and verified_commit in its frontmatter"
                )
            else:
                try:
                    verified = date.fromisoformat(meta["verified_at"])
                except ValueError:
                    errors.append(f"{rel}: verified_at {meta['verified_at']!r} is not a date")
                    verified = None
                if verified and verified > today:
                    errors.append(f"{rel}: verified_at {verified} is in the future")
                if verified and verified + timedelta(days=cfg["review_days"]) < today:
                    (errors if strict else warnings).append(
                        f"{rel}: past review (verified {verified}, review every {cfg['review_days']} days)"
                    )
                files_cited, dirs_cited = cited_files(rel, cfg)
                if not files_cited and not dirs_cited:
                    errors.append(
                        f"{rel}: cites no repository path; a fact points at the file that owns it"
                    )
                is_index = cfg["documents_index"] and rel == f"{ICM}/{cfg['documents_index']}"
                if len(files_cited) > MAX_EVIDENCE and not is_index:
                    # The documents index links every pinned document; each is a node of its
                    # own and drifts on its own, so the cap on the index's evidence is harmless.
                    warnings.append(
                        f"{rel}: cites {len(files_cited)} files; the compiler pins {MAX_EVIDENCE} per "
                        "node (the first alphabetically) and the rest are not watched for drift. "
                        "Split the fact."
                    )
                sha = meta["verified_commit"]
                if not re.fullmatch(r"[0-9a-f]{40}", sha):
                    errors.append(f"{rel}: verified_commit {sha!r} is not a full 40-character sha")
                elif sha not in seen_commits:
                    seen_commits[sha] = git_has_commit(sha)
                if seen_commits.get(sha) is False:
                    errors.append(
                        f"{rel}: verified_commit {sha[:12]} is not a commit of this repository"
                    )
        if PLACEHOLDER.search(without_code(text)):
            errors.append(f"{rel}: a placeholder ({{…}}) is a question, not an answer")
        for m in SECRETS.finditer(text):
            errors.append(f"{rel}: looks like a secret: {m.group(0)[:24]}…")
    for rel in (entry, cfg["pointer"], *cfg["sub_entries"]):
        if (ROOT / rel).is_file():
            text = read(rel)
            if PLACEHOLDER.search(without_code(text)):
                errors.append(f"{rel}: a placeholder ({{…}}) is left")
            for m in SECRETS.finditer(text):
                errors.append(f"{rel}: looks like a secret: {m.group(0)[:24]}…")

    declared = previous_source()
    for key in SERVICE_ONLY_KEYS:
        if declared.get(key):
            errors.append(
                f"{SOURCE} declares {key!r}; a Git workspace's sync does not resolve it (only "
                "repositories, links and workspaces). Declare it on a knowledge workspace in the "
                "dashboard instead."
            )

    # Umbrella shape: pipelines, contracts, the ignore rule.
    pipelines = pipelines_of()
    if pipelines:
        ignore = HOME_DIR / ".gitignore"
        if not ignore.is_file() or "*/stages/*/output/" not in ignore.read_text(encoding="utf-8"):
            errors.append(
                f"{ICM}/.gitignore must ignore */stages/*/output/ (stage outputs are run product)"
            )
        for pipeline in pipelines:
            for f in ("CLAUDE.md", "CONTEXT.md"):
                if not (ROOT / pipeline / f).is_file():
                    errors.append(f"{pipeline}/{f} is missing")
            for folder in sorted(
                p.name for p in (ROOT / pipeline / "stages").iterdir() if p.is_dir()
            ):
                if not re.match(r"^\d\d_[a-z-]+$", folder):
                    errors.append(f"{pipeline}/stages/{folder}: stage folders are NN_name")
                    continue
                try:
                    parse_contract(pipeline, folder)
                except Problem as exc:
                    errors.append(str(exc))
    return errors, warnings


def _twins_of(paths):
    out = []
    for p in paths:
        folder, name = p.rsplit("/", 1) if "/" in p else ("", p)
        other = "AGENTS.md" if name == "CLAUDE.md" else "CLAUDE.md"
        out.append(f"{folder}/{other}" if folder else other)
    return out


# --- main ------------------------------------------------------------------------------


def main(argv=None):
    argv = sys.argv[1:] if argv is None else argv
    for stream in (
        sys.stdout,
        sys.stderr,
    ):  # the messages carry "{…}"; a cp1252 console must not crash
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(encoding="utf-8", errors="replace")
    check = "--check" in argv
    strict = "--strict" in argv
    lint_only = "lint" in argv
    if "--version" in argv:
        print(f"icm standard {STANDARD_VERSION}")
        return 0
    cfg = load_config()

    errors, warnings = lint(cfg, strict=strict)
    for w in warnings:
        print(f"warn {w}")
    for e in errors:
        print(f"FAIL {e}")
    if lint_only:
        print("lint: " + ("clean" if not errors else f"{len(errors)} problem(s)"))
        return 1 if errors else 0

    if cfg["forms"] != "umbrella":
        print(f"form {cfg['forms']!r}: lint only; this repository generates its source another way")
        return 1 if errors else 0
    try:
        written, _, _ = build(cfg)
    except Problem as exc:
        print(f"FAIL {exc}")
        return 1
    stale = []
    for path, text in written.items():
        current = read(path) if (ROOT / path).exists() else None
        if current != text:
            stale.append(path)
            if not check:
                (ROOT / path).write_text(text, encoding="utf-8", newline="\n")
    if check:
        if stale:
            print("FAIL out of date: " + ", ".join(stale) + " (run without --check)")
        if stale or errors:
            return 1
        print("up to date; lint clean")
        return 0
    print("up to date" if not stale else "rewrote " + ", ".join(stale))
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
