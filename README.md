# Introduction
Top implementation for [Seastar](https://github.com/scylladb/seastar).

I use C++20 and Ubuntu 22.04 for dev and test.

# Build
Install xmake first and then run the following commands to build

+ xmake f --mode=debug
+ xmake

# Run tests
+ xmake r os_start_test
+ xmake r timer_test