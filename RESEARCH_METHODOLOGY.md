# Research Methodology & Experimental Setup

> **Project:** Kaoru Compiler  
> **Author:** Arsira Langphang (Noriyuki Marlov Takahiro)  
> **Status:** Research & Methodology Notes (Prior Art Declaration)

---

## Paper Title
**Explicit AST-Driven Stack Reclamation for Bare-Metal ARM Cortex-M Architecture**

---

## 1. Experimental Setup & Benchmarking Methodology

### Experimental Objective
To prove that the **Kaoru Compiler** delivers **Deterministic Stack Reclamation** through front-end AST semantics. By shifting memory management intent to the front-end, Kaoru reduces memory footprint and eliminates runtime cleanup overhead in bounded-memory environments (ARM Cortex-M). This study benchmarks the efficiency of front-end explicit intent against back-end optimization inference found in C (`GCC -O0`, `GCC -O2`) and C++ RAII.

---

### Hardware & Software Environment
| Component | Tool / Specification | Description |
| :--- | :--- | :--- |
| **Target Emulator** | `qemu-system-arm` | Emulating ARM Cortex-M3/M4 architecture on Debian OS |
| **Toolchain** | `arm-none-eabi-gcc`, `arm-none-eabi-g++` | Cross-compilation baselines |
| **Profiler & Debugger** | `arm-none-eabi-gdb` | Register inspection (`sp`/`r13`) & memory state profiling |

---

### Metrics Collection & Baselines

#### Test Case Variant Matrix
* **Version A (Kaoru Compiler):** Uses explicit scope syntax `{ @int x = 10; };` to trigger front-end AST-driven stack flushing via direct assembly emission (without back-end optimization passes).
* **Version B (C / Unoptimized Baseline):** Local variables in nested scopes compiled via `GCC -O0` (evaluates traditional front-end behavior without optimization).
* **Version C (C / Back-End Optimized Baseline):** Local variables in nested scopes compiled via `GCC -O2` (evaluates back-end static inference and stack slot coloring).
* **Version D (C++ RAII Baseline):** Structs/Classes with destructors compiled via `G++ -O2` (evaluates cleanup landing-pad and destructor overhead).

#### Peak Stack Depth Measurement (Bytes)
1. Execute custom GDB scripts to perform **Stack Painting** by filling the designated stack memory range with the pattern `0xDEADBEEF`.
2. Run the test case to completion through the targeted execution scope.
3. Inspect memory address offsets to calculate the exact byte size overwritten by non-`0xDEADBEEF` data.

#### Instruction Count & Determinism Profiling (Cycles)
1. Utilize QEMU alongside GDB instruction profiling to count ARM Thumb-2 assembly instructions executed from scope entry to stack frame restoration.
2. Compare instruction emission determinism between Kaoru's direct front-end emission (`ADD SP, SP, #N`) against C++ RAII cleanup routines/landing-pads and GCC back-end inferred optimizations.

---

## 2. Paper Structural Outline (IEEE Conference Format)

### Page 1: Abstract & Introduction
* **Title:** *Explicit AST-Driven Stack Reclamation for Bare-Metal ARM Cortex-M Architecture*
* **Abstract (~150 words):** Addresses SRAM exhaustion in resource-constrained embedded systems $\rightarrow$ Proposes shift-left memory management using AST scope triggers (`{};`) in the Kaoru Compiler for front-end deterministic stack reclamation $\rightarrow$ Summarizes empirical improvements in peak stack depth and instruction overhead over baseline compilers.
* **I. Introduction:** Explores memory allocation limits on ARM Cortex-M, stack overflow risks, back-end optimizer pass overheads in production compilers, and the drawbacks of runtime-heavy frameworks on bare-metal systems.

### Page 2: Related Work & Proposed Architecture
* **II. Related Work:** Analyzes modern memory management paradigms including C++ RAII and Rust Lifetimes (borrow checker at Front-End/MIR), highlighting code size and landing-pad overhead constraints on microcontrollers.
* **III. AST-Driven Reclamation Mechanism:**
  * Architecture pipeline diagram: `Lexer` $\rightarrow$ `AST Parser` $\rightarrow$ `ARM Thumb-2 Direct Emitter`.
  * Direct code mapping from front-end explicit syntax (`{};`) down to assembly instructions (`ADD SP, SP, #N`) bypassing complex back-end analysis passes.

### Page 3: Evaluation & Results
* **IV. Experimental Setup & Benchmarking:** Configures simulation parameters (QEMU, ARM Cortex-M4, 32KB SRAM).
* **Empirical Analysis (Visual Charts):**
  * **Graph A (Bar Chart):** Peak Stack Usage (Bytes) comparing Kaoru Front-End, `GCC -O0`, `GCC -O2`, and `G++ RAII`.
  * **Graph B (Bar Chart):** Execution Overhead (Instruction Count / Clock Cycles) during scope frame reclamation, comparing direct front-end emissions against back-end inferred instructions.

