# AGENTS.md

This document defines how coding agents should work in this repository.

## 1) Project snapshot

- **Project**: SubrosaDG
- **Language**: C++23
- **Build system**: CMake + Ninja + vcpkg
- **Main code**: `src/`
- **Examples**: `examples/`
- **Build output**: `build/`

SubrosaDG is a high-order DG CFD solver with CPU/GPU backends (SYCL/CUDA/ROCm options in CMake).

## 2) Environment assumptions

- Out-of-source build is required (already configured to use `build/`).
- `VCPKG_ROOT` must be set in the environment.
- CMake presets are defined in `CMakePresets.json`.
- On Linux, this project enforces Fedora in top-level CMake logic.

## 3) Preferred workflow for agents

1. Read relevant files first; avoid speculative edits.
2. Make **minimal, surgical** changes tied to the request.
3. Keep style consistent with existing code.
4. Do **not** run validation/build/test commands unless the user explicitly asks.
5. Do not modify unrelated files.

## 4) Build commands (for user manual validation)

Agents should not execute these by default. They are listed for the user to run manually:

```bash
cmake --preset "Build develop in debug"
cmake --build build -j
```

For examples:

```bash
cmake --preset "Build examples in debug"
cmake --build build -j
```

For docs:

```bash
cmake --preset "Build docs"
cmake --build build -j
```

> Validation ownership: **user-only by default**.

## 5) Code conventions to respect

- Use **C++23** idioms already present in codebase.
- Follow `.clang-format` and `.clang-tidy` configuration.
- Reuse existing utilities/helpers before introducing new abstractions.
- Avoid broad error swallowing and silent fallbacks.
- Keep comments concise and only where logic is non-obvious.

## 6) Repository map (quick)

- `src/`: core solver implementation
- `examples/`: runnable cases
- `docs/`: documentation and Doxygen config
- `cmake/`: CMake modules, triplets, helper scripts
- `libs/`: third-party or internal library sources
- `utils/`: developer utilities and tooling assets

## 7) Safety rules for agents

- Never run destructive git commands (`reset --hard`, forced checkout) unless explicitly requested.
- Do not commit secrets or credentials.
- Do not rewrite history unless explicitly requested.
- If unexpected unrelated local changes exist, leave them untouched.

## 8) Definition of done

A task is complete only when:

1. Requested code/docs changes are implemented.
2. Agent provides suggested validation commands, but does not execute them unless explicitly requested.
3. Changes are scoped, coherent, and do not regress known behavior.
