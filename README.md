## SubrosaDG

SubrosaDG is a CFD project that uses the high-order discontinuous Galerkin (dG) method for computation. The project is based on the pure template construction of C++23 and is open-sourced under the MIT license.

## Milestone

- [x] Gmsh mesh reader(Reconstruct adjacency information)
- [x] 1D Euler equation(Central scheme)
- [x] Refactor all operators to Gemm-like form(use Eigen)
- [x] CPU parallelization(use OpenMP)
- [x] 2D Euler equation(Roe scheme)
- [x] 2D hybrid mesh computation(Triangle/Quadrangle)
- [x] ~~High-order Tecplot visualization(divide element into sub-elements)~~
- [x] High-order Paraview visualization(use VTK arbitrary-order Lagrange element)
- [x] 2D Euler equation(Lax-Friedrichs scheme and HLLC scheme)
- [x] Non-reflecting farfield boundary condition(use Riemann invariant)
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
- [x] Add Source-term (e.g. Boussinesq approximation)
- [x] 3D Euler/Navier-Stokes equation
- [x] Add cl/cd computation module(Paraview FieldData)
- [x] Time dependent boundary condition
- [x] ~~Add Modal basis function(Lobatto function)~~
- [x] ~~Add Shock-capturing method(Artificial Viscosity)~~
- [ ] Local Time Stepping(Time Gaussian integration)
- [x] Add Dockerfile and devcontainer.json
- [x] Incompressible flow solver(Weakly Compressible EOS)
- [x] Weakly Compressible EOS Riemann solver(for Heat Conduction)
- [x] Add pipe boundary condition(Velocity inlet/Pressure outlet)
- [x] Optimize compiler system to IntelSYCL
- [x] Change CPU parallelization to oneTBB
- [x] Change Data-structure to SOA
- [x] Optimize computation(e.g. use more references instead of temporary variables)
- [x] Device matrix struct(Eigen::Map likely)
- [x] Add Device structure for computation(sycl::is_device_copyable)
- [x] Make VariableConvertor adapt to Device matrix struct
- [x] GPU acceleration(use SYCL)
- [ ] Add pyramid Lagrange basis function(in device)
- [ ] Add viscous schemes(Direct-DG)
- [ ] Arbitrary Lagrangian-Eulerian Method(ALE)
- [ ] Rotating machine simulation(in ALE framework)
- [ ] Inviscid Magneto-hydrodynamics Equation(HLLD scheme)
- [ ] Local Divergence-free Projector
- [ ] ~~Change Shock-capturing method(DG/FV hybrid method)~~
- [ ] MPI/OpenMP hybrid parallelization(in CPU)
- [ ] MPI hybrid multi-GPU parallelization(use SYCL)

  ...