### Page 4: Discussion, Conclusion & References
* **V. Discussion:**
  * Trade-off analysis: Explicit front-end intent vs. back-end optimization inference.
  * Benefits of achieving zero-runtime determinism without heavy compiler passes.
  * Engineering trade-offs (e.g., sequential scope instruction count vs. `GCC -O2`) and mitigation strategies during register spilling.
* **VI. Conclusion:** Summarizes the viability of AST scope triggers in guaranteeing memory determinism for bare-metal architectures directly from language design.
* **References:** Citations covering ARM Cortex-M architecture, Rust/C++ RAII mechanisms, and front-end compiler design principles (5–8 citations).

---

## 3. Immediate Action Plan

1. Develop GDB Python scripts for **Stack Painting** (`0xDEADBEEF`) and automated `SP` register tracking in Debian.
2. Compile and generate ARM Thumb-2 assembly directly from the Kaoru Compiler front-end emitter.
3. Collect quantitative data (Bytes and Instruction Cycles) to contrast front-end direct reclamation against back-end inferred optimization, generating dataset visualizations for LaTeX integration.
---

## 4. Post-Evaluation Decision Tree & Continuation Pipeline (Paper 2 Trajectory)
### BRANCH A (IF POSITIVE RESULT): Formal Bounds & Verification

> **Scenario:** Kaoru proves to achieve equal or superior instruction efficiency to `GCC -O2` while maintaining $100\%$ deterministic stack bounds and eliminating C++ RAII landing-pad overhead.

#### Paper 2 Title
**Compile-Time Upper-Bound Verification for Safety-Critical RTOS via Front-End AST Semantics**

#### Key Hypothesis ($H_1$)
By leveraging explicit AST reclamation triggers (`{};`), the compiler can compute the exact, non-probabilistic peak stack depth at compile time, eliminating the need for dynamic runtime stack monitoring or worst-case execution time (WCET) heuristics in safety-critical RTOS applications.

#### Paper Structural Outline (IEEE Format)
* **I. Introduction:** The challenge of stack overflow in safety-critical systems (ISO 26262 / DO-178C standard compliance) and limitations of dynamic stack monitoring.
* **II. Static Bound Synthesis Engine:** Mathematical formulation of stack frame boundaries using front-end AST traversal algorithms without control-flow graph (CFG) state explosion.
* **III. Formal Proof & Verification:** Proving that $\text{Stack}_{\text{Peak}} \le \sum \text{AST}_{\text{Scope}_{\text{Max}}}$ for any non-recursive execution path on ARM Cortex-M.
* **IV. Empirical Evaluation:** Benchmarking static memory upper bounds against FreeRTOS/Zephyr RTOS stack allocation margins.
* **V. Conclusion:** Establishing a certified zero-runtime-overhead stack safety model for mission-critical embedded systems.

---

### BRANCH B (IF TRADE-OFF RESULT): Adaptive Scope Optimization

> **Scenario:** Front-end direct reclamation introduces instruction bloat or register spilling overhead in dense, sequential scopes compared to global back-end optimizations (`GCC -O2`).

#### Paper 2 Title
**Heuristic Scope Coalescing: Balancing Instruction Overhead and Memory Determinism in Front-End Reclamation**

#### Key Hypothesis ($H_1$)
An intermediate AST pass that dynamically coalesces adjacent explicit scopes based on register pressure thresholds will eliminate front-end instruction bloat while preserving guaranteed memory bounds for high-priority variables.

#### Paper Structural Outline (IEEE Format)
* **I. Introduction:** Analyzing the trade-offs of naive front-end stack reclamation: instruction count penalties vs. memory determinism during high register pressure.
* **II. Adaptive Scope Coalescing Algorithm:** Introducing a lightweight AST pass that merges adjacent `{ @int x; };` blocks when register spilling is detected, converting multiple stack pointer adjustments (`ADD SP, SP, #N`) into a single coalesced reclamation frame.
* **III. Compiler Infrastructure:** Implementing register pressure estimation heuristic within the Kaoru Parser before final Thumb-2 code generation.
* **IV. Experimental Benchmark:** Measuring the reduction in instruction overhead (Cycles) and binary footprint (Bytes) across sequential vs. nested scope benchmarks on QEMU.
* **V. Discussion & Conclusion:** Defining the optimal Pareto frontier between strict AST determinism and back-end optimization efficiency.
