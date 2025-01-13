## SubrosaDG

SubrosaDG is a CFD project that uses the high-order discontinuous Galerkin (DG) method for computation. The project is based on the pure template construction of C++23 and is open-sourced under the MIT license.

## Milestone

- [x] Gmsh mesh reader(Reconstruct adjacencies)
- [x] 1D Euler equation(Central scheme)
- [x] Refactor all operators to Gemm-like form(Use Eigen)
- [x] CPU acceleration(Use TBB)
- [x] 2D Euler equation(Roe scheme)
- [x] 2D Euler equation(Lax-Friedrichs scheme and HLLC scheme)
- [x] Riemann invariant for boundary condition
- [x] Periodic boundary condition
- [x] Binary VTU output
- [ ] Documentation for variable storage
- [x] Curved elements(Use high-order Gmsh mesh)
- [x] 2D Navier-Stokes equation(BR1/BR2 scheme)
- [x] Auto RawBinary File initialization
- [x] Parallel Post-processing acceleration
- [x] Compress RawBinary output(use zlib and async io)
- [x] Add SourceTerm (e.g. Boussinesq approximation)
- [x] 3D Euler/Navier-Stokes equation
- [x] Change modal basis function to Legendre polynomial
- [x] Add Shock-Capturing(Artificial Viscosity)
- [ ] Local Time Stepping(Time Gaussian Integration)
- [x] Imcompressible flow solver(Weakly Compressible EOS)
- [ ] GPU acceleration(Use SYCL)
- [ ] Inviscid Magneto-hydrodynamics Equation(HLLD scheme)

  ...
