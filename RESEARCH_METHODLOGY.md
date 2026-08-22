# Research Methodology & Experimental Setup

**Project:** Marlov Language & Kaoru Compiler  
**Author:** Arsira “Marlov” Langphang  
**Status:** Research & Methodology Notes (Prior Art Declaration)

## Paper Title

**Explicit AST-Driven Stack Reclamation for Bare-Metal ARM Cortex-M Architecture**

---

## 1. Experimental Setup & Benchmarking Methodology

### Experimental Objective

To prove that the Kaoru Compiler delivers **Deterministic Stack Reclamation** through front-end AST semantics. By shifting memory management intent to the front-end, Kaoru reduces memory footprint and eliminates runtime cleanup overhead in bounded-memory environments (ARM Cortex-M). This study benchmarks the efficiency of front-end explicit intent against back-end optimization inference found in C (GCC -O0, GCC -O2) and C++ RAII.

### Hardware & Software Environment

| Component            | Tool / Specification              | Description                                                        |
| -------------------- | --------------------------------- | ------------------------------------------------------------------ |
| Target Emulator      | `qemu-system-arm`                 | Emulating ARM Cortex-M3/M4 architecture on Debian OS               |
| Toolchain            | `arm-none-eabi-gcc`, `arm-none-eabi-g++` | Cross-compilation baselines                                 |
| Profiler & Debugger  | `arm-none-eabi-gdb`               | Register inspection (`sp/r13`) & memory state profiling            |

### Metrics Collection & Baselines

#### Test Case Variant Matrix

| Version | Compiler / Baseline            | Description                                                                                                   |
| ------- | ------------------------------ | ------------------------------------------------------------------------------------------------------------- |
| **A**   | Kaoru Compiler                 | Uses explicit scope syntax `{ @int x = 10; };` to trigger front-end AST-driven stack flushing via direct assembly emission. |
| **B**   | C / Unoptimized Baseline       | Local variables in nested scopes compiled via GCC `-O0`.                                                      |
| **C**   | C / Back-End Optimized Baseline| Local variables in nested scopes compiled via GCC `-O2` (evaluates back-end static inference and stack slot coloring). |
| **D**   | C++ RAII Baseline              | Structs/Classes with destructors compiled via G++ `-O2` (evaluates cleanup landing-pad and destructor overhead). |

#### Peak Stack Depth Measurement (Bytes)

1. Execute custom GDB scripts to perform **Stack Painting** by filling the designated stack memory range with the pattern `0xDEADBEEF`.
2. Run the test case to completion through the targeted execution scope.
3. Inspect memory address offsets to calculate the exact byte size overwritten by non-`0xDEADBEEF` data.

#### Instruction Count & Determinism Profiling (Cycles)

- Utilize QEMU alongside GDB instruction profiling to count ARM Thumb-2 assembly instructions executed from scope entry to stack frame restoration.
- Compare instruction emission determinism between Kaoru's direct front-end emission (`ADD SP, SP, #N`) against C++ RAII cleanup routines/landing-pads and GCC back-end inferred optimizations.

---

## 2. Paper Structural Outline (IEEE Conference Format)

### Page 1: Abstract & Introduction

**Title:** Explicit AST-Driven Stack Reclamation for Bare-Metal ARM Cortex-M Architecture

**Abstract** (~150 words):  
Addresses SRAM exhaustion in resource-constrained embedded systems \(\rightarrow\) Proposes shift-left memory management using AST scope triggers (`{};`) in the Kaoru Compiler for front-end deterministic stack reclamation \(\rightarrow\) Summarizes empirical improvements in peak stack depth and instruction overhead over baseline compilers.

**I. Introduction:**  
Explores memory allocation limits on ARM Cortex-M, stack overflow risks, back-end optimizer pass overheads in production compilers, and the drawbacks of runtime-heavy frameworks on bare-metal systems.

### Page 2: Related Work & Proposed Architecture

**II. Related Work:**  
Analyzes modern memory management paradigms including C++ RAII and Rust Lifetimes (borrow checker at Front-End/MIR), highlighting code size and landing-pad overhead constraints on microcontrollers.

