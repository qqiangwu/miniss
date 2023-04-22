# Introduction
Toy implementation for [Seastar](https://github.com/scylladb/seastar).

I use C++20 and Ubuntu 22.04 for dev and test.

# Build
Use [cppship](https://github.com/qqiangwu/cppship) and [conan2](https://conan.io/) for build and tests.

# Run tests
```bash
$ cppship ipc_test
$ cppship os_start_test

# test all
$ cppship test
```