# division.s: the supplement's appendix, as one file that assembles and is tested by test.c.
# x86-64, Intel syntax. Each pair routine divides a and a2 by b: a in rdi, a2 in rsi, b in rdx; it writes the two
# quotients to the memory at rcx (q first, then q2). long_division takes a in rdi and b in rsi and returns q in rax.
    .intel_syntax noprefix
    .text

# ---- the long division inside DIVIDE, written out as instructions (one quotient bit per step)
    .globl long_division
long_division:
    xor     eax, eax            # q <- 0
    xor     edx, edx            # r <- 0
    mov     ecx, 64             # 64 steps
1:  shl     rdi, 1              # top bit of a goes to the carry flag; a <- 2a
    rcl     rdx, 1              # r <- 2r + that bit; the bit pushed out of r goes to the carry flag
    jc      2f                  # r went past 2^64, so r >= b: subtract
    shl     rax, 1              # q <- 2q
    cmp     rdx, rsi            # r < b ?
    jb      3f                  # yes: this quotient bit is 0
    or      rax, 1              # q <- q + 1
    sub     rdx, rsi            # r <- r - b
    jmp     3f
2:  shl     rax, 1              # q <- 2q
    or      rax, 1              # q <- q + 1
    sub     rdx, rsi            # r <- r - b (mod 2^64, right since r < 2b)
3:  dec     ecx                 # one step fewer
    jnz     1b                  # until no steps remain
    ret

# ---- control: two divides
    .globl pair_control
pair_control:
    mov     r8, rdx             # b
    mov     rax, rdi            # low word of the dividend: a
    xor     edx, edx            # high word: 0
    div     r8                  # q in rax (remainder in rdx)
    mov     [rcx], rax
    mov     rax, rsi            # a2
    xor     edx, edx
    div     r8
    mov     [rcx+8], rax
    ret

# ---- the 32-bit divide when a, a2 and b are all below 2^32, otherwise control
    .globl pair_32
pair_32:
    mov     r8, rdx             # b
    mov     rax, rdi
    or      rax, rsi
    or      rax, r8             # a OR a2 OR b
    shr     rax, 32             # any bit above the low 32?
    jnz     4f                  # yes: the 64-bit divide
    mov     eax, edi            # a, as 32 bits
    xor     edx, edx
    div     r8d                 # 32-bit divide: q in eax
    mov     [rcx], rax
    mov     eax, esi
    xor     edx, edx
    div     r8d
    mov     [rcx+8], rax
    ret
4:  mov     rax, rdi
    xor     edx, edx
    div     r8
    mov     [rcx], rax
    mov     rax, rsi
    xor     edx, edx
    div     r8
    mov     [rcx+8], rax
    ret

# ---- one Newton step: x <- x * (2 - o*x), o in r8, x in rax; uses r9
    .macro newton
    mov     r9, r8
    imul    r9, rax             # o*x
    neg     r9
    add     r9, 2               # 2 - o*x
    imul    rax, r9             # x * (2 - o*x)
    .endm

# ---- reuse: one inverse (5-bit start, 4 Newton steps), used for both dividends
    .globl pair_reuse
pair_reuse:
    mov     r10, rcx            # where the quotients go (cl is needed for shifts)
    tzcnt   rcx, rdx            # s <- trailing zeros of b
    mov     r8, rdx
    shr     r8, cl              # o <- b >> s, odd
    lea     rax, [r8+r8*2]      # 3o
    xor     rax, 2              # x <- 3o XOR 2: right in the low 5 bits
    newton                      # 10 bits
    newton                      # 20 bits
    newton                      # 40 bits
    newton                      # 64 bits
    shr     rdi, cl             # a >> s
    imul    rdi, rax            # q <- (a >> s) * x
    mov     [r10], rdi
    shr     rsi, cl             # a2 >> s
    imul    rsi, rax            # q2 <- (a2 >> s) * x
    mov     [r10+8], rsi
    ret

# ---- the inverse from the table: b in rdx; x in rax, s in cl; uses r8, r9, r11
    .macro table_inverse
    tzcnt   rcx, rdx            # s <- trailing zeros of b
    mov     r8, rdx
    shr     r8, cl              # o <- b >> s, odd
    mov     r9, r8
    shr     r9, 1
    and     r9, 127             # index: (o >> 1) AND 127
    lea     r11, [rip+inv_table]
    movzx   eax, byte ptr [r11+r9]  # x <- TABLE[index]: right in the low 8 bits
    newton                      # 16 bits
    newton                      # 32 bits
    newton                      # 64 bits
    .endm

# ---- table: the inverse from the table, worked out again for each dividend
    .globl pair_table
pair_table:
    mov     r10, rcx
    table_inverse
    shr     rdi, cl
    imul    rdi, rax            # q
    mov     [r10], rdi
    table_inverse               # the same steps again, for the second dividend
    shr     rsi, cl
    imul    rsi, rax            # q2
    mov     [r10+8], rsi
    ret

# ---- table and reuse: the inverse from the table once, used for both dividends
    .globl pair_table_reuse
pair_table_reuse:
    mov     r10, rcx
    table_inverse
    shr     rdi, cl
    imul    rdi, rax            # q
    mov     [r10], rdi
    shr     rsi, cl
    imul    rsi, rax            # q2
    mov     [r10+8], rsi
    ret

# ---- the table: entry i is the inverse of 2i + 1 mod 256 (128 bytes, read-only, made with the program)
    .section .rodata
inv_table:
    .byte 0x01, 0xab, 0xcd, 0xb7, 0x39, 0xa3, 0xc5, 0xef, 0xf1, 0x1b, 0x3d, 0xa7, 0x29, 0x13, 0x35, 0xdf
    .byte 0xe1, 0x8b, 0xad, 0x97, 0x19, 0x83, 0xa5, 0xcf, 0xd1, 0xfb, 0x1d, 0x87, 0x09, 0xf3, 0x15, 0xbf
    .byte 0xc1, 0x6b, 0x8d, 0x77, 0xf9, 0x63, 0x85, 0xaf, 0xb1, 0xdb, 0xfd, 0x67, 0xe9, 0xd3, 0xf5, 0x9f
    .byte 0xa1, 0x4b, 0x6d, 0x57, 0xd9, 0x43, 0x65, 0x8f, 0x91, 0xbb, 0xdd, 0x47, 0xc9, 0xb3, 0xd5, 0x7f
    .byte 0x81, 0x2b, 0x4d, 0x37, 0xb9, 0x23, 0x45, 0x6f, 0x71, 0x9b, 0xbd, 0x27, 0xa9, 0x93, 0xb5, 0x5f
    .byte 0x61, 0x0b, 0x2d, 0x17, 0x99, 0x03, 0x25, 0x4f, 0x51, 0x7b, 0x9d, 0x07, 0x89, 0x73, 0x95, 0x3f
    .byte 0x41, 0xeb, 0x0d, 0xf7, 0x79, 0xe3, 0x05, 0x2f, 0x31, 0x5b, 0x7d, 0xe7, 0x69, 0x53, 0x75, 0x1f
    .byte 0x21, 0xcb, 0xed, 0xd7, 0x59, 0xc3, 0xe5, 0x0f, 0x11, 0x3b, 0x5d, 0xc7, 0x49, 0x33, 0x55, 0xff
    .section .note.GNU-stack,"",@progbits
