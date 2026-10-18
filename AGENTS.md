# AGENTS.md

Instructions for coding agents working on SubrosaDG.

## Project

SubrosaDG is a C++23 CFD solver based on the high-order discontinuous Galerkin (DG) method. It uses a template-based architecture, Eigen for matrix operations, Gmsh for meshes, oneTBB for CPU parallelism, and SYCL for device execution. GPU build options target Intel, NVIDIA, and AMD devices through the SYCL toolchain.

## Repository map

- `src/Mesh/`: mesh reading, geometry, elements, basis functions, and quadrature.
- `src/Solver/`: physical models, numerical fluxes, boundary conditions, spatial discretization, and time integration.
- `src/Utils/`: shared types, concepts, constants, device matrices, and system control.
- `src/View/`: raw binary output, ParaView output, and runtime display.
- `examples/`: runnable simulation cases; each `.cpp` file defines an executable with the same base name.
- `cmake/`: build helpers, generated-source templates, and vcpkg overlays.
- `docs/`: Doxygen configuration and LaTeX development notes.
- `utils/`: the development executable source and editor/debugger tooling.
- `develop/`: experiments, scripts, and development notes.
- `libs/`: bundled dependencies.
- `build/`: build products, executables in `build/bin/`, and simulation results in `build/out/`.

## Workflow

- Read the relevant implementation and configuration before editing. Prefer current source and CMake files when older development notes disagree with them.
- Keep changes focused on the requested task. Reuse existing helpers and preserve unrelated local changes.
- Agents may configure, build, run a relevant example, and perform targeted formatting or static analysis as needed for the task without asking for separate permission. When testing is needed and the required environment is available, build and run one representative example relevant to the change.
- A documentation-only change normally does not require compilation.
- Report what changed, what was checked, and what remains unverified. Never describe a suggested command as having been executed.

## Build environment

- Use CMake 3.30 or newer, Ninja, and a working vcpkg installation referenced by `VCPKG_ROOT`.
- Out-of-source builds are required. Configure presets are defined in `CMakePresets.json` and share the `build/` directory.
- On Linux, the current build requires Fedora and selects Intel oneAPI `icpx` at `/opt/intel/oneapi/compiler/latest/bin/icpx`. IntelSYCL and oneTBB are required even for the CPU configuration.
- Before validation, run `source /opt/intel/oneapi/setvars.sh` in the shell used to configure, build, and run the example. Keep these commands in the same shell session, or initialize oneAPI again in each new shell.
- The macOS CMake branch selects Homebrew LLVM. Do not assume a backend works on a platform without verification.
- GPU presets use architecture-specific SYCL targets configured in `CMakeLists.txt`. Check the installed compiler, runtime, and target hardware before attempting GPU validation.
- Reconfigure when switching presets. Preserve existing build outputs and user settings; do not delete `build/` as a routine troubleshooting step.
- For validation, always configure with `Build examples in release`, regardless of the currently active preset. There is no need to preserve or restore the previous preset.

## Build and run reference

Run validation from the repository root using the `Build examples in release` preset and one relevant example. Enable `SubrosaDG::CommandLineEnum::Agent` on the example's `system` before running it. For examples without an agent command-line option, construct the system as follows:

```cpp
SubrosaDG::System<SimulationControl> system{SubrosaDG::CommandLineEnum::Agent};
```

```bash
# Initialize oneAPI, then configure and validate one example in Agent mode.
source /opt/intel/oneapi/setvars.sh
cmake --preset "Build examples in release"
cmake --build build --target vortex_2d_ceuler --parallel
./build/bin/vortex_2d_ceuler
```

Avoid the `all_examples` target for routine validation; building and running one relevant example is sufficient.

Examples contain their simulation settings in source and may generate meshes and replace results under `build/out/<example_name>/`. Inspect the case and existing output before running it.

## Code conventions

- Follow the existing C++23 template-based design and the `SubrosaDG` namespace conventions.
- Many files under `src/` have a `.cpp` extension but are included as headers and use include guards. Preserve this structure; do not turn them into separate translation units without an explicit architectural reason.
- Follow `.clang-format`, `.clang-tidy`, and `.editorconfig`. C++ formatting uses Google style, two-space indentation, and a 120-column limit.
- Use `CamelCase` for types and namespaces, `camelBack` for functions, `lower_case` for variables, a trailing underscore for data members, and `kCamelCase` for compile-time constants. Follow nearby exceptions when required by an existing interface.
- Reuse project scalar/index types and host/device abstractions. Preserve matrix dimensions, storage order, and device-copyability assumptions.
- Prioritize runtime performance in core numerical computations. Avoid excessive `throw` statements, exception handling, and repeated defensive checks, especially in hot loops, flux evaluation, matrix operations, and device kernels. Do not use exceptions for normal control flow or add exception-based checks to device kernels.
- Validate inputs and configuration at setup or I/O boundaries where practical, then rely on established invariants in the computation path. Prefer compile-time constraints or debug assertions for internal invariants when appropriate. Reserve exceptions for meaningful failures outside performance-critical paths; do not silently ignore errors or remove necessary correctness checks merely to avoid exceptions.
- Explain non-obvious numerical reasoning and invariants in concise comments. Update relevant documentation when behavior or public interfaces change.
- When formatting, format only the affected files. The CMake `clang-format` target rewrites all collected source and example files.

## Generated files and dependencies

- `src/SubrosaDG.cpp` is generated during configuration from `cmake/source/SubrosaDG.cpp.in` and the source list collected in `cmake/GlobFiles.cmake`. Make persistent changes in the template or collection logic.
- `build/include/Cmake.cpp` is generated from `cmake/source/Cmake.cpp.in`.
- CMake also generates VS Code settings and debugger configuration. Preserve user customizations and edit the corresponding templates when changing shared defaults.
- Do not modify bundled libraries, dependency versions, or vcpkg overlays unless required by the task.
- Keep generated meshes, simulation results, binaries, and profiling reports out of source changes. Preserve existing reference data.

## Numerical validation and performance

- The current CMake configuration does not register a CTest suite. When testing is needed, use one existing case in `examples/` directly; a separate test suite is not required for routine changes.
- Avoid excessive testing. Do not run every example or expand validation across multiple cases or configurations unless the user explicitly requests broader coverage. Once the selected example passes, stop testing unless a subsequent change or failure requires a targeted rerun.
- For solver changes, check numerical behavior as well as compilation and process exit status. Choose relevant quantities such as residuals, conservation errors, analytical-solution errors, or lift/drag coefficients.
- Choose one example that best represents the affected solver behavior, use the `Build examples in release` preset, and enable `CommandLineEnum::Agent`. Report the scope tested without claiming coverage of all dimensions, element types, physical models, or backends.
- Preserve numerical schemes, boundary-condition semantics, and precision unless changing them is part of the task. Explain intentional numerical changes and their expected effects.
- For performance comparisons, keep the case, mesh, polynomial order, hardware, compiler configuration, and output settings consistent. Separate initialization and warm-up from steady-state timing.
- Recheck numerical results after changes to arithmetic order, precision, memory layout, or parallel execution. Claim speedups only when supported by comparable measurements.
- If the required environment is unavailable, state the limitation and provide the exact commands needed for follow-up validation.

## Git and change safety

- Preserve unrelated edits and user-provided data.
- Do not use destructive Git commands or rewrite history unless explicitly requested.
- Do not include secrets, credentials, or machine-specific generated artifacts in commits.
- Before handing off, review the diff for accidental changes and clearly distinguish completed work from pending validation.
