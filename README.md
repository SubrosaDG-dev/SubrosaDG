## SubrosaDG

SubrosaDG is a CFD project that uses the high-order discontinuous Galerkin (DG) method for computation. The project is based on the pure template construction of C++23 and is open-sourced under the MIT license.

## Milestone

- [x] 1D Euler equation
- [x] 2D Euler equation(Roe scheme)
- [x] 2D Euler equation(Lax-Friedrichs scheme and HLLC scheme)
- [x] Riemann invariant for boundary condition
- [x] Periodic boundary condition
- [x] Ascii/Binary VTU output
- [ ] Documentation for variable storage
- [x] Curved elements
- [x] 2D Navier-Stokes equation(BR1/BR2 scheme)
- [x] Auto RawBinary File initialization
- [x] Parallel Post-processing acceleration
- [x] Compress RawBinary output(use zlib and async io)
- [ ] Cuda gemm acceleration(use cutlass)
- [x] Add SourceTerm (e.g. Boussinesq approximation)
- [x] 3D Euler/Navier-Stokes equation
- [x] Change modal basis function to Legendre polynomial
- [x] Add Shock-Capturing(Artificial Viscosity)
- [ ] Local Time Stepping(Time Gaussian Integration)
- [x] Imcompressible flow solver(Weakly Compressible EOS)

  ...
