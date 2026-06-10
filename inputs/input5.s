.data
print_fmt: .string "%ld \n"

.text

.globl main
main:
 pushq %rbp
 movq %rsp, %rbp
 subq $8, %rsp
 movq $1, %rax
 movq %rax, -8(%rbp)
dowhile_0:
 movq -8(%rbp), %rax
 movq %rax, %r10
 movq $1, %rax
 cmpq %rax, %r10
 je case_1_1
 movq $2, %rax
 cmpq %rax, %r10
 je case_1_2
 movq $3, %rax
 cmpq %rax, %r10
 je case_1_3
 jmp default_1
case_1_1:
 movq $101, %rax
 movq %rax, %rsi
 leaq print_fmt(%rip), %rdi
 movq $0, %rax
 call printf@PLT
 jmp endswitch_1
 jmp endswitch_1
case_1_2:
 movq $202, %rax
 movq %rax, %rsi
 leaq print_fmt(%rip), %rdi
 movq $0, %rax
 call printf@PLT
 jmp endswitch_1
 jmp endswitch_1
case_1_3:
 movq $303, %rax
 movq %rax, %rsi
 leaq print_fmt(%rip), %rdi
 movq $0, %rax
 call printf@PLT
 jmp endswitch_1
 jmp endswitch_1
default_1:
 movq $999, %rax
 movq %rax, %rsi
 leaq print_fmt(%rip), %rdi
 movq $0, %rax
 call printf@PLT
endswitch_1:
 movq -8(%rbp), %rax
 pushq %rax
 movq $1, %rax
 movq %rax, %rcx
 popq %rax
 addq %rcx, %rax
 movq %rax, -8(%rbp)
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
 jne dowhile_0
endwhile_0:
 movq $0, %rax
 jmp .end_main
.end_main:
 leave
 ret

.section .note.GNU-stack,"",@progbits
