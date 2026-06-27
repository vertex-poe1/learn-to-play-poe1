<!-- .claude/CLAUDE.md (markdown) -->

# Project Guidelines

## Commits

- Only commit when the user explicitly says "commit"
- Only commit immediately after that message — do not commit after subsequent prompts even if they seem related

## Build Verification

- Always run `just test` after making code changes
- Do not ask the user to test until the tests pass

## Task Runner

Use `just` tasks whenever possible instead of raw commands:

| Task | Use instead of |
|------|---------------|
| `just build` | `cmake --preset ... && cmake --build ...` |
| `just test` | `ctest --preset ... -LE perf` |
| `just test-all` | `ctest --preset ...` |
| `just test-perf` | running perf tests manually |
| `just run` | running the app binary directly |
| `just clean` | `cmake -E rm -rf build dist` |