**III. AST-Driven Reclamation Mechanism:**  
Architecture pipeline diagram: **Lexer \(\rightarrow\) AST Parser \(\rightarrow\) ARM Thumb-2 Direct Emitter**.  
Direct code mapping from front-end explicit syntax (`{};`) down to assembly instructions (`ADD SP, SP, #N`) bypassing complex back-end analysis passes.

### Page 3: Evaluation & Results

**IV. Experimental Setup & Benchmarking:**  
Configures simulation parameters (QEMU, ARM Cortex-M4, 32KB SRAM).

**Empirical Analysis (Visual Charts):**

- **Graph A (Bar Chart):** Peak Stack Usage (Bytes) comparing Kaoru Front-End, GCC -O0, GCC -O2, and G++ RAII.
- **Graph B (Bar Chart):** Execution Overhead (Instruction Count / Clock Cycles) during scope frame reclamation, comparing direct front-end emissions against back-end inferred instructions.

### Page 4: Discussion, Conclusion & References

**V. Discussion:**  
- Trade-off analysis: Explicit front-end intent vs. back-end optimization inference.
- Benefits of achieving zero-runtime determinism without heavy compiler passes.
- Engineering trade-offs (e.g., sequential scope instruction count vs. GCC -O2) and mitigation strategies during register spilling.

**VI. Conclusion:**  
Summarizes the viability of AST scope triggers in guaranteeing memory determinism for bare-metal architectures directly from language design.

**References:**  
Citations covering ARM Cortex-M architecture, Rust/C++ RAII mechanisms, and front-end compiler design principles (5–8 citations).

---

## 3. Immediate Action Plan

1. Develop GDB Python scripts for Stack Painting (`0xDEADBEEF`) and automated SP register tracking in Debian.
2. Compile and generate ARM Thumb-2 assembly directly from the Kaoru Compiler front-end emitter.
3. Collect quantitative data (Bytes and Instruction Cycles) to contrast front-end direct reclamation against back-end inferred optimization, generating dataset visualizations for LaTeX integration.

---

## 4. Post-Evaluation Decision Tree & Continuation Pipeline (Paper 2 Trajectory)

### BRANCH A (IF POSITIVE RESULT): Formal Bounds & Verification

**Scenario:** Kaoru proves to achieve equal or superior instruction efficiency to GCC -O2 while maintaining \(100\%\) deterministic stack bounds and eliminating C++ RAII landing-pad overhead.

**Paper 2 Title:**  
Compile-Time Upper-Bound Verification for Safety-Critical RTOS via Front-End AST Semantics

**Key Hypothesis (\(H_1\)):**  
By leveraging explicit AST reclamation triggers (`{};`), the compiler can compute the exact, non-probabilistic peak stack depth at compile time, eliminating the need for dynamic runtime stack monitoring or worst-case execution time (WCET) heuristics in safety-critical RTOS applications.

**Paper Structural Outline (IEEE Format):**

1. **Introduction:** The challenge of stack overflow in safety-critical systems (ISO 26262 / DO-178C standard compliance) and limitations of dynamic stack monitoring.
2. **Static Bound Synthesis Engine:** Mathematical formulation of stack frame boundaries using front-end AST traversal algorithms without control-flow graph (CFG) state explosion.
3. **Formal Proof & Verification:** Proving that \(\text{Stack}_{\text{Peak}} \le \sum \text{AST}_{\text{Scope}_{\text{Max}}}\) for any non-recursive execution path on ARM Cortex-M.
4. **Empirical Evaluation:** Benchmarking static memory upper bounds against FreeRTOS/Zephyr RTOS stack allocation margins.
5. **Conclusion:** Establishing a certified zero-runtime-overhead stack safety model for mission-critical embedded systems.

### BRANCH B (IF TRADE-OFF RESULT): Adaptive Scope Optimization

**Scenario:** Front-end direct reclamation introduces instruction bloat or register spilling overhead in dense, sequential scopes compared to global back-end optimizations (GCC -O2).

**Paper 2 Title:**  
Heuristic Scope Coalescing: Balancing Instruction Overhead and Memory Determinism in Front-End Reclamation

