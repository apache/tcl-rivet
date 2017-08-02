In order to build Rivet with cmake, the following software is required:

a) cmake, version 3.2 or newer.

Compiling Rivet with cmake:
-------------------------------------------------------

1) Change the working directory to "cmake".

2) Execute the following commands:
  
   cmake -E make_directory build
   cmake -E chdir build cmake --config Release ..
   cmake --build build --target all --config Release --clean-first
