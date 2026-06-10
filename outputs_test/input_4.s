.data
print_fmt: .string "%ld \n"

.text

.globl main
main:
 pushq %rbp
 movq %rsp, %rbp
 subq $8, %rsp
 movq $2, %rax
 movq %rax, -8(%rbp)
 movq -8(%rbp), %rax
 movq %rax, %r10
 movq $1, %rax
 cmpq %rax, %r10
 je case_0_1
 movq $2, %rax
 cmpq %rax, %r10
 je case_0_2
 movq $3, %rax
 cmpq %rax, %r10
 je case_0_3
 jmp default_0
case_0_1:
 movq $10, %rax
 movq %rax, %rsi
 leaq print_fmt(%rip), %rdi
 movq $0, %rax
 call printf@PLT
 jmp endswitch_0
 jmp endswitch_0
case_0_2:
 movq $20, %rax
 movq %rax, %rsi
 leaq print_fmt(%rip), %rdi
 movq $0, %rax
 call printf@PLT
 jmp endswitch_0
 jmp endswitch_0
case_0_3:
 movq $30, %rax
 movq %rax, %rsi
 leaq print_fmt(%rip), %rdi
 movq $0, %rax
 call printf@PLT
 jmp endswitch_0
 jmp endswitch_0
default_0:
 movq $99, %rax
 movq %rax, %rsi
 leaq print_fmt(%rip), %rdi
 movq $0, %rax
 call printf@PLT
endswitch_0:
 movq $0, %rax
 jmp .end_main
.end_main:
 leave
 ret

.section .note.GNU-stack,"",@progbits
