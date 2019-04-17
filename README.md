# TriQ
TriQ is the backend compiler for the [Scaffold](https://github.com/epiqc/ScaffCC) quantum programming language. TriQ takes two inputs: 1) a gate sequence produced by [ScaffCC](https://github.com/epiqc/ScaffCC) and 2) qubit connectivity and calibration data for the target machine. It compiles the program gate sequence by choosing a good initial placement of the program qubits on the hardware qubits, reducing communication, and by applying gate optimization techniques. 

# Supported Backends
TriQ currently supports both superconducting and trapped ion qubit quantum computers. It supports superconducting devices from IBM (IBMQ14, IBMQ5) and Rigetti (Aspen1, Aspen3). It supports a 5-qubit trapped ion system from University of Maryland.

# Install
Dependencies:
1. [ScaffCC](https://github.com/epiqc/ScaffCC)
2. [Z3 SMT Solver](https://github.com/Z3Prover/z3)

To install TriQ, 
```
./configure
make 
```

# Compiling a program
To run a program prog_name.scaffold with TriQ, first exract an intermediate representation using ScaffCC.
```
./scaffold.sh -b prog_name.scaffold
```
Run TriQ on the intermediate representation
```
python ir2dag.py prog_name.qasm prog_name.in
./triq prog_name.in <output_file_name> <target_backend_name> 
```
For example, to compile programs for IBMQ5 
```
./triq prog_name.in prog_name.out ibmqx5
```
For each backend, TriQ assumes that the error data for single qubit, two-qubit and readout operations are specified in the config directory. For example, for IBMQ5, the Single qubit fidelity data should be specified in ibmqx5_S.rlb. Each line in this file specifies a qubit number and the corresponding fidelity. Two-qubit gate fidelity is specified in ibmqx5_T.rlb and readout fidelity is specified in ibmqx5_M.rlb.

# Coming Soon
More compiler options and helper methods to extract reliablity data automatically rather than specifying them manually in the config files.

# Questions
Please reach out to Prakash Murali (pmurali@princeton.edu) for any questions or clarifications.

# Citing TriQ
If you use TriQ in your work, we would appreciate it if you cite our paper:

Prakash Murali, Jonathan M. Baker, Ali Javadi Abhari, Frederic T. Chong, Margaret Martonosi. "Noise-Adaptive Compiler Mappings for Noisy Intermediate-Scale Quantum Computers", International Conference on Architectural Support for Programming Languages and Operating Systems (ASPLOS), 2019.
