# Kaoru compiler and Marlov language
A programming language built completely from scratch for my vocational diploma final project.(wip)


# Marlov Language Specification

### *"Marlov Language For Everyone"*
---

## 1. Introduction & Architecture

The **Marlov** language is a high-performance, system-level programming language driven by its core compiler, **Kaoru**. Its internal engine is built using Native C paired with x86-64 Inline Assembly (`cpuid`) to handle hardware-level security binding and direct memory management.

The system supports two file types:

* **`.mlov` (Header / Interface File):** Used for declaring structures, interfaces, and permission scopes.
* **`.ml` (Implementation File):** Used for writing core application logic and routines.

---

## 2. Eserem Philosophy (ES.ER.EM) & Memory Management

The execution workflow of the Kaoru Compiler strictly adheres to the **Eserem** paradigm:

* **ES — Everything is Structure:** Core data structures rely on C-based Abstract Syntax Tree nodes (`ASTNode`) to ensure stability, high performance, and seamless support for Recursive Descent Parsing.
* **ER — Everything in RAM (Stack Scope vs. Explicit Heap):**
* **Standard Curly Braces `{ }` (Normal Scope):** Operates identically to standard C, scoping variables on the Stack RAM.
* **Semicolon-Terminated Braces `{ };` (Stack Release Trigger):** Instructs the Kaoru Compiler to emit the Assembly instruction `add rsp, N` to immediately flush the Stack Frame and reclaim RAM upon exiting the block, without relying on a Garbage Collector (Zero-GC).


* **EM — Everything is Modular:** All code modules are isolated within an Isolation Sandbox, validated by a Security Gatekeeper before execution is authorized.

---

## 3. Permission System & Hardware Binding (Security Context)

Marlov enforces permission management via Bitmask Flags (`SecurityContext`). The `@sys` directive must be declared exclusively at the header of the source file.

```marlov
/-- Request disk read permission (minimum required by Kaoru to read source files) --/
@sys.disk.read;

```

### Hardware Signature Verification

1. When Kaoru initializes, the `get_hardware_signature()` function in `mcodegen.c` executes x86-64 Inline Assembly (`cpuid`) to extract Processor Serial/Info directly from the CPU.
2. The retrieved data is processed using Bitwise XOR and Shift operations to compute a 64-bit Hardware Hash (`uint64_t`).
3. If `@sys.disk.read` is missing from the header, the compiler terminates execution immediately (`Security Violation`).
4. The generated Assembly code injects a register-level signature verification check. If the hardware footprint does not match the compiling machine, execution branches to `self_destruct` and triggers a `ud2` (Undefined Instruction) to immediately trigger a self-crash (Domain Collapse).

---

## 4. Syntax & Scope Lifecycle

### 4.1 Standard C-Style Scope `{ }`

Used for standard control flow (e.g., functions, `if` statements, `while` loops) without releasing Stack memory immediately upon exiting the block.

```marlov
if (x > 0) {
    @int temp = x;
}

```

### 4.2 Immediate Stack Flush Block `{ };`

Appending a semicolon `;` after a closing brace `}` triggers the compiler to call `scope_exit()`, calculating the current `stack_offset` and immediately reclaiming Stack RAM space.

```marlov
{
    @int temp_data = 100;
    /-- Work with temporary variable --/
}; /-- Instantly flushes Stack Frame via `add rsp, N` --/

```

---

## 5. Data Types & Variable Declarations

Static Type Checking at compile-time uses the `@` prefix symbol to explicitly declare types.

```marlov
/-- Variable Declarations --/
@int player_speed = 100;       /-- 4-Byte Integer --/
@str player_name = "Kaoru";    /-- String Literal Sequence --/

/-- Arithmetic Expressions & Precedence --/
@int total_score = 10 + 20 * 3; /-- Supports Operator Precedence (* / before + -) --/

```

---

## 6. Operators

Kaoru's Parser uses a **Recursive Descent Parsing** approach to establish operator precedence:

1. **Primary (`parse_primary`):** Numeric literals (`TOKEN_NUMBER`), string literals (`TOKEN_STRING_LIT`), and variable identifiers (`TOKEN_IDENTIFIER`).
2. **Multiplicative (`parse_multiplicative`):** Multiplication `*` and Division `/` (higher precedence).
3. **Additive (`parse_expression`):** Addition `+` and Subtraction `-` (lower precedence).

---

## 7. Kaoru Compiler Pipeline Summary

| Marlov Syntax | Token (`mtypes.h`) | AST Node Creation (`mparser.c`) | Compiler Behavior |
| --- | --- | --- | --- |
| `@sys.disk.read;` | `TOKEN_AT_SYS` | `parse_program()` | Sets Bitmask `PERM_DISK_READ` (`0x01`) |
| `@int x = 10;` | `TOKEN_AT_INT` | `create_var_decl_node()` | Allocates symbol on Stack & creates `NODE_VAR_DECL` AST Node |
| `10 + 20 * 3;` | `TOKEN_PLUS`, `TOKEN_STAR` | `create_add_node()`, `create_mul_node()` | Folds into Binary Tree based on Operator Precedence |
| `{ }` | `TOKEN_LBRACE`, `TOKEN_RBRACE` | `parse_block()` | Generates standard C-style scope |
| `{ };` | `TOKEN_RBRACE` + `TOKEN_SEMICOLON` | `scope_exit()` | Emits instructions to shift Stack Pointer and release RAM instantly |

---

## 8. Valid Marlov Code Sample (Parser & Security Compliant)

```marlov
/-- 1. File header must explicitly request disk read permissions --/
@sys.disk.read;

/-- 2. Entry point and execution scope --/
{
    /-- Variable declaration and precedence-based expression evaluation --/
    @int base_damage = 50;
    @int bonus = 10 * 2;
    @int total_damage = base_damage + bonus;
    
    @str status = "READY";
}; /-- Exits block and flushes Stack Frame, immediately releasing RAM --/

```
