<open__stdZsyncio_u435>:
endbr64
push   %r12
mov    %rsi,%rax
movzbl %cl,%ecx
mov    %r8,%r12
push   %rbp
mov    %rdi,%rbp
push   %rbx
sub    $0xa0,%rsp
mov    %fs:0x28,%rsi
mov    %rsi,0x98(%rsp)
xor    %esi,%esi
lea    0xdc0b(%rip),%rsi        # <FormatOpen__stdZsyncio_u423>
mov    (%rsi,%rcx,8),%rsi
test   %rax,%rax
je     <open__stdZsyncio_u435+0xc0>
lea    0x8(%rdx),%rdi
call   <fopen@plt>
mov    %rax,%rbx
test   %rax,%rax
je     <open__stdZsyncio_u435+0x81>
mov    %rax,%rdi
call   <fileno@plt>
mov    %rsp,%rsi
mov    %eax,%edi
call   <fstat@plt>
test   %eax,%eax
js     <open__stdZsyncio_u435+0x88>
mov    0x18(%rsp),%eax
and    $0xf000,%eax
cmp    $0x4000,%eax
jne    <open__stdZsyncio_u435+0x88>
mov    %rbx,%rdi
call   <fclose@plt>
xor    %eax,%eax
jmp    <open__stdZsyncio_u435+0x98>
nopl   (%rax)
mov    %rbx,0x0(%rbp)
test   %r12,%r12
jg     <open__stdZsyncio_u435+0xd0>
je     <open__stdZsyncio_u435+0xe8>
mov    $0x1,%eax
mov    0x98(%rsp),%rdx
sub    %fs:0x28,%rdx
jne    <open__stdZsyncio_u435+0xfb>
add    $0xa0,%rsp
pop    %rbx
pop    %rbp
pop    %r12
ret
nopw   0x0(%rax,%rax,1)
lea    0x922c(%rip),%rdi        # <digits100__stdZprivateZdigitsutils_u2+0x173>
jmp    <open__stdZsyncio_u435+0x46>
nopl   0x0(%rax)
mov    %r12,%rcx
xor    %edx,%edx
xor    %esi,%esi
mov    %rbx,%rdi
call   <setvbuf@plt>
jmp    <open__stdZsyncio_u435+0x93>
nopl   0x0(%rax)
xor    %ecx,%ecx
mov    $0x2,%edx
xor    %esi,%esi
mov    %rbx,%rdi
call   <setvbuf@plt>
jmp    <open__stdZsyncio_u435+0x93>
call   <__stack_chk_fail@plt>

<open__stdZsyncio_u502>:
endbr64
push   %r15
mov    %rcx,%r8
movzbl %dl,%ecx
push   %r14
push   %r13
push   %r12
push   %rbp
mov    %rsi,%rbp
push   %rbx
mov    %rdi,%rbx
mov    %rbp,%rdx
mov    %rbx,%rsi
sub    $0x18,%rsp
mov    %fs:0x28,%rax
mov    %rax,0x8(%rsp)
xor    %eax,%eax
mov    %rsp,%rdi
movq   $0x0,(%rsp)
call   <open__stdZsyncio_u435>
test   %al,%al
je     <open__stdZsyncio_u502+0x70>
mov    (%rsp),%rax
mov    0x8(%rsp),%rdx
sub    %fs:0x28,%rdx
jne    <open__stdZsyncio_u502+0x125>
add    $0x18,%rsp
pop    %rbx
pop    %rbp
pop    %r12
pop    %r13
pop    %r14
pop    %r15
ret
nop
mov    $0x8,%esi
mov    $0x40,%edi
lea    0x893a(%rip),%r14        # <digits100__stdZprivateZdigitsutils_u2+0xeb>
call   <nimNewObj>
lea    0xd(%rbx),%rdi
mov    %rax,%r12
lea    0xd87c(%rip),%rax        # <NTIv2__TRNQsmYgvvbxY1uwCHSBGQ_>
mov    %rax,(%r12)
mov    %r14,0x10(%r12)
call   <rawNewString>
movabs $0x6f20746f6e6e6163,%rcx
movabs $0x203a6e65706f2074,%rsi
mov    %rdx,%r13
lea    0xd(%rax),%r15
lea    0x8(%rax,%rdx,1),%rdx
mov    %rcx,(%rdx)
mov    %rsi,0x5(%rdx)
movb   $0x0,0x8(%r15,%r13,1)
test   %rbx,%rbx
jle    <open__stdZsyncio_u502+0xee>
lea    0x15(%rax,%r13,1),%rdi
lea    0x8(%rbp),%rsi
mov    %rbx,%rdx
add    %rbx,%r15
call   <memcpy@plt>
movb   $0x0,0x8(%r15,%r13,1)
mov    %r15,0x18(%r12)
mov    $0x323,%r8d
mov    %r14,%rsi
mov    %r12,%rdi
mov    %r13,0x20(%r12)
lea    0x88b8(%rip),%rcx        # <digits100__stdZprivateZdigitsutils_u2+0xf3>
lea    0x88ce(%rip),%rdx        # <digits100__stdZprivateZdigitsutils_u2+0x110>
movq   $0x0,0x8(%r12)
call   <raiseExceptionEx>
jmp    <open__stdZsyncio_u502+0x48>
call   <__stack_chk_fail@plt>
nopw   0x0(%rax,%rax,1)