**Key Hypothesis (\(H_1\)):**  
An intermediate AST pass that dynamically coalesces adjacent explicit scopes based on register pressure thresholds will eliminate front-end instruction bloat while preserving guaranteed memory bounds for high-priority variables.

**Paper Structural Outline (IEEE Format):**

1. **Introduction:** Analyzing the trade-offs of naive front-end stack reclamation: instruction count penalties vs. memory determinism during high register pressure.
2. **Adaptive Scope Coalescing Algorithm:** Introducing a lightweight AST pass that merges adjacent `{ @int x; };` blocks when register spilling is detected, converting multiple stack pointer adjustments (`ADD SP, SP, #N`) into a single coalesced reclamation frame.
3. **Compiler Infrastructure:** Implementing register pressure estimation heuristic within the Kaoru Parser before final Thumb-2 code generation.
4. **Experimental Benchmark:** Measuring the reduction in instruction overhead (Cycles) and binary footprint (Bytes) across sequential vs. nested scope benchmarks on QEMU.
5. **Discussion & Conclusion:** Defining the optimal Pareto frontier between strict AST determinism and back-end optimization efficiency.

### IDEA

Unlike traditional compiler architectures where the middle-end infers memory lifecycles via control-flow graphs, our architecture establishes the front-end AST as the authoritative intent-giver for stack reclamation. The middle-end functions strictly as a constraint-preserving optimizer—improving execution velocity while contractually preserving the explicit structural boundaries defined by the AST.

### Equation Idea: Uncontextualized Operator == False

**Q1: Why is the 'vs' of N vs NP ambiguous?**

The expression `N vs NP` is ambiguous because the token `vs` is not a well-defined binary operator in formal language or complexity theory. It does not specify which relation is being asserted between the two operands:

- Set equality: \(N = NP\)
- Strict inclusion: \(N \subsetneq NP\)
- Many-one reducibility: \(N \leq_m NP\)
- Polynomial-time equivalence: \(N \equiv_P NP\)
- Resource-bounded separation: \(N \cap NP = \emptyset\)

Without a context that fixes the intended relation, the operator `vs` is an **uncontextualized operator**. In denotational semantics, an operator without a defined interpretation maps to bottom/false:

\[
\llbracket \text{vs} \rrbracket_{\Gamma} = \bot
\]

Thus `Uncontextualized Operator == False` is not a claim about `N` or `NP`, but about the missing semantic context: an operator that has not been given a relation type cannot produce a truth value.

**Q2: What do we do with the N vs NP equation in which part of Compiler?**

In the Kaoru compiler architecture, the N vs NP equation is not treated as an open complexity-theoretic problem to be solved by the middle-end. Instead, it is used as a **design guard** at the front-end/middle-end boundary:

- **Front-end policy layer:** The AST scope trigger `{};` classifies stack reclamation as a **deterministic N problem**—a direct \(O(1)\) stack-pointer adjustment (`ADD SP, SP, #N`) determined entirely by lexical context.
- **Middle-end constraint layer:** The middle-end is allowed only to optimize under the constraint that it does not turn this deterministic N problem into an NP-style search problem (e.g., CFG liveness inference, graph-coloring slot assignment, or landing-pad reconstruction).
- **Compiler correctness check:** Before emitting Thumb-2 assembly, the compiler verifies that every stack reclamation satisfies \(SP_{exit} == SP_{entry}\) with a single deterministic frame adjustment. If a proposed optimization would require solving an NP-complete liveness or scheduling problem, it is rejected as violating the architectural contract.

Thus the "equation" is used in the **front-end policy validator and the middle-end constraint checker**, not as a computational problem for the compiler to solve. It encodes the architectural commitment that memory reclamation intent is fixed by AST syntax, and the only "vs" relation allowed is a deterministic, compile-time, \(O(1)\) one.

---

## 5. Paper 3 Trajectory: Tight Front-End/CodeGen Coupling & IR Dogma Challenge

### Core Thesis & Title

**Challenging the IR Isolation Dogma: Direct Front-End to Target-Emitter Synergy for Deterministic Microcontroller Architecture**

### Core Research Hypothesis (\(H_1\))

