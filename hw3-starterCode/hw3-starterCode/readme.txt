Assignment #3: Ray tracing

FULL NAME: Ziang Liu


MANDATORY FEATURES
------------------

<Under "Status" please indicate whether it has been implemented and is
functioning correctly.  If not, please explain the current status.>

Feature:                                 Status: finish? (yes/no)
-------------------------------------    -------------------------
1) Ray tracing triangles                  yes

2) Ray tracing sphere                     yes

3) Triangle Phong Shading                 yes

4) Sphere Phong Shading                   yes

5) Shadows rays                           yes

6) Still images                           yes

                                          000-004.jpg:   Core credit rendering
                                          005-006.jpg:   Recursive ray tracing reflection
                                          007.jpg:       High-resolution anti-aliasing (compare with 003.jpg)
                                          008.jpg:       Soft shadow demo
   
7) Extra Credit (up to 20 points)
========================================================================================================================================
1. Recursive ray tracing to allow reflections.
   To test this, change the value of the variable "ENABLE_REFLECTION" at the very beginning of hw3.cpp to true and recompile with "make".
   Demo: 
========================================================================================================================================
2. Parallelization using OpenMP. This allows parallel computation of pixel colors to hugely speed up rendering.
   The hw3.cpp already includes code for this, but it requires "libomp". I modified the makefile to use "g++-9" because using vanilla "g++"
   would somehow lead to the usage of "Apple Clang", which does not support OpenMP (it should work with any recent version of g++, just
   make sure you don't just use "g++" and modify the makefile accordingly). For your convinience, I created two makefiles, namely
   "makefile_openmp" and "makefile_original". To use the sequencial version, copy contents in "makefile_original" to "Makefile" and compile
   with "make" as usual. To test parallelization, copy over contents in "makefile_openmp". Then, go to the top of "hw3.cpp", set the 
   "ENABLE_OPENMP" variable to "true", and recompile. This would allow the execution of OpenMP codes. The program also prints number of 
   threads used during execution in command line.
========================================================================================================================================
3. HRAA (High-resolution Anti-Aliasing). Achieved by shooting 4 additional rays for each pixel and taking the weighted average when computing
final colors. This is most visible in "table.scene", illustrated in screenshots: TODO: fill in img number
========================================================================================================================================
4. Soft shadow using modified version of test2.scene, using a cluster of 36 lights in place of the original 1 light. 
   Test file: "soft_test2.scene"
========================================================================================================================================