<opAdd__control_u82>:
endbr64
movzbl %dl,%edx
movzbl %dil,%edi
jmp    <addC__control_u63>

<opSub__control_u88>:
endbr64
add    $0xffffff80,%edx
movzbl %dil,%edi
movzbl %dl,%edx
jmp    <addC__control_u63>
data16 cs nopw 0x0(%rax,%rax,1)
xchg   %ax,%ax

<opMul__control_u106>:
endbr64
push   %r15
push   %r14
mov    %rcx,%r14
push   %r13
mov    %edx,%r13d
push   %r12
and    $0xffffffc0,%r13d
push   %rbp
mov    %rsi,%rbp
push   %rbx
mov    %edi,%ebx
and    $0xffffffc0,%ebx
add    %r13d,%ebx
sub    $0x38,%rsp
mov    (%rcx),%r9
mov    (%rsi),%r15
mov    %fs:0x28,%rax
mov    %rax,0x28(%rsp)
xor    %eax,%eax
movq   $0x0,0x10(%rsp)
lea    (%r15,%r9,1),%rdx
mov    %r9,0x8(%rsp)
lea    0x8(,%rdx,8),%rdi
mov    %rdx,(%rsp)
call   <malloc@plt>
mov    (%rsp),%rdx
mov    0x8(%rsp),%r9
xor    %esi,%esi
lea    0x8(%rax),%rdi
lea    0x8(%r14),%r8
mov    %r15,%rcx
mov    %rax,%r12
mov    %rdx,(%rax)
lea    0x8(%rbp),%rdx
call   <rMac__routines_u92>
mov    %bl,0x10(%rsp)
mov    %rax,(%r12)
mov    0x10(%rsp),%rax
mov    %r12,0x18(%rsp)
mov    0x28(%rsp),%rcx
sub    %fs:0x28,%rcx
jne    <opMul__control_u106+0xb9>
add    $0x38,%rsp
mov    %r12,%rdx
pop    %rbx
pop    %rbp
pop    %r12
pop    %r13
pop    %r14
pop    %r15
ret
call   <__stack_chk_fail@plt>
xchg   %ax,%ax

<opDiv__control_u116>:
endbr64
push   %r15
push   %r14
push   %r13
push   %r12
push   %rbp
mov    %rsi,%rbp
push   %rbx
mov    %rcx,%rbx
sub    $0x438,%rsp
mov    (%rsi),%r15
mov    (%rcx),%r9
mov    %edi,(%rsp)
mov    %edx,0x4(%rsp)
mov    %fs:0x28,%rax
mov    %rax,0x428(%rsp)
xor    %eax,%eax
mov    %r15,%rax
movq   $0x0,0x10(%rsp)
sub    %r9,%rax
mov    %rax,%r13
add    $0x1,%r13
js     <opDiv__control_u116+0x160>
lea    0x10(,%rax,8),%rdi
mov    %r9,0x8(%rsp)
call   <malloc@plt>
mov    0x8(%rsp),%r9
mov    %r13,(%rax)
mov    %rax,%r12
lea    (%r15,%r9,1),%r13
test   %r13,%r13
jle    <opDiv__control_u116+0x170>
cmp    $0x80,%r13
jle    <opDiv__control_u116+0x1d0>
lea    0x0(,%r13,8),%rdi
call   <malloc@plt>
mov    0x8(%rsp),%r9
lea    0x8(%rbp),%rdx
mov    %rax,%r14
lea    0x8(%rbx),%rax
test   %r15,%r15
jg     <opDiv__control_u116+0x18c>
nopl   0x0(%rax)
test   %r9,%r9
jle    <opDiv__control_u116+0xe1>
lea    (%r14,%r15,8),%rsi
lea    (%rax,%r9,8),%rdi
sub    %rbx,%rsi
nopl   0x0(%rax,%rax,1)
mov    (%rax),%rdx
mov    %rdx,-0x8(%rsi,%rax,1)
add    $0x8,%rax
cmp    %rax,%rdi
jne    <opDiv__control_u116+0xd0>
push   %r9
mov    %r15,%r8
mov    %r15,%rcx
xor    %edx,%edx
push   %r15
lea    0x8(%r12),%rdi
mov    %r14,%rsi
call   <rDivCore__routines_u254>
mov    %rax,%r15
pop    %rax
pop    %rdx
cmp    $0x80,%r13
jg     <opDiv__control_u116+0x1c0>
movzbl (%rsp),%ebx
movzbl 0x4(%rsp),%ebp
mov    %r15,(%r12)
mov    %r12,%rdx
mov    %r12,0x18(%rsp)
and    $0xffffffc0,%ebx
and    $0xffffffc0,%ebp
sub    %ebp,%ebx
mov    %bl,0x10(%rsp)
mov    0x10(%rsp),%rax
mov    0x428(%rsp),%rcx
sub    %fs:0x28,%rcx
jne    <opDiv__control_u116+0x1d7>
add    $0x438,%rsp
pop    %rbx
pop    %rbp
pop    %r12
pop    %r13
pop    %r14
pop    %r15
ret
nopl   0x0(%rax,%rax,1)
xor    %r13d,%r13d
mov    $0x8,%edi
jmp    <opDiv__control_u116+0x5f>
nopl   (%rax)
mov    $0x1,%r13d
lea    0x20(%rsp),%r14
lea    0x8(%rbp),%rdx
lea    0x8(%rbx),%rax
test   %r15,%r15
jle    <opDiv__control_u116+0xb8>
mov    %r14,%rdi
lea    (%rdx,%r15,8),%r8
sub    %rbp,%rdi
cs nopw 0x0(%rax,%rax,1)
mov    (%rdx),%rsi
mov    %rsi,-0x8(%rdi,%rdx,1)
add    $0x8,%rdx
cmp    %r8,%rdx
jne    <opDiv__control_u116+0x1a0>
jmp    <opDiv__control_u116+0xb8>
cs nopw 0x0(%rax,%rax,1)
mov    %r14,%rdi
call   <free@plt>
jmp    <opDiv__control_u116+0x10c>
nopl   (%rax)
lea    0x20(%rsp),%r14
jmp    <opDiv__control_u116+0x17b>
call   <__stack_chk_fail@plt>
nopl   0x0(%rax)