Integrating explicit front-end AST scope semantics directly into target-aware code generation—bypassing non-deterministic middle-end IR lifecycle analysis—yields deterministic stack bounds, reduces compilation complexity, and maintains optimal cycle efficiency for resource-constrained ARM Cortex-M bare-metal architectures without compromising system-level safety invariants.

**Formal Statement (\(H_1\)):**  
Direct coupling between lexical AST scope boundaries and Thumb-2 target emitters eliminates the semantic loss inherent in IR lowering, replacing heuristic CFG liveness inferences with guaranteed \(O(1)\) stack frame reclamation (\(SP_{exit} == SP_{entry}\)) at static compile-time.

**Null Hypothesis (\(H_0\)):**  
Front-end AST-driven direct emission provides no statistically significant reduction in peak stack depth (\(Stack_{Peak}\)) or instruction overhead compared to traditional back-end CFG slot-coloring algorithms (GCC -O2).

### Core Rationale

Industrial middle-end IR passes strip front-end scope intents. Re-inferring variable lifetimes via back-end CFGs introduces non-determinism and code bloat. Unifying front-end AST triggers directly with target-specific code generation preserves lexical intent while achieving zero-runtime-overhead memory determinism.

### Key Architectural Paradigm Shifts (To Be Benchmarked)

1. **Shift-Left Intent Contract**  
   Traditional compilers treat Front-Ends as passive syntax parsers that pass opaque IRs to Middle-Ends. Paper 3 formalizes the AST Scope Trigger (`{};`) as an immutable contract—transferring authority directly to the target emitter to execute immediate, deterministic SP adjustments without waiting for Back-End inference passes.

2. **Target-Aware Front-End Emitter (KISS Principle)**  
   Instead of forcing intermediate lowering to generic, architecture-agnostic IRs that obfuscate hardware constraints, Kaoru demonstrates that target-aware emitter hooks at the AST level allow \(O(1)\) assembly emission (`ADD SP, SP, #N`) directly for fixed-target architectures (ARM Thumb-2 ISA), drastically lowering compiler execution time and memory footprint.

### Paper 3 Structural Outline (IEEE Conference Format)

1. **Introduction:** The historical context of industrial compiler separation of concerns (GCC/LLVM) and why target-agnostic IR abstractions create critical non-determinism and code bloat in resource-constrained bare-metal environments (SRAM-bound ARM Cortex-M).
2. **The Architectural Dogma of IR Lifecycle Inference:** Mathematical and structural breakdown of semantic loss during AST-to-IR lowering. Demonstrating how Back-End CFG Liveness Analysis wastes compilation cycles "guessing" intent that the Front-End AST already possessed.
3. **Target-Coupled Scope Emitter Engine:** Structural blueprint of Kaoru’s direct AST-to-Thumb-2 translation layer. Detail-oriented walkthrough of lexical scope entry/exit state tracking and \(O(1)\) frame-pointer synchronization (SP alignment).
4. **Empirical Evaluation & Benchmarking:**
   - **Compilation Speed & Pass Complexity:** Time-to-binary compilation metrics comparing Kaoru’s direct pipeline against GCC -O0/-O2 pass stacks.
   - **Code Bloat & Landing-Pad Elimination:** Quantitative comparison of emitted assembly instruction density across nested scope boundaries.
   - **Deterministic Memory Invariance:** Verification of \(SP_{exit} == SP_{entry}\) boundary conditions under high register pressure.
5. **Discussion & Architecture Trade-offs:** Addressing software engineering concerns regarding compiler modularity vs. bare-metal performance optimization.
6. **Conclusion:** Re-evaluating compiler pipeline architecture for next-generation safety-critical microcontrollers.

---

## 6. Paper 4 Trajectory: Physical World Optimization via Lightweight Middle-End

### Core Thesis & Title

**Lightweight Middle-End Energy & Thermal Coalescing for Scope-Driven Bare-Metal Compilers**

### Core Research Hypothesis (\(H_1\))

A deterministic, lightweight middle-end pass that evaluates dynamic instruction density—without re-inferring AST scope semantics—significantly reduces processor toggle rates and dynamic power consumption on ARM Cortex-M while preserving front-end scope invariants.

