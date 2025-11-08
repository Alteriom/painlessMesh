# Copilot Instructions Directory

This directory contains file-specific and context-specific instructions for GitHub Copilot coding agent.

## Structure

Each `.instructions.md` file in this directory can specify which files it applies to using YAML frontmatter:

```yaml
---
applies_to:
  - "path/pattern/**/*.ext"
---
```

## Available Instructions

### `testing.instructions.md`
**Applies to:** `test/**/*.cpp`, `test/**/*.hpp`

Comprehensive testing guidelines including:
- Test framework usage (Catch2)
- Writing unit tests for packages
- Running and debugging tests
- Test coverage goals

### `alteriom-packages.instructions.md`
**Applies to:** `examples/alteriom/**/*.hpp`, `examples/alteriom/**/*.ino`

Package development guidelines including:
- Package templates and patterns
- Type ID management
- Memory-efficient field types
- Arduino example patterns
- Best practices and common mistakes

### `workflows.instructions.md`
**Applies to:** `.github/workflows/**/*.yml`

CI/CD workflow guidelines including:
- Workflow overview
- Modifying build and test steps
- Security best practices
- Troubleshooting workflows

## How Copilot Uses These Instructions

When GitHub Copilot works on files matching the `applies_to` patterns, it will automatically:
1. Load the relevant instruction file
2. Follow the specific guidelines for that context
3. Apply the coding patterns and conventions
4. Use the templates and examples provided

## General Instructions

For general repository guidelines, see:
- `/.instructions.md` - Root-level quick start guide
- `.github/copilot-instructions.md` - Comprehensive repository instructions
- `.github/copilot-quick-reference.md` - Copy-paste templates
- `.github/copilot-troubleshooting.md` - Common issues and solutions
- `.github/README-DEVELOPMENT.md` - Development workflow

## Adding New Instructions

To add context-specific instructions:

1. Create a new `.instructions.md` file in this directory
2. Add YAML frontmatter with `applies_to` patterns
3. Write clear, actionable guidance
4. Include examples and templates
5. Update this README with a description

Example template:
```markdown
---
applies_to:
  - "your/path/**/*.ext"
---

# Title of Your Instructions

Brief description of what these instructions cover.

## Section 1
Content...

## Section 2
Content...
```

## Best Practices for Instructions

1. **Be Specific**: Provide concrete examples and patterns
2. **Be Actionable**: Give step-by-step guidance
3. **Include Examples**: Show code snippets and templates
4. **Explain Why**: Help Copilot understand the reasoning
5. **Keep Updated**: Maintain accuracy as the codebase evolves
6. **Cross-Reference**: Link to related instruction files

## Questions?

For questions about these instructions or suggestions for improvements:
1. Check the troubleshooting guide
2. Review existing examples in the codebase
3. Consult with repository maintainers
4. Open an issue for discussion