<opGcd__control_u186>:
endbr64
push   %r15
mov    %rcx,%r15
push   %r14
push   %r13
push   %r12
push   %rbp
push   %rbx
sub    $0x438,%rsp
mov    (%rsi),%r13
mov    (%rcx),%r14
mov    %fs:0x28,%rax
mov    %rax,0x428(%rsp)
xor    %eax,%eax
movq   $0x0,0x10(%rsp)
test   %r13,%r13
jne    <opGcd__control_u186+0x108>
test   %r14,%r14
jne    <opGcd__control_u186+0x1f0>
lea    0x405d(%rip),%rdi        # <digits100__stdZprivateZdigitsutils_u2+0x240>
call   <specError__routines_u9>
mov    $0x8,%edi
mov    $0x1,%ebp
lea    0x20(%rsp),%r12
call   <malloc@plt>
movq   $0x0,(%rax)
mov    %rax,%rbx
lea    0x8(%r15),%rax
test   %r14,%r14
jle    <opGcd__control_u186+0xa1>
lea    (%r12,%r13,8),%rsi
lea    (%rax,%r14,8),%rdi
sub    %r15,%rsi
nopw   0x0(%rax,%rax,1)
mov    (%rax),%rdx
mov    %rdx,-0x8(%rsi,%rax,1)
add    $0x8,%rax
cmp    %rax,%rdi
jne    <opGcd__control_u186+0x90>
mov    %r13,%r8
mov    %r13,%rcx
lea    0x8(%rbx),%rdi
mov    %r14,%r9
xor    %edx,%edx
mov    %r12,%rsi
call   <rGcdCore__routines_u306>
mov    %rax,%r13
cmp    $0x80,%rbp
jg     <opGcd__control_u186+0x1c0>
mov    %r13,(%rbx)
mov    0x10(%rsp),%rax
mov    %rbx,%rdx
mov    %rbx,0x18(%rsp)
mov    0x428(%rsp),%rcx
sub    %fs:0x28,%rcx
jne    <opGcd__control_u186+0x261>
add    $0x438,%rsp
pop    %rbx
pop    %rbp
pop    %r12
pop    %r13
pop    %r14
pop    %r15
ret
nopl   0x0(%rax)
test   %r14,%r14
je     <opGcd__control_u186+0x228>
cmp    %r14,%r13
jg     <opGcd__control_u186+0x1b0>
mov    %r13,%rbp
mov    %r13,%rax
lea    0x8(,%rax,8),%rdi
mov    %rsi,0x8(%rsp)
call   <malloc@plt>
mov    0x8(%rsp),%rsi
mov    %rbp,(%rax)
lea    0x0(%r13,%r14,1),%rbp
mov    %rax,%rbx
test   %rbp,%rbp
jle    <opGcd__control_u186+0x1e0>
cmp    $0x80,%rbp
jle    <opGcd__control_u186+0x1d0>
lea    0x0(,%rbp,8),%rdi
mov    %rsi,0x8(%rsp)
call   <malloc@plt>
mov    0x8(%rsp),%rsi
mov    %rax,%r12
lea    0x8(%rsi),%rdx
lea    0x8(%r15),%rax
test   %r13,%r13
jle    <opGcd__control_u186+0x7a>
mov    %r12,%rdi
lea    (%rdx,%r13,8),%rcx
sub    %rsi,%rdi
nopl   0x0(%rax)
mov    (%rdx),%rsi
mov    %rsi,-0x8(%rdi,%rdx,1)
add    $0x8,%rdx
cmp    %rdx,%rcx
jne    <opGcd__control_u186+0x190>
jmp    <opGcd__control_u186+0x7a>
cs nopw 0x0(%rax,%rax,1)
mov    %r14,%rbp
mov    %r14,%rax
jmp    <opGcd__control_u186+0x120>
nopl   0x0(%rax,%rax,1)
mov    %r12,%rdi
call   <free@plt>
jmp    <opGcd__control_u186+0xc8>
nopl   (%rax)
lea    0x20(%rsp),%r12
jmp    <opGcd__control_u186+0x16e>
nopw   0x0(%rax,%rax,1)
mov    $0x1,%ebp
lea    0x20(%rsp),%r12
jmp    <opGcd__control_u186+0x16e>
nopl   0x0(%rax)
jle    <opGcd__control_u186+0x58>
lea    0x8(,%r14,8),%rdi
mov    %rsi,0x8(%rsp)
call   <malloc@plt>
cmp    $0x80,%r14
mov    0x8(%rsp),%rsi
mov    %r14,(%rax)
mov    %rax,%rbx
jle    <opGcd__control_u186+0x250>
mov    %r14,%rbp
jmp    <opGcd__control_u186+0x154>
nopl   0x0(%rax)
test   %r13,%r13
jns    <opGcd__control_u186+0x266>
mov    $0x8,%edi
mov    $0x1,%ebp
lea    0x20(%rsp),%r12
call   <malloc@plt>
movq   $0x0,(%rax)
mov    %rax,%rbx
jmp    <opGcd__control_u186+0xa1>
lea    0x8(%r15),%rax
mov    %r14,%rbp
lea    0x20(%rsp),%r12
jmp    <opGcd__control_u186+0x7f>
call   <__stack_chk_fail@plt>
lea    0x8(,%r13,8),%rdi
mov    %rsi,0x8(%rsp)
mov    %r13,%rbp
call   <malloc@plt>
mov    0x8(%rsp),%rsi
mov    %r13,(%rax)
mov    %rax,%rbx
jmp    <opGcd__control_u186+0x14b>
nopl   0x0(%rax,%rax,1)