**Formal Statement (\(H_1\)):**  
A lightweight mathematical middle-end pass operating purely on physical constraints (Dynamic Instruction Density & Register Transition Frequencies) reduces dynamic switching power consumption (\(P_{dynamic} = \alpha C V^2 f\)) by minimizing high-frequency stack pointer adjustments (`ADD SP, SP, #N`), while maintaining \(100\%\) compliance with front-end lexical AST scope boundaries.

**Null Hypothesis (\(H_0\)):**  
Evaluating dynamic instruction density via a lightweight middle-end pass yields no statistically significant decrease in processor gate switching rates or thermal dissipation profiles compared to direct uncoalesced front-end AST instruction emission.

### Core Rationale

Front-end AST direct reclamation optimizes memory footprint and determinism, but dense/rapid scope flushes can increase dynamic instruction toggles. Traditional middle-ends attempt to reconstruct scope semantics from scratch. Kaoru’s lightweight middle-end operates strictly as a **Physical Optimization Engine**—taking immutable intent from the AST and applying linear algebraic scheduling to minimize dynamic power dissipation without stripping scope guarantees.

### Key Architectural Paradigm Shifts (To Be Benchmarked)

1. **Immutable Intent, Flexible Physical Pacing**  
   Unlike GCC/LLVM where the middle-end mutates or guesses lifecycle scopes, Kaoru’s middle-end treats the front-end AST contract as immutable. It only optimizes the physical execution rhythm (instruction grouping and bus toggle reduction) to prevent local thermal hotspots in silicon.

2. **Deterministic Power-Aware Scheduling (\(O(1)\) Pass)**  
   Instead of executing computationally expensive Graph Coloring or Complex Control-Flow Graph (CFG) Liveness Analysis, the pass evaluates a simple Linear Instruction Density Matrix to merge adjacent stack adjustments only when register pressure and thermal thresholds dictate.

### Paper 4 Structural Outline (IEEE Conference Format)

1. **Introduction:** Physical constraints in ultra-low-power microcontrollers (Energy Harvesting Systems, Medical Implants, Automotive Sensors). The impact of dynamic gate toggling (\(P_{dynamic}\)) caused by high-frequency stack reclamation.
2. **The Dual-Role Pipeline:** Defining the boundary between Semantic Authority (Front-End AST) and Physical Energy Management (Lightweight Middle-End).
3. **Thermal & Energy Coalescing Formulation:**
   - Mathematical formulation of the Instruction Density Threshold (\(\Delta_{density}\)).
   - Algorithmic design of the \(O(1)\) Scope Coalescing Pass for ARM Thumb-2 instructions.
4. **Empirical Evaluation & Hardware Simulation:**
   - **Bus & Gate Toggle Analysis:** Counting switching bits across internal ARM Cortex-M registers using QEMU/GDB execution trace analysis.
   - **Dynamic Power Profile Estimation:** Benchmarking power consumption metrics (\(mJ\)/execution) across direct AST emission vs. lightweight middle-end coalesced emission vs. GCC -O2.
   - **Thermal Hotspot Mitigation:** Analyzing thermal dissipation patterns in high-density loop and scope executions.
5. **Discussion & Pareto Optimization:** Mapping the Pareto Frontier between Absolute Memory Determinism (Paper 1/3), Formal Bounds Safety (Paper 2), and Energy/Thermal Efficiency (Paper 4).
6. **Conclusion:** Establishing a holistic, hardware-aware compiler framework that unifies language semantics with physical silicon realities.

---

## 7. Strategic Defense Strategy & Q&A

### Architectural Domain Boundary Defense

**Q:** What about edge cases X, Y, and Z? (e.g., dynamic heap allocation, unbounded recursion, or application-level side effects)

**A:** Those cases are intentionally excluded from the target domain—bare-metal safety-critical systems—by design, in exchange for 100% compile-time determinism and zero runtime overhead. Attempting to solve application-level wicked problems inside the compiler front-end only introduces over-engineering and semantic loss, as seen in traditional production toolchains like GCC and LLVM.
