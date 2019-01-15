# Building The Framework

Running `# make` will build the framework using cmake and any C++11 compiler.

# Generating Benchmarks

Running the following command in this directory will generate the micro benchmark cases used in our paper:

`# ./gen_paper_micro_benchmark.py | tee cases.txt`

The cases will be generated in directory `paper-micro-bench`.

However, experiments on each case will take a lot of resources (3~4 hours & ~20GB RAM per case on our server).
You may use the alternative command to generate case that cut down the scale to half:

`# BENCHMARKS_SIMPLE=1 ./gen_paper_micro_benchmark.py | tee cases.txt`

# Running Experiments

Running the following command in this directory will profile different algorithms on each case, potentially in parallel:

`# ./paper_micro_benchmark.py -j [NUMBER_OF_PARALLEL_JOBS] -i cases.txt`

For small cases generated with `BENCHMARKS_SIMPLE=1`, you can reduce the number of trials for each algorithm to speed up:

`# ./paper_micro_benchmark.py -j [NUMBER_OF_PARALLEL_JOBS] -i cases.txt -s [NUMBER_OF_TRIALS]`

The number of trials used in our paper is 5e7. For small cases 1e5 ("-s 100000" in parameter) would give you enough precision to be confident.

Results will be generated in directory `paper-micro-bench`.

# Getting Result Tables

The following command will read the results and generate `table-micro-{1,2}.{tex,html}` in current directory:

`# ./gen_tables.py` or `# NUM_TRIALS=100000 ./gen_tables.py` for small cases

The files `table-micro-{1,2}.tex` are directed included in our paper (as Table 1 and Table 2), and `table-micro-{1,2}.html` are tables that can be viewed in a html browser.

# A Complete Example For Toy Benchmarks

```
# make
# BENCHMARKS_SIMPLE=1 ./gen_paper_micro_benchmark.py | tee cases.txt
# ./paper_micro_benchmark.py -i cases.txt -s 100000
# NUM_TRIALS=100000 ./gen_tables.py
```

This would take ~10 minutes inside a Ubuntu VM running on a laptop.

Use command `# w3m table-micro-1.html` to preview the result in console.

# Commands For Micro Benchmarks In Paper

```
# make
# ./gen_paper_micro_benchmark.py | tee cases.txt
# ./paper_micro_benchmark.py -j 2 -i cases.txt
# ./gen_tables.py
```

This would take ~30 hours on our server with two E5-2640 CPUs and 64GB RAM, running Ubuntu 16.04.

# Clean all binaries and results

`# make clean`

# Internals For Extending The Framework

The base definitions of programs (as dependency graphs) are in `Base.{cpp,hpp}`

All scheduling algorithms are implemented in `Scheduler.{cpp,hpp}`.
All of them are based on topological sort on the dependency graph with different scheduling decisions.
DFSExplorer comes with partial order reduction by maintaining the sleep set, a classic technique for POR.

The benchmark programs are generated with `DataGen.cpp,Generators.{cpp,hpp}`, which is able to generate different kinds of program patterns based on parameters.

With generated programs, all sampling and measurements are done with `Calc.cpp`, based on all above components.
It first generate the ground truth by using DFSExplorer to enumerate every interleaving of the program and calculate its characteristics.
Then it runs each sampling algorithm and measure its coverage.
Finally it aggregates all data and output to a file. `gen_tables.py` will take these file to generate tables used in our paper.