<opNeg__control_u94>:
endbr64
push   %r12
push   %rbp
mov    %rsi,%rbp
push   %rbx
mov    %edi,%ebx
sub    $0x20,%rsp
mov    (%rsi),%r12
mov    %fs:0x28,%rax
mov    %rax,0x18(%rsp)
xor    %eax,%eax
movq   $0x0,(%rsp)
lea    0x8(,%r12,8),%rdi
call   <malloc@plt>
mov    %r12,(%rax)
test   %r12,%r12
jle    <opNeg__control_u94+0x60>
lea    0x8(%rbp),%rdx
mov    %rax,%rdi
lea    (%rdx,%r12,8),%r8
sub    %rbp,%rdi
nop
mov    (%rdx),%rcx
mov    %rcx,(%rdi,%rdx,1)
add    $0x8,%rdx
cmp    %r8,%rdx
jne    <opNeg__control_u94+0x50>
add    $0xffffff80,%ebx
mov    %rax,0x8(%rsp)
mov    0x8(%rsp),%rdx
mov    %bl,(%rsp)
mov    (%rsp),%rax
mov    0x18(%rsp),%rcx
sub    %fs:0x28,%rcx
jne    <opNeg__control_u94+0x8d>
add    $0x20,%rsp
pop    %rbx
pop    %rbp
pop    %r12
ret
call   <__stack_chk_fail@plt>
data16 cs nopw 0x0(%rax,%rax,1)
nopl   (%rax)

