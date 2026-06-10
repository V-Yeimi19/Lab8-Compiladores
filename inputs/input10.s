.data
print_fmt: .string "%ld \n"

.text

.globl cond
cond:
 pushq %rbp
 movq %rsp, %rbp
 subq $8, %rsp
 movq %rdi, -8(%rbp)
 movq -8(%rbp), %rax
 pushq %rax
 movq $4, %rax
 movq %rax, %rcx
 popq %rax
 cmpq %rcx, %rax
 movq $0, %rax
 setle %al
 movzbq %al, %rax
 cmpq $0, %rax
 je else_0
 movq $0, %rax
 jmp .end_cond
 jmp endif_0
else_0:
 movq $1, %rax
 jmp .end_cond
endif_0:
.end_cond:
 leave
 ret

.globl main
main:
 pushq %rbp
 movq %rsp, %rbp
 subq $0, %rsp
 movq $5, %rax
 movq %rax, %rdi
 call cond
 movq %rax, %rsi
 leaq print_fmt(%rip), %rdi
 movq $0, %rax
 call printf@PLT
 movq $3, %rax
 movq %rax, %rdi
 call cond
 movq %rax, %rsi
 leaq print_fmt(%rip), %rdi
 movq $0, %rax
 call printf@PLT
 movq $0, %rax
 jmp .end_main
.end_main:
 leave
 ret

.section .note.GNU-stack,"",@progbits
