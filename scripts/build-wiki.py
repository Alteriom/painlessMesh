#!/usr/bin/env python3
"""Build flattened GitHub Wiki pages from the docsify documentation."""

import posixpath
import re
from pathlib import Path


SOURCE_ROOT = Path("docsify-site")
OUTPUT_ROOT = Path("wiki_build")
SKIPPED_NAMES = {"_sidebar.md", "_navbar.md"}
ROOT_PAGES = {
    Path("BRIDGE_TO_INTERNET.md"): "Bridge-To-Internet.md",
    Path("USER_GUIDE.md"): "USER_GUIDE.md",
}
ROOT_LINKS = {
    Path("README.md"): "Home.md",
    Path("RELEASE_GUIDE.md"): "Release-Guide.md",
    Path("CHANGELOG.md"): "Changelog.md",
}
DOC_ALIASES = {
    "../docs/troubleshooting/common-issues.md":
        "troubleshooting-common-issues.md",
    "../docs/troubleshooting/debugging.md":
        "troubleshooting-common-issues.md",
    "../docs/troubleshooting/faq.md": "troubleshooting-faq.md",
}
REPOSITORY_BASE = "https://github.com/Alteriom/painlessMesh/"
MARKDOWN_LINK = re.compile(r"(\[[^\]]*\]\()([^)]+)(\))")
HTML_HREF = re.compile(r"(href\s*=\s*[\"'])([^\"']+)([\"'])", re.IGNORECASE)
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

    def rewrite_destination(destination: str) -> str:
        if destination.startswith(("#", "/", "http://", "https://", "mailto:")):
            return destination

        path, separator, fragment = destination.partition("#")
        resolved = posixpath.normpath(
            posixpath.join(source_relative.parent.as_posix(), path)
        )
        target = page_names.get(resolved)
        if target is not None:
            rewritten = target
            if separator:
                rewritten += "#" + fragment
            return rewritten

        if resolved.startswith("../"):
            repository_path = resolved[3:]
            candidate = Path(repository_path)
            if candidate.exists():
                kind = "blob" if candidate.is_file() else "tree"
                rewritten = f"{REPOSITORY_BASE}{kind}/main/{repository_path}"
                if separator:
                    rewritten += "#" + fragment
                return rewritten

        if path.endswith(".md"):
            raise ValueError(
                f"Unresolved Markdown link in {source_relative}: {destination}"
            )
        return destination

    def replace(match: re.Match[str]) -> str:
        return (match.group(1) + rewrite_destination(match.group(2)) +
                match.group(3))

    content = MARKDOWN_LINK.sub(replace, content)
    return HTML_HREF.sub(replace, content)


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
        for source, output_name in {**ROOT_PAGES, **ROOT_LINKS}.items()
    })
    page_names.update(DOC_ALIASES)

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
