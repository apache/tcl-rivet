In order to build Rivet with cmake, the following software is required:

a) cmake, version 3.2 or newer.

Compiling Rivet with cmake:
-------------------------------------------------------

1) Change the working directory to "cmake".

2) Execute the following commands (all commands must be executed from the
   "cmake" directory):

   cmake -E make_directory build
   cmake -E chdir build cmake ..
   cmake --build build --target all --clean-first
   cmake --build build --target install

3) To install mod_rivet.so/Rivet library to a custom location,
   the following commands can be used:

   cmake -E make_directory build
   cmake -E chdir build cmake \
     -DAPACHE_MODULE_DIR=/home/tcl/rivet/branches/cmake/cmake/test/modules \
     -DAPACHE_LIB_DIR=/home/tcl/rivet/branches/cmake/cmake/test/ ..
   cmake --build build --target install

4) To install mod_rivet.so/Rivet library in a system where Apache Server is not
   in a known location (i.e. under Windows), you can speficy APACHE_ROOT:

   cmake -E make_directory build
   cmake -E chdir build cmake -DAPACHE_ROOT=G:/Apache24 ..
   cmake --build build --config Release --target install
