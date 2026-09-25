# --- KAORU RUNTIME ASSEMBLY GENERATED FILE ---
.intel_syntax noprefix
.text
# Authorized Hardware Hash: 0xA00F11E941C1FC
# Permissions Mask: 0x0


.global marlov_test_scope
marlov_test_scope:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    # Bind parameters (0 count)
    # --- Scope Block ENTER ---
    mov rax, 1
    # Variable Decl: a initialized at offset -8
    mov QWORD PTR [rbp - 8], rax
    # --- Scope Block ENTER ---
    mov rax, 2
    # Variable Decl: b initialized at offset -8
    mov QWORD PTR [rbp - 8], rax
    # --- If Statement Start ---
    # Symbol reference: b
    mov rax, QWORD PTR [rbp - 8]
    push rax
    # Symbol reference: a
    mov rax, QWORD PTR [rbp - 8]
    pop rbx
    cmp rax, rbx
    sete al
    movzx rax, al
    cmp rax, 0
    je .L_end_if_0
    # --- Scope Block ENTER ---
    mov rax, 3
    # Variable Decl: c initialized at offset -8
    mov QWORD PTR [rbp - 8], rax
    # --- Return Statement ---
    mov rax, 0
    mov rsp, rbp
    pop rbp
    ret
    # --- Scope Block EXIT ---
.L_end_if_0:
    # --- If Statement End ---
    # --- Scope Block EXIT ---
    # --- Scope Block EXIT ---
    mov rsp, rbp
    pop rbp
    ret

# Suppress non-executable stack linker warning
.section .note.GNU-stack,"",@progbits
