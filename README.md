# Kaoru Compiler (Marlov Language)

**Kaoru** is an experimental ahead-of-time (AOT) frontend and code-generation pipeline for the **Marlov Programming Language**. Designed with a strict "Readable First, Keep It Simple, Stupid (KISS), and No Over-Engineering" philosophy, Kaoru translates Marlov high-level source files (`.ml`) and header interface declarations (`.mlov`) into platform-native assembly (x86_64 and AArch64) with runtime hardware signature verification.

---

## 📌 Research Branch Notice: ARM Cortex-M Target

> **Branch:** `research/arm-cortex-m-dag`
> **Target Environment:** Bare-Metal Embedded Systems (ARM Cortex-M Series with High SRAM Constraints)

In safety-critical bare-metal environments, bounded memory footprints and strict dynamic analysis are paramount. Due to **Rice's Theorem**, proving non-trivial semantic properties (such as stack overflow guarantees or exact memory boundaries) on arbitrary recursive execution paths is undecidable.

To overcome this constraint in highly limited SRAM environments:
* **Recursion Elimination:** The research branch enforces strict static analysis that forbids and eliminates recursive function calls at compile time.
* **DAG-Based Lowering:** Control flow and call graphs are transformed into a **Directed Acyclic Graph (DAG)**.
* **Bounded SRAM Guarantee:** Translating code paths into DAGs allows the compiler to compute exact static frame bounds and guarantee safe execution limits without dynamic stack runtime surprises.

---

## Current Architecture & Features (Main Branch)

* **Dual Target Assembly Emission:** Generates assembly output supporting both `x86_64` (System V / Windows x64 ABI) and `AArch64` target architectures.
* **Layered Pipeline:**
  * **Lexer (`mlexer.c`):** Tokenizes Marlov directives (`@func`, `@print`, `@panic`, `@sys.disk.read`), operators, identifiers, and literals.
  * **Parser (`mparser.c`):** Constructs AST representations for control flows (`if/else`, `while`, `for`), functions, pointers, and memory operations.
  * **Symbol & Header Linking (`mlov` Interface):** Resolves interface definitions and exports external symbol linkages (`.global` / `.extern`).
  * **Intermediate Representation (`mir.c` / `mir.h`):** Lowers AST to Medium-level IR with 8-byte stack frame alignment invariants (`Align8`).
  * **Code Generator (`mcodegen.c`):** Outputs architecture-specific assembly routines and cross-platform hardware signature verification guards.
* **Bare-Metal Memory & Pointer Primitives:** Full support for dereferencing (`*p`), address-of (`&x`), 8-bit byte-level loads/stores (`load_b`, `store_b`), and array indexing.
* **Bitwise & Arithmetic Operations:** Supports arithmetic, relational, and low-level bitwise logic (`&`, `|`, `^`, `<<`, `>>`).
* **Hardware Signature Guard:** Compiles hardware CPU fingerprint checks into target binaries (`cpuid`, MIDR_EL1, ARM p15) for execution authorization.

---

## Source File Overview

```text
/Kaoru Compiler
├── main.c        # CLI driver, header/source parser launcher, file I/O dispatcher
├── mtypes.h      # Core compiler types, AST node definitions, Token enums
├── mlexer.c      # Lexical analyzer for Marlov syntax and directives
├── mparser.c     # AST parser & Symbol table linkage implementation
├── mir.h         # MIR opcodes and struct definitions
├── mir.c         # Lowering AST into MIR representation
└── mcodegen.c    # Machine assembly code generator (x86_64 & AArch64)
---
## Building & Usage
---
Prerequisites
Standard C compiler (gcc or clang) with C99 support.

Compilation
Compile the Kaoru frontend compiler driver:

Bash
gcc -std=c99 -O2 main.c -o kaoru
Execution
To compile a Marlov source file (.ml) with an optional header interface (.mlov):

Bash
# Basic Compilation
./kaoru source.ml -o output.s

# Compilation with Header Interface
./kaoru source.ml -include interface.mlov -o output.s

# Inspect AST Structure
./kaoru source.ml --dump-ast

# test benchmark 
cd 'Kaoru Compiler'
cd marlov-benchmark
make clean && make
---
## Marlov Language Directives Quick Reference
@func <name>(<args>) { ... }: Function declaration.

@print(<expr>): Print string or integer values to standard output.

@sys.disk.read: Security permission directive.

@panic(<msg>): Trigger runtime error diagnostic trap.

@exit(<code>): Terminate process execution.