<opCmp__control_u224>:
endbr64
push   %r13
shr    $0x6,%dl
push   %r12
sub    $0x28,%rsp
mov    (%rsi),%r12
mov    (%rcx),%rax
test   %r12,%r12
jne    <opCmp__control_u224+0x30>
test   %rax,%rax
jne    <opCmp__control_u224+0xa0>
add    $0x28,%rsp
pop    %r12
pop    %r13
ret
nopl   0x0(%rax)
shr    $0x6,%dil
mov    %edi,%r13d
and    $0x2,%r13d
test   %rax,%rax
je     <opCmp__control_u224+0xb8>
mov    %edi,%r8d
xor    %edx,%r8d
cmp    $0x2,%r8b
je     <opCmp__control_u224+0xb8>
cmp    %dl,%dil
je     <opCmp__control_u224+0x7b>
lea    0x3d32(%rip),%rdi        # <digits100__stdZprivateZdigitsutils_u2+0x24a>
mov    %rcx,0x18(%rsp)
mov    %rsi,0x10(%rsp)
mov    %rax,0x8(%rsp)
call   <specError__routines_u9>
mov    0x18(%rsp),%rcx
mov    0x10(%rsp),%rsi
mov    0x8(%rsp),%rax
lea    0x8(%rcx),%rdx
lea    0x8(%rsi),%rdi
mov    %rax,%rcx
mov    %r12,%rsi
call   <rCmp__routines_u197>
test   %r13b,%r13b
je     <opCmp__control_u224+0x23>
neg    %rax
jmp    <opCmp__control_u224+0x23>
nopl   0x0(%rax,%rax,1)
and    $0x2,%edx
je     <opCmp__control_u224+0xbd>
add    $0x28,%rsp
mov    $0x1,%eax
pop    %r12
pop    %r13
ret
nopl   0x0(%rax,%rax,1)
test   %r13b,%r13b
je     <opCmp__control_u224+0xa5>
add    $0x28,%rsp
mov    $0xffffffffffffffff,%rax
pop    %r12
pop    %r13
ret
nopl   (%rax)

<loop_add>:
endbr64
push   %r13
push   %r12
push   %rbp
push   %rbx
sub    $0x8,%rsp
mov    0x8703(%rip),%rbp        # <gN__bench_u26>
test   %rdi,%rdi
jle    <loop_add+0xa0>
mov    %rdi,%r13
test   %rbp,%rbp
jle    <loop_add+0xab>
xor    %r12d,%r12d
nopl   (%rax)
xor    %ebx,%ebx
nopw   0x0(%rax,%rax,1)
mov    0x8701(%rip),%rax        # <gRW__bench_u21>
mov    (%rax,%rbx,8),%rdi
call   <free@plt>
mov    0x86f9(%rip),%rax        # <gBW__bench_u20>
mov    (%rax,%rbx,8),%rcx
mov    0x8706(%rip),%rax        # <gBH__bench_u17>
movzbl (%rax,%rbx,1),%edx
mov    0x86eb(%rip),%rax        # <gAW__bench_u19>
mov    (%rax,%rbx,8),%rsi
mov    0x86f8(%rip),%rax        # <gAH__bench_u16>
movzbl (%rax,%rbx,1),%edi
call   <opAdd__control_u82>
mov    0x86d8(%rip),%rcx        # <gRH__bench_u18>
mov    %al,(%rcx,%rbx,1)
mov    0x86b6(%rip),%rax        # <gRW__bench_u21>
mov    %rdx,(%rax,%rbx,8)
add    $0x1,%rbx
cmp    %rbx,%rbp
jne    <loop_add+0x38>
add    $0x1,%r12
cmp    %r12,%r13
jne    <loop_add+0x30>
add    $0x8,%rsp
pop    %rbx
pop    %rbp
pop    %r12
pop    %r13
ret
xor    %eax,%eax
test   $0x1,%dil
je     <loop_add+0xbe>
mov    $0x1,%eax
cmp    $0x1,%rdi
je     <loop_add+0xa0>
add    $0x2,%rax
cmp    %rax,%r13
je     <loop_add+0xa0>
add    $0x2,%rax
cmp    %rax,%r13
jne    <loop_add+0xbe>
jmp    <loop_add+0xa0>
data16 cs nopw 0x0(%rax,%rax,1)
nopl   (%rax)

<loop_sub>:
endbr64
push   %r13
push   %r12
push   %rbp
push   %rbx
sub    $0x8,%rsp
mov    0x8623(%rip),%rbp        # <gN__bench_u26>
test   %rdi,%rdi
jle    <loop_sub+0xa0>
mov    %rdi,%r13
test   %rbp,%rbp
jle    <loop_sub+0xab>
xor    %r12d,%r12d
nopl   (%rax)
xor    %ebx,%ebx
nopw   0x0(%rax,%rax,1)
mov    0x8621(%rip),%rax        # <gRW__bench_u21>
mov    (%rax,%rbx,8),%rdi
call   <free@plt>
mov    0x8619(%rip),%rax        # <gBW__bench_u20>
mov    (%rax,%rbx,8),%rcx
mov    0x8626(%rip),%rax        # <gBH__bench_u17>
movzbl (%rax,%rbx,1),%edx
mov    0x860b(%rip),%rax        # <gAW__bench_u19>
mov    (%rax,%rbx,8),%rsi
mov    0x8618(%rip),%rax        # <gAH__bench_u16>
movzbl (%rax,%rbx,1),%edi
call   <opSub__control_u88>
mov    0x85f8(%rip),%rcx        # <gRH__bench_u18>
mov    %al,(%rcx,%rbx,1)
mov    0x85d6(%rip),%rax        # <gRW__bench_u21>
mov    %rdx,(%rax,%rbx,8)
add    $0x1,%rbx
cmp    %rbx,%rbp
jne    <loop_sub+0x38>
add    $0x1,%r12
cmp    %r12,%r13
jne    <loop_sub+0x30>
add    $0x8,%rsp
pop    %rbx
pop    %rbp
pop    %r12
pop    %r13
ret
xor    %eax,%eax
test   $0x1,%dil
je     <loop_sub+0xbe>
mov    $0x1,%eax
cmp    $0x1,%rdi
je     <loop_sub+0xa0>
add    $0x2,%rax
cmp    %rax,%r13
je     <loop_sub+0xa0>
add    $0x2,%rax
cmp    %rax,%r13
jne    <loop_sub+0xbe>
jmp    <loop_sub+0xa0>
data16 cs nopw 0x0(%rax,%rax,1)
nopl   (%rax)

