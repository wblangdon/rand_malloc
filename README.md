Compile with GNU GCC compiler (will need tweaks to run on other versions of malloc).

Run on command line:
  1st argument is seed for pseudo random number generator (optional)
  2nd argument is number of malloc blocks to try (default 1)

  Example:
    rand_malloc 1801513025 2153137
  expected output:
    rand_malloc $Revision: 1.33 $ seed=1801513025 2153137 3145728x10 glibc 2.34 stable
    2178301 Max_malloc 670189 bytes
    peak_malloc 811008 bytes

  2178301 is the number of simulated ticks (each malloc is one tick).
  Max_malloc 670189 is the peak number of useful bytes (created by malloc).
  peak_malloc 811008 is the maximum size of the heap (as reported by mallinfo2).
