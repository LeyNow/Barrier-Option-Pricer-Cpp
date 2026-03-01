# Barrier-Option-Pricer-Cpp
A C++ pricer for Barrier option. It was created by Monte Carlo model.

If you wish to use it, here are the parameters to provide for **greater accuracy**:
- M : 100 000 - 500 000
- N : 252 - 1008

Here is the time for M = 200 000 and N = 504 with multithreading : ~20 seconds. Without this feature, the time is : 1.15 minute.