<loop_mul>:
endbr64
push   %r13
push   %r12
push   %rbp
push   %rbx
sub    $0x8,%rsp
mov    0x8543(%rip),%rbp        # <gN__bench_u26>
test   %rdi,%rdi
jle    <loop_mul+0xa0>
mov    %rdi,%r13
test   %rbp,%rbp
jle    <loop_mul+0xab>
xor    %r12d,%r12d
nopl   (%rax)
xor    %ebx,%ebx
nopw   0x0(%rax,%rax,1)
mov    0x8541(%rip),%rax        # <gRW__bench_u21>
mov    (%rax,%rbx,8),%rdi
call   <free@plt>
mov    0x8539(%rip),%rax        # <gBW__bench_u20>
mov    (%rax,%rbx,8),%rcx
mov    0x8546(%rip),%rax        # <gBH__bench_u17>
movzbl (%rax,%rbx,1),%edx
mov    0x852b(%rip),%rax        # <gAW__bench_u19>
mov    (%rax,%rbx,8),%rsi
mov    0x8538(%rip),%rax        # <gAH__bench_u16>
movzbl (%rax,%rbx,1),%edi
call   <opMul__control_u106>
mov    0x8518(%rip),%rcx        # <gRH__bench_u18>
mov    %al,(%rcx,%rbx,1)
mov    0x84f6(%rip),%rax        # <gRW__bench_u21>
mov    %rdx,(%rax,%rbx,8)
add    $0x1,%rbx
cmp    %rbx,%rbp
jne    <loop_mul+0x38>
add    $0x1,%r12
cmp    %r12,%r13
jne    <loop_mul+0x30>
add    $0x8,%rsp
pop    %rbx
pop    %rbp
pop    %r12
pop    %r13
ret
xor    %eax,%eax
test   $0x1,%dil
je     <loop_mul+0xbe>
mov    $0x1,%eax
cmp    $0x1,%rdi
je     <loop_mul+0xa0>
add    $0x2,%rax
cmp    %rax,%r13
je     <loop_mul+0xa0>
add    $0x2,%rax
cmp    %rax,%r13
jne    <loop_mul+0xbe>
jmp    <loop_mul+0xa0>
data16 cs nopw 0x0(%rax,%rax,1)
nopl   (%rax)

<loop_growth>:
endbr64
push   %r15
push   %r14
push   %r13
push   %r12
push   %rbp
push   %rbx
sub    $0x28,%rsp
mov    %rdi,0x18(%rsp)
test   %rdi,%rdi
jle    <loop_growth+0x15f>
movq   $0x0,0x10(%rsp)
nopl   0x0(%rax)
cmpq   $0x0,0x8438(%rip)        # <gChains__bench_u201>
jle    <loop_growth+0x149>
movq   $0x0,0x8(%rsp)
nopw   0x0(%rax,%rax,1)
mov    0x8(%rsp),%rbx
mov    0x847c(%rip),%rax        # <gRes__bench_u200+0x8>
shl    $0x4,%rbx
add    %rbx,%rax
mov    0x10(%rax),%rdi
call   <free@plt>
mov    0x8495(%rip),%rcx        # <gX0__bench_u197+0x8>
add    %rbx,%rcx
mov    0x83eb(%rip),%rbx        # <gSteps__bench_u202>
mov    0x8(%rcx),%r14
movzbl 0x8(%rcx),%r15d
mov    0x10(%rcx),%r13
test   %rbx,%rbx
jle    <loop_growth+0x115>
xor    %ebp,%ebp
nopl   (%rax)
mov    0x8(%rsp),%rax
movzbl %r15b,%edi
mov    %r13,%rsi
imul   %rax,%rbx
mov    0x8449(%rip),%rax        # <gY__bench_u198+0x8>
add    %rbp,%rbx
shl    $0x4,%rbx
add    %rbx,%rax
movzbl 0x8(%rax),%edx
mov    0x10(%rax),%rcx
call   <opMul__control_u106>
mov    %rdx,%r12
mov    %rax,%r14
test   %rbp,%rbp
je     <loop_growth+0xd9>
mov    %r13,%rdi
call   <free@plt>
add    0x8408(%rip),%rbx        # <gZ__bench_u199+0x8>
movzbl %r14b,%edi
mov    %r12,%rsi
add    $0x1,%rbp
movzbl 0x8(%rbx),%edx
mov    0x10(%rbx),%rcx
call   <opAdd__control_u82>
mov    %r12,%rdi
mov    %rax,%r14
mov    %eax,%r15d
mov    %rdx,%r13
call   <free@plt>
mov    0x8358(%rip),%rbx        # <gSteps__bench_u202>
cmp    %rbp,%rbx
jg     <loop_growth+0x98>
mov    0x8(%rsp),%rax
mov    %r15b,%r14b
mov    %rax,%rdx
add    $0x1,%rax
shl    $0x4,%rdx
add    0x83a9(%rip),%rdx        # <gRes__bench_u200+0x8>
mov    %rax,0x8(%rsp)
cmp    %rax,0x8335(%rip)        # <gChains__bench_u201>
mov    %r14,0x8(%rdx)
mov    %r13,0x10(%rdx)
jg     <loop_growth+0x50>
addq   $0x1,0x10(%rsp)
mov    0x10(%rsp),%rax
cmp    %rax,0x18(%rsp)
jne    <loop_growth+0x30>
add    $0x28,%rsp
pop    %rbx
pop    %rbp
pop    %r12
pop    %r13
pop    %r14
pop    %r15
ret
xchg   %ax,%ax

