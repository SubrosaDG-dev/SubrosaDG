## SubrosaDG

SubrosaDG is a CFD project that uses the high-order discontinuous Galerkin (DG) method for computation. The project is based on the pure template construction of C++23 and is open-sourced under the MIT license.

## Milestone

- [x] Gmsh mesh reader(Reconstruct adjacencies)
- [x] 1D Euler equation(Central scheme)
- [x] Refactor all operators to Gemm-like form(use Eigen)
- [x] CPU parallelization(use OpenMP)
- [x] 2D Euler equation(Roe scheme)
- [x] 2D hybrid mesh computation(Triangle/Quadrangle)
- [x] ~~High-order Tecplot visualization(divide element into sub-elements)~~
- [x] High-order Paraview visualization(use VTK arbitrary-order Lagrange element)
- [x] 2D Euler equation(Lax-Friedrichs scheme and HLLC scheme)
- [x] Non-Reflecting farfield boundary condition(use Riemann invariant)
- [x] Periodic boundary condition(change mesh topology)
- [x] Binary VTU output(use vtu11)
- [x] Terminal tui residual visualization(use tqdm-cpp)
- [x] Add DG method discretization documentation
- [ ] Documentation for variable storage
- [x] High-order Isoparametric element(Gmsh generated mesh)
- [x] 2D Navier-Stokes equation(BR1/BR2 scheme)
- [x] Auto RawBinary File initialization
- [x] Parallel Post-processing(use OpenMP)
- [x] Compress RawBinary output(use zstd and async io)
- [x] Add Source-Term (e.g. Boussinesq approximation)
- [x] 3D Euler/Navier-Stokes equation
- [x] Time dependent boundary condition
- [x] Add Modal basis function(Lobatto function)
- [x] ~~Add Shock-Capturing(Artificial Viscosity)~~
- [ ] Local Time Stepping(Time Gaussian integration)
- [x] Add Dockerfile and devcontainer.json
- [x] Incompressible flow solver(Weakly Compressible EOS)
- [x] Weakly Compressible EOS Riemann solver(for Heat Conduction)
- [x] Optimize compiler system to IntelSYCL
- [x] Change CPU parallelization to oneTBB
- [ ] Change data-structure to SOA
- [ ] GPU acceleration(use SYCL)
- [ ] Inviscid Magneto-hydrodynamics Equation(HLLD scheme)
- [ ] Local Divergence-free Projector
- [ ] Change Shock-Capturing method(DG/FV hybrid method)
- [ ] MPI/OpenMP hybrid parallelization(in CPU)
- [ ] MPI hybrid multi-GPU parallelization(use SYCL)

  ...
