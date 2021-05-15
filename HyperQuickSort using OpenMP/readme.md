# Introduction
This repo contains the implementation of the algorithm, HyperQuickSort. Parallel Quicksort implemented using Hyper-cube topology. 

It is implemented in C using OpenMP library. 

The details related to algoithm and implementation as well as the various performance related matrics has beem calcuated and derived in the design document.

# Instructions for running the program

  To compile: 
    
  To run:  
  
    ./hyperquicksort_openmp <no_of_elements(N)> <choice>
   
   here choice can be 0 or 1.
   
   0 -> for printing the time taken for sorting  and  1 -> for printing the sorted array as well as time taken.
   
   eg:   ./hyperquicksort_openmp 32768 0
   
   **NOTE: The number of threads spawned will always be a power of 2. And it can be changed by altering a MACRO in code.**