<loop_div>:
endbr64
push   %r13
push   %r12
push   %rbp
push   %rbx
sub    $0x8,%rsp
mov    0x82f3(%rip),%rbp        # <gN__bench_u26>
test   %rdi,%rdi
jle    <loop_div+0xa0>
mov    %rdi,%r13
test   %rbp,%rbp
jle    <loop_div+0xab>
xor    %r12d,%r12d
nopl   (%rax)
xor    %ebx,%ebx
nopw   0x0(%rax,%rax,1)
mov    0x82f1(%rip),%rax        # <gRW__bench_u21>
mov    (%rax,%rbx,8),%rdi
call   <free@plt>
mov    0x82e9(%rip),%rax        # <gBW__bench_u20>
mov    (%rax,%rbx,8),%rcx
mov    0x82f6(%rip),%rax        # <gBH__bench_u17>
movzbl (%rax,%rbx,1),%edx
mov    0x82db(%rip),%rax        # <gAW__bench_u19>
mov    (%rax,%rbx,8),%rsi
mov    0x82e8(%rip),%rax        # <gAH__bench_u16>
movzbl (%rax,%rbx,1),%edi
call   <opDiv__control_u116>
mov    0x82c8(%rip),%rcx        # <gRH__bench_u18>
mov    %al,(%rcx,%rbx,1)
mov    0x82a6(%rip),%rax        # <gRW__bench_u21>
mov    %rdx,(%rax,%rbx,8)
add    $0x1,%rbx
cmp    %rbx,%rbp
jne    <loop_div+0x38>
add    $0x1,%r12
cmp    %r12,%r13
jne    <loop_div+0x30>
add    $0x8,%rsp
pop    %rbx
pop    %rbp
pop    %r12
pop    %r13
ret
xor    %eax,%eax
test   $0x1,%dil
je     <loop_div+0xbe>
mov    $0x1,%eax
cmp    $0x1,%rdi
je     <loop_div+0xa0>
add    $0x2,%rax
cmp    %rax,%r13
je     <loop_div+0xa0>
add    $0x2,%rax
cmp    %rax,%r13
jne    <loop_div+0xbe>
jmp    <loop_div+0xa0>
data16 cs nopw 0x0(%rax,%rax,1)
nopl   (%rax)

<loop_gcd>:
endbr64
push   %r13
push   %r12
push   %rbp
push   %rbx
sub    $0x8,%rsp
mov    0x8213(%rip),%rbp        # <gN__bench_u26>
test   %rdi,%rdi
jle    <loop_gcd+0xa0>
mov    %rdi,%r13
test   %rbp,%rbp
jle    <loop_gcd+0xab>
xor    %r12d,%r12d
nopl   (%rax)
xor    %ebx,%ebx
nopw   0x0(%rax,%rax,1)
mov    0x8211(%rip),%rax        # <gRW__bench_u21>
mov    (%rax,%rbx,8),%rdi
call   <free@plt>
mov    0x8209(%rip),%rax        # <gBW__bench_u20>
mov    (%rax,%rbx,8),%rcx
mov    0x8216(%rip),%rax        # <gBH__bench_u17>
movzbl (%rax,%rbx,1),%edx
mov    0x81fb(%rip),%rax        # <gAW__bench_u19>
mov    (%rax,%rbx,8),%rsi
mov    0x8208(%rip),%rax        # <gAH__bench_u16>
movzbl (%rax,%rbx,1),%edi
call   <opGcd__control_u186>
mov    0x81e8(%rip),%rcx        # <gRH__bench_u18>
mov    %al,(%rcx,%rbx,1)
mov    0x81c6(%rip),%rax        # <gRW__bench_u21>
mov    %rdx,(%rax,%rbx,8)
add    $0x1,%rbx
cmp    %rbx,%rbp
jne    <loop_gcd+0x38>
add    $0x1,%r12
cmp    %r12,%r13
jne    <loop_gcd+0x30>
add    $0x8,%rsp
pop    %rbx
pop    %rbp
pop    %r12
pop    %r13
ret
xor    %eax,%eax
test   $0x1,%dil
je     <loop_gcd+0xbe>
mov    $0x1,%eax
cmp    $0x1,%rdi
je     <loop_gcd+0xa0>
add    $0x2,%rax
cmp    %rax,%r13
je     <loop_gcd+0xa0>
add    $0x2,%rax
cmp    %rax,%r13
jne    <loop_gcd+0xbe>
jmp    <loop_gcd+0xa0>
data16 cs nopw 0x0(%rax,%rax,1)
nopl   (%rax)

