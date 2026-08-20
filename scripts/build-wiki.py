#!/usr/bin/env python3
"""Build flattened GitHub Wiki pages from the docsify documentation."""

import posixpath
import re
from pathlib import Path


SOURCE_ROOT = Path("docsify-site")
OUTPUT_ROOT = Path("wiki_build")
SKIPPED_NAMES = {"_sidebar.md", "_navbar.md"}
ROOT_PAGES = {Path("BRIDGE_TO_INTERNET.md"): "Bridge-To-Internet.md"}
MARKDOWN_LINK = re.compile(r"(\[[^\]]*\]\()([^)]+)(\))")
DOXYGEN_BASE = "https://alteriom.github.io/painlessMesh/api-reference/"
RELATIVE_DOXYGEN_LINK = re.compile(
    r"(?:\.\./)+api-reference/([^\s\"'()]+\.html(?:#[^\s\"'()]*)?)"
)


def wiki_name(relative_path: Path) -> str:
    return "-".join(relative_path.with_suffix("").parts) + ".md"


def rewrite_links(content: str, source_relative: Path,
                  page_names: dict[str, str]) -> str:
    content = RELATIVE_DOXYGEN_LINK.sub(
        lambda match: DOXYGEN_BASE + match.group(1), content
    )

    def replace(match: re.Match[str]) -> str:
        destination = match.group(2)
        if destination.startswith(("#", "/", "http://", "https://", "mailto:")):
            return match.group(0)

        path, separator, fragment = destination.partition("#")
        if not path.endswith(".md"):
            return match.group(0)

        resolved = posixpath.normpath(
            posixpath.join(source_relative.parent.as_posix(), path)
        )
        target = page_names.get(resolved)
        if target is None:
            raise ValueError(
                f"Unresolved Markdown link in {source_relative}: {destination}"
            )

        rewritten = target
        if separator:
            rewritten += "#" + fragment
        return match.group(1) + rewritten + match.group(3)

    return MARKDOWN_LINK.sub(replace, content)


def main() -> None:
    pages = [
        path for path in SOURCE_ROOT.rglob("*.md")
        if path.name not in SKIPPED_NAMES and "node_modules" not in path.parts
    ]
    page_names = {
        path.relative_to(SOURCE_ROOT).as_posix():
        wiki_name(path.relative_to(SOURCE_ROOT))
        for path in pages
    }
    page_names.update({
        "../" + source.as_posix(): output_name
        for source, output_name in ROOT_PAGES.items()
    })

    OUTPUT_ROOT.mkdir(parents=True, exist_ok=True)
    for source in pages:
        relative = source.relative_to(SOURCE_ROOT)
        content = source.read_text(encoding="utf-8")
        rewritten = rewrite_links(content, relative, page_names)
        (OUTPUT_ROOT / page_names[relative.as_posix()]).write_text(
            rewritten, encoding="utf-8"
        )

    for source, output_name in ROOT_PAGES.items():
        content = source.read_text(encoding="utf-8")
        rewritten = rewrite_links(content, Path("..") / source, page_names)
        (OUTPUT_ROOT / output_name).write_text(
            rewritten, encoding="utf-8"
        )


if __name__ == "__main__":
    main()
