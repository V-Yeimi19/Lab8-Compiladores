.data
print_fmt: .string "%ld \n"

.text

.globl main
main:
 pushq %rbp
 movq %rsp, %rbp
 subq $24, %rsp
 movq $1, %rax
 movq %rax, -8(%rbp)
dowhile_0:
 movq -8(%rbp), %rax
 pushq %rax
 movq $2, %rax
 movq %rax, %rcx
 popq %rax
 addq %rcx, %rax
 movq %rax, -16(%rbp)
 movq $1, %rax
 movq %rax, -24(%rbp)
while_1:
 movq -16(%rbp), %rax
 pushq %rax
 movq $1, %rax
 movq %rax, %rcx
 popq %rax
 cmpq %rcx, %rax
 movq $0, %rax
 setg %al
 movzbq %al, %rax
 cmpq $0, %rax
 je endwhile_1
 movq -24(%rbp), %rax
 pushq %rax
 movq -16(%rbp), %rax
 movq %rax, %rcx
 popq %rax
 imulq %rcx, %rax
 movq %rax, -24(%rbp)
 movq -16(%rbp), %rax
 pushq %rax
 movq $1, %rax
 movq %rax, %rcx
 popq %rax
 subq %rcx, %rax
 movq %rax, -16(%rbp)
 jmp while_1
endwhile_1:
 movq -8(%rbp), %rax
 pushq %rax
case_2_0:
 movq (%rsp), %rax
 cmpq $1, %rax
 jne case_2_1
 movq -24(%rbp), %rax
 movq %rax, %rsi
 leaq print_fmt(%rip), %rdi
 movq $0, %rax
 call printf@PLT
 jmp endswitch_2
 jmp endswitch_2
case_2_1:
 movq (%rsp), %rax
 cmpq $2, %rax
 jne default_2
 movq -24(%rbp), %rax
 movq %rax, %rsi
 leaq print_fmt(%rip), %rdi
 movq $0, %rax
 call printf@PLT
 jmp endswitch_2
 jmp endswitch_2
default_2:
 movq $0, %rax
 movq %rax, %rsi
 leaq print_fmt(%rip), %rdi
 movq $0, %rax
 call printf@PLT
endswitch_2:
 popq %rax
 movq -8(%rbp), %rax
 pushq %rax
 movq $1, %rax
 movq %rax, %rcx
 popq %rax
 addq %rcx, %rax
 movq %rax, -8(%rbp)
 movq -8(%rbp), %rax
 pushq %rax
 movq $3, %rax
 movq %rax, %rcx
 popq %rax
 cmpq %rcx, %rax
 movq $0, %rax
 setle %al
 movzbq %al, %rax
 cmpq $0, %rax
 jne dowhile_0
enddowhile_0:
 movq $0, %rax
 jmp .end_main
.end_main:
 leave
 ret

.section .note.GNU-stack,"",@progbits