<loop_neg>:
endbr64
push   %r13
push   %r12
push   %rbp
push   %rbx
sub    $0x8,%rsp
mov    0x8133(%rip),%rbp        # <gN__bench_u26>
test   %rdi,%rdi
jle    <loop_neg+0x82>
mov    %rdi,%r13
test   %rbp,%rbp
jle    <loop_neg+0x8d>
xor    %r12d,%r12d
nopl   (%rax)
xor    %ebx,%ebx
nopw   0x0(%rax,%rax,1)
mov    0x8139(%rip),%rax        # <gRW__bench_u21>
mov    (%rax,%rbx,8),%rdi
call   <free@plt>
mov    0x8139(%rip),%rax        # <gAW__bench_u19>
mov    (%rax,%rbx,8),%rsi
mov    0x8146(%rip),%rax        # <gAH__bench_u16>
movzbl (%rax,%rbx,1),%edi
call   <opNeg__control_u94>
mov    0x8126(%rip),%rcx        # <gRH__bench_u18>
mov    %al,(%rcx,%rbx,1)
mov    0x8104(%rip),%rax        # <gRW__bench_u21>
mov    %rdx,(%rax,%rbx,8)
add    $0x1,%rbx
cmp    %rbx,%rbp
jne    <loop_neg+0x30>
add    $0x1,%r12
cmp    %r12,%r13
jne    <loop_neg+0x28>
add    $0x8,%rsp
pop    %rbx
pop    %rbp
pop    %r12
pop    %r13
ret
xor    %eax,%eax
test   $0x1,%dil
je     <loop_neg+0xa0>
mov    $0x1,%eax
cmp    $0x1,%rdi
je     <loop_neg+0x82>
add    $0x2,%rax
cmp    %rax,%r13
je     <loop_neg+0x82>
add    $0x2,%rax
cmp    %rax,%r13
jne    <loop_neg+0xa0>
jmp    <loop_neg+0x82>
data16 cs nopw 0x0(%rax,%rax,1)
nop

<loop_cmp>:
endbr64
push   %r13
push   %r12
push   %rbp
push   %rbx
sub    $0x8,%rsp
mov    0x8073(%rip),%rbp        # <gN__bench_u26>
test   %rdi,%rdi
jle    <loop_cmp+0x7d>
mov    %rdi,%r13
test   %rbp,%rbp
jle    <loop_cmp+0x88>
xor    %r12d,%r12d
nopl   (%rax)
xor    %ebx,%ebx
nopw   0x0(%rax,%rax,1)
mov    0x8081(%rip),%rax        # <gBW__bench_u20>
mov    (%rax,%rbx,8),%rcx
mov    0x808e(%rip),%rax        # <gBH__bench_u17>
movzbl (%rax,%rbx,1),%edx
mov    0x8073(%rip),%rax        # <gAW__bench_u19>
mov    (%rax,%rbx,8),%rsi
mov    0x8080(%rip),%rax        # <gAH__bench_u16>
movzbl (%rax,%rbx,1),%edi
call   <opCmp__control_u224>
mov    0x8028(%rip),%rdx        # <gRC__bench_u25>
mov    %al,(%rdx,%rbx,1)
add    $0x1,%rbx
cmp    %rbx,%rbp
jne    <loop_cmp+0x30>
add    $0x1,%r12
cmp    %r13,%r12
jne    <loop_cmp+0x28>
add    $0x8,%rsp
pop    %rbx
pop    %rbp
pop    %r12
pop    %r13
ret
xor    %eax,%eax
test   $0x1,%dil
je     <loop_cmp+0x9b>
mov    $0x1,%eax
cmp    $0x1,%rdi
je     <loop_cmp+0x7d>
add    $0x2,%rax
cmp    %rax,%r13
je     <loop_cmp+0x7d>
add    $0x2,%rax
cmp    %rax,%r13
jne    <loop_cmp+0x9b>
jmp    <loop_cmp+0x7d>
nop

