# GNU C version 4.0.1 (Apple Inc. build 5465) (i686-apple-darwin9)
#	compiled by GNU C version 4.0.1 (Apple Inc. build 5465).
# GGC heuristics: --param ggc-min-expand=100 --param ggc-min-heapsize=131072
# options passed:  -D__DYNAMIC__ -fPIC -mmacosx-version-min=10.5.1 -m64
# -mtune=generic -march=apple -auxbase -fverbose-asm
# options enabled:  -fPIC -falign-jumps-max-skip -falign-loops
# -falign-loops-max-skip -fargument-alias -fasynchronous-unwind-tables
# -fbranch-count-reg -fcommon -feliminate-unused-debug-types -ffunction-cse
# -fgcse-lm -fident -fivopts -fkeep-static-consts -fleading-underscore
# -flocal-alloc -floop-optimize2 -fpeephole -freg-struct-return
# -fsched-interblock -fsched-spec -fsched-stalled-insns-dep
# -fsplit-ivs-in-unroller -ftree-loop-im -ftree-loop-ivcanon
# -ftree-loop-optimize -funwind-tables -fverbose-asm
# -fzero-initialized-in-bss -m80387 -mhard-float -mno-soft-float -mieee-fp
# -mfp-ret-in-387 -maccumulate-outgoing-args -mmmx -msse -msse2 -msse3
# -m128bit-long-double -m64 -mtune=generic64 -march=apple
# -mmacosx-version-min=10.5.1

# Compiler executable checksum: 0a7d9e41e786877ed5cbeb90e063cdab

	.data
	.align 2
_iSeeDelta.5028:
	.long	-17
	.long	15
	.align 2
_pPawn.5027:
	.long	2
	.long	3
	.text
.globl _SlowGetAttacks
_SlowGetAttacks:
LFB33:
	pushq	%rbp	#
LCFI0:
	movq	%rsp, %rbp	#,
LCFI1:
	movq	%rdi, -40(%rbp)	# pList, pList
	movq	%rsi, -48(%rbp)	# pos, pos
	movl	%edx, -52(%rbp)	# cSquare, cSquare
	movl	%ecx, -56(%rbp)	# uSide, uSide
	movq	-40(%rbp), %rax	# pList, pList
	movl	$0, (%rax)	#, <variable>.uCount
	movl	-56(%rbp), %eax	# uSide, uSide.5
	mov	%eax, %eax	# uSide.5, uSide.5
	leaq	0(,%rax,4), %rdx	#, tmp125
	leaq	_iSeeDelta.5028(%rip), %rax	#, tmp126
	movl	(%rdx,%rax), %eax	# iSeeDelta, D.5038
	addl	-52(%rbp), %eax	# cSquare, tmp127
	movl	%eax, -16(%rbp)	# tmp127, c
	movl	-16(%rbp), %eax	# c, D.5040
	andl	$136, %eax	#, D.5040
	testl	%eax, %eax	# D.5040
	jne	L2	#,
	movl	-16(%rbp), %eax	# c, c.6
	movq	-48(%rbp), %rdx	# pos, pos
	mov	%eax, %eax	# c.6, c.6
	movl	(%rdx,%rax,8), %eax	# <variable>.D.2182.pPiece, <variable>.D.2182.pPiece
	movl	%eax, -20(%rbp)	# <variable>.D.2182.pPiece, p
	movl	-56(%rbp), %eax	# uSide, uSide.7
	mov	%eax, %eax	# uSide.7, uSide.7
	leaq	0(,%rax,4), %rdx	#, tmp132
	leaq	_pPawn.5027(%rip), %rax	#, tmp133
	movl	(%rdx,%rax), %eax	# pPawn, D.5043
	cmpl	-20(%rbp), %eax	# p, D.5043
	jne	L2	#,
	movq	-40(%rbp), %rdx	# pList, pList
	movl	-20(%rbp), %eax	# p, p
	movl	%eax, 4(%rdx)	# p, <variable>.pPiece
	movq	-40(%rbp), %rdx	# pList, pList
	movl	-16(%rbp), %eax	# c, c
	movl	%eax, 8(%rdx)	# c, <variable>.cLoc
	movq	-40(%rbp), %rax	# pList, pList
	movl	$100, 12(%rax)	#, <variable>.uVal
	movq	-40(%rbp), %rax	# pList, pList
	movl	$1, (%rax)	#, <variable>.uCount
L2:
	leaq	-16(%rbp), %rax	#, tmp258
	addl	$2, (%rax)	#, c
	movl	-16(%rbp), %eax	# c, D.5044
	andl	$136, %eax	#, D.5044
	testl	%eax, %eax	# D.5044
	jne	L5	#,
	movl	-16(%rbp), %eax	# c, c.8
	movq	-48(%rbp), %rdx	# pos, pos
	mov	%eax, %eax	# c.8, c.8
	movl	(%rdx,%rax,8), %eax	# <variable>.D.2182.pPiece, <variable>.D.2182.pPiece
	movl	%eax, -20(%rbp)	# <variable>.D.2182.pPiece, p
	movl	-56(%rbp), %eax	# uSide, uSide.9
	mov	%eax, %eax	# uSide.9, uSide.9
	leaq	0(,%rax,4), %rdx	#, tmp144
	leaq	_pPawn.5027(%rip), %rax	#, tmp145
	movl	(%rdx,%rax), %eax	# pPawn, D.5047
	cmpl	-20(%rbp), %eax	# p, D.5047
	jne	L5	#,
	movq	-40(%rbp), %rax	# pList, pList
	movl	(%rax), %eax	# <variable>.uCount, D.5048
	movq	-40(%rbp), %rcx	# pList, pList
	mov	%eax, %edx	# D.5048, D.5048
	movq	%rdx, %rax	# D.5048, D.5048
	addq	%rax, %rax	# D.5048
	addq	%rdx, %rax	# D.5048, D.5048
	salq	$2, %rax	#, tmp150
	addq	%rcx, %rax	# pList, tmp151
	leaq	4(%rax), %rdx	#, tmp152
	movl	-20(%rbp), %eax	# p, p
	movl	%eax, (%rdx)	# p, <variable>.pPiece
	movq	-40(%rbp), %rax	# pList, pList
	movl	(%rax), %eax	# <variable>.uCount, D.5049
	movq	-40(%rbp), %rcx	# pList, pList
	mov	%eax, %edx	# D.5049, D.5049
	movq	%rdx, %rax	# D.5049, D.5049
	addq	%rax, %rax	# D.5049
	addq	%rdx, %rax	# D.5049, D.5049
	salq	$2, %rax	#, tmp158
	addq	%rcx, %rax	# pList, tmp159
	leaq	8(%rax), %rdx	#, tmp160
	movl	-16(%rbp), %eax	# c, c
	movl	%eax, (%rdx)	# c, <variable>.cLoc
	movq	-40(%rbp), %rax	# pList, pList
	movl	(%rax), %eax	# <variable>.uCount, D.5050
	movq	-40(%rbp), %rcx	# pList, pList
	mov	%eax, %edx	# D.5050, D.5050
	movq	%rdx, %rax	# D.5050, D.5050
	addq	%rax, %rax	# D.5050
	addq	%rdx, %rax	# D.5050, D.5050
	salq	$2, %rax	#, tmp166
	addq	%rcx, %rax	# pList, tmp167
	addq	$12, %rax	#, tmp168
	movl	$100, (%rax)	#, <variable>.uVal
	movq	-40(%rbp), %rax	# pList, pList
	movl	(%rax), %eax	# <variable>.uCount, D.5051
	leal	1(%rax), %edx	#, D.5052
	movq	-40(%rbp), %rax	# pList, pList
	movl	%edx, (%rax)	# D.5052, <variable>.uCount
L5:
	movl	-56(%rbp), %eax	# uSide, uSide.10
	movq	-48(%rbp), %rdx	# pos, pos
	mov	%eax, %eax	# uSide.10, uSide.10
	salq	$5, %rax	#, tmp173
	addq	%rdx, %rax	# pos, tmp174
	addq	$1280, %rax	#, tmp175
	movl	(%rax), %eax	# <variable>.uNonPawnCount, D.5054
	decl	%eax	#
	movl	%eax, -60(%rbp)	#, x
	jmp	L8	#
L9:
	movl	-56(%rbp), %eax	# uSide, uSide.11
	movl	-60(%rbp), %edx	# x, x.12
	movq	-48(%rbp), %rcx	# pos, pos
	mov	%eax, %eax	# uSide.11, uSide.11
	mov	%edx, %edx	# x.12, x.12
	salq	$4, %rax	#, tmp179
	addq	%rdx, %rax	# x.12, tmp180
	movl	1144(%rcx,%rax,4), %eax	# <variable>.cNonPawns, tmp181
	movl	%eax, -16(%rbp)	# tmp181, c
	movl	-16(%rbp), %eax	# c, c.13
	movq	-48(%rbp), %rdx	# pos, pos
	mov	%eax, %eax	# c.13, c.13
	movl	(%rdx,%rax,8), %eax	# <variable>.D.2182.pPiece, <variable>.D.2182.pPiece
	movl	%eax, -20(%rbp)	# <variable>.D.2182.pPiece, p
	movl	-16(%rbp), %edx	# c, c.14
	movl	-52(%rbp), %eax	# cSquare, cSquare.15
	movl	%edx, %ecx	# c.14,
	subl	%eax, %ecx	# cSquare.15,
	movl	%ecx, %eax	#, tmp185
	movl	%eax, -12(%rbp)	# tmp185, iIndex
	movl	-12(%rbp), %eax	# iIndex, iIndex
	cltq
	salq	$2, %rax	#, D.5061
	movq	%rax, %rdx	# D.5061, D.5062
	movq	_g_pVectorDelta@GOTPCREL(%rip), %rax	#, tmp187
	movq	(%rax), %rax	# g_pVectorDelta, g_pVectorDelta.16
	addq	%rax, %rdx	# g_pVectorDelta.16, D.5064
	movl	-20(%rbp), %eax	# p, D.5065
	andl	$1, %eax	#, D.5065
	mov	%eax, %eax	# D.5065, D.5065
	movzbl	(%rdx,%rax), %eax	# <variable>.iVector, D.5066
	movzbl	%al, %edx	# D.5066, D.5067
	movl	-20(%rbp), %eax	# p, p
	shrl	%eax	# D.5068
	movl	%eax, %ecx	# D.5068, D.5069
	movl	%edx, %eax	# D.5067, D.5070
	sarl	%cl, %eax	# D.5069, D.5070
	xorl	$1, %eax	#, D.5072
	andl	$1, %eax	#, D.5074
	testb	%al, %al	# D.5075
	jne	L10	#,
	movl	-20(%rbp), %eax	# p, D.5076
	andl	$6, %eax	#, D.5076
	cmpl	$4, %eax	#, D.5076
	jne	L12	#,
	movq	-40(%rbp), %rax	# pList, pList
	movl	(%rax), %eax	# <variable>.uCount, D.5077
	movq	-40(%rbp), %rcx	# pList, pList
	mov	%eax, %edx	# D.5077, D.5077
	movq	%rdx, %rax	# D.5077, D.5077
	addq	%rax, %rax	# D.5077
	addq	%rdx, %rax	# D.5077, D.5077
	salq	$2, %rax	#, tmp194
	addq	%rcx, %rax	# pList, tmp195
	leaq	4(%rax), %rdx	#, tmp196
	movl	-20(%rbp), %eax	# p, p
	movl	%eax, (%rdx)	# p, <variable>.pPiece
	movq	-40(%rbp), %rax	# pList, pList
	movl	(%rax), %eax	# <variable>.uCount, D.5078
	movq	-40(%rbp), %rcx	# pList, pList
	mov	%eax, %edx	# D.5078, D.5078
	movq	%rdx, %rax	# D.5078, D.5078
	addq	%rax, %rax	# D.5078
	addq	%rdx, %rax	# D.5078, D.5078
	salq	$2, %rax	#, tmp202
	addq	%rcx, %rax	# pList, tmp203
	leaq	8(%rax), %rdx	#, tmp204
	movl	-16(%rbp), %eax	# c, c
	movl	%eax, (%rdx)	# c, <variable>.cLoc
	movq	-40(%rbp), %rax	# pList, pList
	movl	(%rax), %edi	# <variable>.uCount, D.5079
	movl	-20(%rbp), %eax	# p, p
	shrl	%eax	# D.5080
	movq	_g_PieceData@GOTPCREL(%rip), %rcx	#, tmp208
	mov	%eax, %edx	# D.5080, D.5080
	movq	%rdx, %rax	# D.5080, D.5080
	addq	%rax, %rax	# D.5080
	addq	%rdx, %rax	# D.5080, D.5080
	salq	$3, %rax	#, tmp211
	movl	(%rax,%rcx), %ecx	# <variable>.uValue, D.5081
	movq	-40(%rbp), %rsi	# pList, pList
	mov	%edi, %edx	# D.5079, D.5079
	movq	%rdx, %rax	# D.5079, D.5079
	addq	%rax, %rax	# D.5079
	addq	%rdx, %rax	# D.5079, D.5079
	salq	$2, %rax	#, tmp215
	addq	%rsi, %rax	# pList, tmp216
	addq	$12, %rax	#, tmp217
	movl	%ecx, (%rax)	# D.5081, <variable>.uVal
	movq	-40(%rbp), %rax	# pList, pList
	movl	(%rax), %eax	# <variable>.uCount, D.5082
	leal	1(%rax), %edx	#, D.5083
	movq	-40(%rbp), %rax	# pList, pList
	movl	%edx, (%rax)	# D.5083, <variable>.uCount
	jmp	L10	#
L12:
	movl	-12(%rbp), %eax	# iIndex, iIndex
	cltq
	salq	$2, %rax	#, D.5085
	movq	%rax, %rdx	# D.5085, D.5086
	movq	_g_pVectorDelta@GOTPCREL(%rip), %rax	#, tmp221
	movq	(%rax), %rax	# g_pVectorDelta, g_pVectorDelta.17
	leaq	(%rdx,%rax), %rax	#, D.5088
	movzbl	3(%rax), %eax	# <variable>.iNegDelta, D.5089
	movsbl	%al,%eax	# D.5089, D.5089
	movl	%eax, -4(%rbp)	# D.5089, iDelta
	movl	-4(%rbp), %eax	# iDelta, iDelta.18
	addl	-52(%rbp), %eax	# cSquare, tmp223
	movl	%eax, -8(%rbp)	# tmp223, cBlockIndex
	jmp	L14	#
L15:
	movl	-8(%rbp), %eax	# cBlockIndex, cBlockIndex.19
	movq	-48(%rbp), %rdx	# pos, pos
	mov	%eax, %eax	# cBlockIndex.19, cBlockIndex.19
	movl	(%rdx,%rax,8), %eax	# <variable>.D.2182.pPiece, D.5092
	testl	%eax, %eax	# D.5092
	jne	L10	#,
	movl	-4(%rbp), %edx	# iDelta, iDelta.20
	leaq	-8(%rbp), %rax	#, tmp260
	addl	%edx, (%rax)	# iDelta.20, cBlockIndex
L14:
	movl	-8(%rbp), %eax	# cBlockIndex, cBlockIndex
	cmpl	-16(%rbp), %eax	# c, cBlockIndex
	jne	L15	#,
	movq	-40(%rbp), %rax	# pList, pList
	movl	(%rax), %eax	# <variable>.uCount, D.5094
	movq	-40(%rbp), %rcx	# pList, pList
	mov	%eax, %edx	# D.5094, D.5094
	movq	%rdx, %rax	# D.5094, D.5094
	addq	%rax, %rax	# D.5094
	addq	%rdx, %rax	# D.5094, D.5094
	salq	$2, %rax	#, tmp231
	addq	%rcx, %rax	# pList, tmp232
	leaq	4(%rax), %rdx	#, tmp233
	movl	-20(%rbp), %eax	# p, p
	movl	%eax, (%rdx)	# p, <variable>.pPiece
	movq	-40(%rbp), %rax	# pList, pList
	movl	(%rax), %eax	# <variable>.uCount, D.5095
	movq	-40(%rbp), %rcx	# pList, pList
	mov	%eax, %edx	# D.5095, D.5095
	movq	%rdx, %rax	# D.5095, D.5095
	addq	%rax, %rax	# D.5095
	addq	%rdx, %rax	# D.5095, D.5095
	salq	$2, %rax	#, tmp239
	addq	%rcx, %rax	# pList, tmp240
	leaq	8(%rax), %rdx	#, tmp241
	movl	-16(%rbp), %eax	# c, c
	movl	%eax, (%rdx)	# c, <variable>.cLoc
	movq	-40(%rbp), %rax	# pList, pList
	movl	(%rax), %edi	# <variable>.uCount, D.5096
	movl	-20(%rbp), %eax	# p, p
	shrl	%eax	# D.5097
	movq	_g_PieceData@GOTPCREL(%rip), %rcx	#, tmp245
	mov	%eax, %edx	# D.5097, D.5097
	movq	%rdx, %rax	# D.5097, D.5097
	addq	%rax, %rax	# D.5097
	addq	%rdx, %rax	# D.5097, D.5097
	salq	$3, %rax	#, tmp248
	movl	(%rax,%rcx), %ecx	# <variable>.uValue, D.5098
	movq	-40(%rbp), %rsi	# pList, pList
	mov	%edi, %edx	# D.5096, D.5096
	movq	%rdx, %rax	# D.5096, D.5096
	addq	%rax, %rax	# D.5096
	addq	%rdx, %rax	# D.5096, D.5096
	salq	$2, %rax	#, tmp252
	addq	%rsi, %rax	# pList, tmp253
	addq	$12, %rax	#, tmp254
	movl	%ecx, (%rax)	# D.5098, <variable>.uVal
	movq	-40(%rbp), %rax	# pList, pList
	movl	(%rax), %eax	# <variable>.uCount, D.5099
	leal	1(%rax), %edx	#, D.5100
	movq	-40(%rbp), %rax	# pList, pList
	movl	%edx, (%rax)	# D.5100, <variable>.uCount
L10:
	decl	-60(%rbp)	# x
L8:
	cmpl	$-1, -60(%rbp)	#, x
	jne	L9	#,
	leave
	ret
LFE33:
__PushDown:
LFB34:
	pushq	%rbp	#
LCFI2:
	movq	%rsp, %rbp	#,
LCFI3:
	subq	$48, %rsp	#,
LCFI4:
	movq	%rdi, -40(%rbp)	# p, p
	movl	%esi, -44(%rbp)	# u, u
	movl	-44(%rbp), %eax	# u, u
	addl	%eax, %eax	# D.5121
	incl	%eax	# tmp74
	movl	%eax, -12(%rbp)	# tmp74, l
	movq	-40(%rbp), %rax	# p, p
	movl	(%rax), %eax	# <variable>.uCount, D.5122
	cmpl	-12(%rbp), %eax	# l, D.5122
	jbe	L29	#,
	movl	-12(%rbp), %eax	# l, tmp76
	incl	%eax	# tmp76
	movl	%eax, -8(%rbp)	# tmp76, r
	movl	-44(%rbp), %eax	# u, u
	movl	%eax, -4(%rbp)	# u, uSmallest
	movl	-12(%rbp), %eax	# l, l.21
	movq	-40(%rbp), %rcx	# p, p
	mov	%eax, %edx	# l.21, l.21
	movq	%rdx, %rax	# l.21, l.21
	addq	%rax, %rax	# l.21
	addq	%rdx, %rax	# l.21, l.21
	salq	$2, %rax	#, tmp81
	addq	%rcx, %rax	# p, tmp82
	addq	$12, %rax	#, tmp83
	movl	(%rax), %esi	# <variable>.uVal, D.5124
	movl	-44(%rbp), %eax	# u, u.22
	movq	-40(%rbp), %rcx	# p, p
	mov	%eax, %edx	# u.22, u.22
	movq	%rdx, %rax	# u.22, u.22
	addq	%rax, %rax	# u.22
	addq	%rdx, %rax	# u.22, u.22
	salq	$2, %rax	#, tmp87
	addq	%rcx, %rax	# p, tmp88
	addq	$12, %rax	#, tmp89
	movl	(%rax), %eax	# <variable>.uVal, D.5126
	cmpl	%eax, %esi	# D.5126, D.5124
	jae	L23	#,
	movl	-12(%rbp), %eax	# l, l
	movl	%eax, -4(%rbp)	# l, uSmallest
L23:
	movq	-40(%rbp), %rax	# p, p
	movl	(%rax), %eax	# <variable>.uCount, D.5127
	cmpl	-8(%rbp), %eax	# r, D.5127
	jbe	L25	#,
	movl	-8(%rbp), %eax	# r, r.23
	movq	-40(%rbp), %rcx	# p, p
	mov	%eax, %edx	# r.23, r.23
	movq	%rdx, %rax	# r.23, r.23
	addq	%rax, %rax	# r.23
	addq	%rdx, %rax	# r.23, r.23
	salq	$2, %rax	#, tmp95
	addq	%rcx, %rax	# p, tmp96
	addq	$12, %rax	#, tmp97
	movl	(%rax), %esi	# <variable>.uVal, D.5129
	movl	-4(%rbp), %eax	# uSmallest, uSmallest.24
	movq	-40(%rbp), %rcx	# p, p
	mov	%eax, %edx	# uSmallest.24, uSmallest.24
	movq	%rdx, %rax	# uSmallest.24, uSmallest.24
	addq	%rax, %rax	# uSmallest.24
	addq	%rdx, %rax	# uSmallest.24, uSmallest.24
	salq	$2, %rax	#, tmp101
	addq	%rcx, %rax	# p, tmp102
	addq	$12, %rax	#, tmp103
	movl	(%rax), %eax	# <variable>.uVal, D.5131
	cmpl	%eax, %esi	# D.5131, D.5129
	jae	L25	#,
	movl	-8(%rbp), %eax	# r, r
	movl	%eax, -4(%rbp)	# r, uSmallest
L25:
	movl	-4(%rbp), %eax	# uSmallest, uSmallest
	cmpl	-44(%rbp), %eax	# u, uSmallest
	je	L29	#,
	movl	-4(%rbp), %eax	# uSmallest, uSmallest.25
	movq	-40(%rbp), %rcx	# p, p
	mov	%eax, %edx	# uSmallest.25, uSmallest.25
	movq	%rdx, %rax	# uSmallest.25, uSmallest.25
	addq	%rax, %rax	# uSmallest.25
	addq	%rdx, %rax	# uSmallest.25, uSmallest.25
	leaq	0(,%rax,4), %rdx	#, tmp109
	movq	4(%rdx,%rcx), %rax	# <variable>.data, tmp110
	movq	%rax, -32(%rbp)	# tmp110, temp
	movl	12(%rdx,%rcx), %eax	# <variable>.data, tmp111
	movl	%eax, -24(%rbp)	# tmp111, temp
	movl	-4(%rbp), %eax	# uSmallest, uSmallest.26
	movl	-44(%rbp), %ecx	# u, u.27
	movq	-40(%rbp), %rsi	# p, p
	mov	%eax, %edx	# uSmallest.26, uSmallest.26
	movq	%rdx, %rax	# uSmallest.26, uSmallest.26
	addq	%rax, %rax	# uSmallest.26
	addq	%rdx, %rax	# uSmallest.26, uSmallest.26
	leaq	0(,%rax,4), %rdi	#, tmp115
	movq	-40(%rbp), %r8	# p, p
	mov	%ecx, %edx	# u.27, u.27
	movq	%rdx, %rax	# u.27, u.27
	addq	%rax, %rax	# u.27
	addq	%rdx, %rax	# u.27, u.27
	leaq	0(,%rax,4), %rdx	#, tmp119
	movq	4(%rdx,%r8), %rax	# <variable>.data, tmp120
	movq	%rax, 4(%rdi,%rsi)	# tmp120, <variable>.data
	movl	12(%rdx,%r8), %eax	# <variable>.data, tmp121
	movl	%eax, 12(%rdi,%rsi)	# tmp121, <variable>.data
	movl	-44(%rbp), %eax	# u, u.28
	movq	-40(%rbp), %rcx	# p, p
	mov	%eax, %edx	# u.28, u.28
	movq	%rdx, %rax	# u.28, u.28
	addq	%rax, %rax	# u.28
	addq	%rdx, %rax	# u.28, u.28
	leaq	0(,%rax,4), %rdx	#, tmp125
	movq	-32(%rbp), %rax	# temp, temp
	movq	%rax, 4(%rdx,%rcx)	# temp, <variable>.data
	movl	-24(%rbp), %eax	# temp, temp
	movl	%eax, 12(%rdx,%rcx)	# temp, <variable>.data
	movl	-4(%rbp), %esi	# uSmallest, uSmallest
	movq	-40(%rbp), %rdi	# p, p
	call	__PushDown	#
L29:
	leave
	ret
LFE34:
__BuildHeap:
LFB35:
	pushq	%rbp	#
LCFI5:
	movq	%rsp, %rbp	#,
LCFI6:
	subq	$32, %rsp	#,
LCFI7:
	movq	%rdi, -24(%rbp)	# p, p
	movq	-24(%rbp), %rax	# p, p
	movl	(%rax), %eax	# <variable>.uCount, D.5154
	shrl	%eax	# D.5155
	decl	%eax	# D.5156
	movl	%eax, -8(%rbp)	# D.5156, iStart
	movl	-8(%rbp), %eax	# iStart, iStart
	movl	%eax, -4(%rbp)	# iStart, i
	jmp	L31	#
L32:
	movl	-4(%rbp), %esi	# i, i.29
	movq	-24(%rbp), %rdi	# p, p
	call	__PushDown	#
	leaq	-4(%rbp), %rax	#, tmp66
	decl	(%rax)	# i
L31:
	cmpl	$-1, -4(%rbp)	#, i
	jg	L32	#,
	leave
	ret
LFE35:
__BubbleUp:
LFB36:
	pushq	%rbp	#
LCFI8:
	movq	%rsp, %rbp	#,
LCFI9:
	subq	$32, %rsp	#,
LCFI10:
	movq	%rdi, -24(%rbp)	# p, p
	movl	%esi, -28(%rbp)	# u, u
	cmpl	$0, -28(%rbp)	#, u
	je	L39	#,
	movl	-28(%rbp), %eax	# u, D.5164
	decl	%eax	# D.5164
	shrl	%eax	# tmp67
	movl	%eax, -4(%rbp)	# tmp67, uParent
	movl	-4(%rbp), %eax	# uParent, uParent.30
	movq	-24(%rbp), %rcx	# p, p
	mov	%eax, %edx	# uParent.30, uParent.30
	movq	%rdx, %rax	# uParent.30, uParent.30
	addq	%rax, %rax	# uParent.30
	addq	%rdx, %rax	# uParent.30, uParent.30
	salq	$2, %rax	#, tmp71
	addq	%rcx, %rax	# p, tmp72
	addq	$12, %rax	#, tmp73
	movl	(%rax), %esi	# <variable>.uVal, D.5166
	movl	-28(%rbp), %eax	# u, u.31
	movq	-24(%rbp), %rcx	# p, p
	mov	%eax, %edx	# u.31, u.31
	movq	%rdx, %rax	# u.31, u.31
	addq	%rax, %rax	# u.31
	addq	%rdx, %rax	# u.31, u.31
	salq	$2, %rax	#, tmp77
	addq	%rcx, %rax	# p, tmp78
	addq	$12, %rax	#, tmp79
	movl	(%rax), %eax	# <variable>.uVal, D.5168
	cmpl	%eax, %esi	# D.5168, D.5166
	jbe	L39	#,
	movl	-4(%rbp), %eax	# uParent, uParent.32
	movq	-24(%rbp), %rcx	# p, p
	mov	%eax, %edx	# uParent.32, uParent.32
	movq	%rdx, %rax	# uParent.32, uParent.32
	addq	%rax, %rax	# uParent.32
	addq	%rdx, %rax	# uParent.32, uParent.32
	leaq	0(,%rax,4), %rdx	#, tmp83
	movq	4(%rdx,%rcx), %rax	# <variable>.data, tmp84
	movq	%rax, -16(%rbp)	# tmp84, temp
	movl	12(%rdx,%rcx), %eax	# <variable>.data, tmp85
	movl	%eax, -8(%rbp)	# tmp85, temp
	movl	-4(%rbp), %eax	# uParent, uParent.33
	movl	-28(%rbp), %ecx	# u, u.34
	movq	-24(%rbp), %rsi	# p, p
	mov	%eax, %edx	# uParent.33, uParent.33
	movq	%rdx, %rax	# uParent.33, uParent.33
	addq	%rax, %rax	# uParent.33
	addq	%rdx, %rax	# uParent.33, uParent.33
	leaq	0(,%rax,4), %rdi	#, tmp89
	movq	-24(%rbp), %r8	# p, p
	mov	%ecx, %edx	# u.34, u.34
	movq	%rdx, %rax	# u.34, u.34
	addq	%rax, %rax	# u.34
	addq	%rdx, %rax	# u.34, u.34
	leaq	0(,%rax,4), %rdx	#, tmp93
	movq	4(%rdx,%r8), %rax	# <variable>.data, tmp94
	movq	%rax, 4(%rdi,%rsi)	# tmp94, <variable>.data
	movl	12(%rdx,%r8), %eax	# <variable>.data, tmp95
	movl	%eax, 12(%rdi,%rsi)	# tmp95, <variable>.data
	movl	-28(%rbp), %eax	# u, u.35
	movq	-24(%rbp), %rcx	# p, p
	mov	%eax, %edx	# u.35, u.35
	movq	%rdx, %rax	# u.35, u.35
	addq	%rax, %rax	# u.35
	addq	%rdx, %rax	# u.35, u.35
	leaq	0(,%rax,4), %rdx	#, tmp99
	movq	-16(%rbp), %rax	# temp, temp
	movq	%rax, 4(%rdx,%rcx)	# temp, <variable>.data
	movl	-8(%rbp), %eax	# temp, temp
	movl	%eax, 12(%rdx,%rcx)	# temp, <variable>.data
	movl	-4(%rbp), %esi	# uParent, uParent
	movq	-24(%rbp), %rdi	# p, p
	call	__BubbleUp	#
L39:
	leave
	ret
LFE36:
__ClearPieceByLocation:
LFB38:
	pushq	%rbp	#
LCFI11:
	movq	%rsp, %rbp	#,
LCFI12:
	subq	$32, %rsp	#,
LCFI13:
	movq	%rdi, -24(%rbp)	# pList, pList
	movl	%esi, -28(%rbp)	# cLoc, cLoc
	movl	$0, -4(%rbp)	#, x
	jmp	L41	#
L42:
	movl	-4(%rbp), %eax	# x, x.37
	movq	-24(%rbp), %rcx	# pList, pList
	mov	%eax, %edx	# x.37, x.37
	movq	%rdx, %rax	# x.37, x.37
	addq	%rax, %rax	# x.37
	addq	%rdx, %rax	# x.37, x.37
	salq	$2, %rax	#, tmp64
	addq	%rcx, %rax	# pList, tmp65
	addq	$8, %rax	#, tmp66
	movl	(%rax), %eax	# <variable>.cLoc, D.5199
	cmpl	-28(%rbp), %eax	# cLoc, D.5199
	jne	L43	#,
	movl	-4(%rbp), %esi	# x, x
	movq	-24(%rbp), %rdi	# pList, pList
	call	__RemoveItem	#
	jmp	L46	#
L43:
	leaq	-4(%rbp), %rax	#, tmp71
	incl	(%rax)	# x
L41:
	movq	-24(%rbp), %rax	# pList, pList
	movl	(%rax), %eax	# <variable>.uCount, D.5200
	cmpl	-4(%rbp), %eax	# x, D.5200
	ja	L42	#,
L46:
	leave
	ret
LFE38:
__RemoveItem:
LFB37:
	pushq	%rbp	#
LCFI14:
	movq	%rsp, %rbp	#,
LCFI15:
	subq	$16, %rsp	#,
LCFI16:
	movq	%rdi, -8(%rbp)	# pList, pList
	movl	%esi, -12(%rbp)	# x, x
	movq	-8(%rbp), %rax	# pList, pList
	movl	(%rax), %eax	# <variable>.uCount, D.5181
	decl	%eax	# D.5182
	cmpl	-12(%rbp), %eax	# x, D.5182
	je	L48	#,
	movl	-12(%rbp), %edx	# x, x.36
	movq	-8(%rbp), %rax	# pList, pList
	movl	(%rax), %eax	# <variable>.uCount, D.5184
	leal	-1(%rax), %ecx	#, D.5185
	movq	-8(%rbp), %rsi	# pList, pList
	mov	%edx, %edx	# x.36, x.36
	movq	%rdx, %rax	# x.36, x.36
	addq	%rax, %rax	# x.36
	addq	%rdx, %rax	# x.36, x.36
	leaq	0(,%rax,4), %rdi	#, tmp72
	movq	-8(%rbp), %r8	# pList, pList
	mov	%ecx, %edx	# D.5185, D.5185
	movq	%rdx, %rax	# D.5185, D.5185
	addq	%rax, %rax	# D.5185
	addq	%rdx, %rax	# D.5185, D.5185
	leaq	0(,%rax,4), %rdx	#, tmp76
	movq	4(%rdx,%r8), %rax	# <variable>.data, tmp77
	movq	%rax, 4(%rdi,%rsi)	# tmp77, <variable>.data
	movl	12(%rdx,%r8), %eax	# <variable>.data, tmp78
	movl	%eax, 12(%rdi,%rsi)	# tmp78, <variable>.data
	movq	-8(%rbp), %rax	# pList, pList
	movl	(%rax), %eax	# <variable>.uCount, D.5186
	leal	-1(%rax), %edx	#, D.5187
	movq	-8(%rbp), %rax	# pList, pList
	movl	%edx, (%rax)	# D.5187, <variable>.uCount
	movq	-8(%rbp), %rdi	# pList, pList
	movl	$0, %esi	#,
	call	__PushDown	#
	jmp	L51	#
L48:
	movq	-8(%rbp), %rax	# pList, pList
	movl	(%rax), %eax	# <variable>.uCount, D.5188
	leal	-1(%rax), %edx	#, D.5189
	movq	-8(%rbp), %rax	# pList, pList
	movl	%edx, (%rax)	# D.5189, <variable>.uCount
L51:
	leave
	ret
LFE37:
__MinLegalPiece:
LFB39:
	pushq	%rbp	#
LCFI17:
	movq	%rsp, %rbp	#,
LCFI18:
	subq	$80, %rsp	#,
LCFI19:
	movq	%rdi, -24(%rbp)	# pos, pos
	movl	%esi, -28(%rbp)	# uColor, uColor
	movq	%rdx, -40(%rbp)	# pList, pList
	movq	%rcx, -48(%rbp)	# pOther, pOther
	movq	%r8, -56(%rbp)	# pc, pc
	movl	%r9d, -60(%rbp)	# cIgnore, cIgnore
	movl	$0, -64(%rbp)	#, x
	jmp	L53	#
L54:
	movl	-64(%rbp), %eax	# x, x.38
	movq	-40(%rbp), %rcx	# pList, pList
	mov	%eax, %edx	# x.38, x.38
	movq	%rdx, %rax	# x.38, x.38
	addq	%rax, %rax	# x.38
	addq	%rdx, %rax	# x.38, x.38
	salq	$2, %rax	#, tmp76
	addq	%rcx, %rax	# pList, tmp77
	addq	$4, %rax	#, tmp78
	movl	(%rax), %eax	# <variable>.pPiece, <variable>.pPiece
	movl	%eax, -8(%rbp)	# <variable>.pPiece, p
	movl	-8(%rbp), %eax	# p, D.5223
	andl	$14, %eax	#, D.5223
	cmpl	$12, %eax	#, D.5223
	jne	L55	#,
	movq	-48(%rbp), %rax	# pOther, pOther
	movl	(%rax), %eax	# <variable>.uCount, D.5224
	testl	%eax, %eax	# D.5224
	jne	L57	#,
	movq	-40(%rbp), %rax	# pList, pList
	movl	8(%rax), %edx	# <variable>.cLoc, D.5225
	movq	-56(%rbp), %rax	# pc, pc
	movl	%edx, (%rax)	# D.5225,* pc
	movq	-40(%rbp), %rax	# pList, pList
	movl	$0, (%rax)	#, <variable>.uCount
	movl	-8(%rbp), %eax	# p,
	movl	%eax, -68(%rbp)	#, D.5226
	jmp	L59	#
L55:
	movl	-28(%rbp), %eax	# uColor, uColor.39
	movq	-24(%rbp), %rdx	# pos, pos
	mov	%eax, %eax	# uColor.39, uColor.39
	salq	$6, %rax	#, tmp86
	addq	%rdx, %rax	# pos, tmp87
	addq	$1136, %rax	#, tmp88
	movl	8(%rax), %eax	# <variable>.cNonPawns, tmp89
	movl	%eax, -12(%rbp)	# tmp89, cKing
	movl	-64(%rbp), %eax	# x, x.40
	movq	-40(%rbp), %rcx	# pList, pList
	mov	%eax, %edx	# x.40, x.40
	movq	%rdx, %rax	# x.40, x.40
	addq	%rax, %rax	# x.40
	addq	%rdx, %rax	# x.40, x.40
	salq	$2, %rax	#, tmp93
	addq	%rcx, %rax	# pList, tmp94
	addq	$8, %rax	#, tmp95
	movl	(%rax), %esi	# <variable>.cLoc, D.5229
	movl	-12(%rbp), %edx	# cKing, cKing
	movq	-24(%rbp), %rdi	# pos, pos
	call	_ExposesCheck	#
	movl	%eax, -4(%rbp)	# D.5230, c
	movl	-4(%rbp), %eax	# c, D.5233
	andl	$136, %eax	#, D.5233
	testl	%eax, %eax	# D.5233
	jne	L60	#,
	movl	-4(%rbp), %eax	# c, c
	cmpl	-60(%rbp), %eax	# cIgnore, c
	jne	L57	#,
L60:
	movl	-64(%rbp), %eax	# x, x.41
	movq	-40(%rbp), %rcx	# pList, pList
	mov	%eax, %edx	# x.41, x.41
	movq	%rdx, %rax	# x.41, x.41
	addq	%rax, %rax	# x.41
	addq	%rdx, %rax	# x.41, x.41
	salq	$2, %rax	#, tmp102
	addq	%rcx, %rax	# pList, tmp103
	addq	$8, %rax	#, tmp104
	movl	(%rax), %edx	# <variable>.cLoc, D.5235
	movq	-56(%rbp), %rax	# pc, pc
	movl	%edx, (%rax)	# D.5235,* pc
	movq	-40(%rbp), %rdi	# pList, pList
	movl	-64(%rbp), %esi	# x, x
	call	__RemoveItem	#
	movl	-8(%rbp), %eax	# p,
	movl	%eax, -68(%rbp)	#, D.5226
	jmp	L59	#
L57:
	incl	-64(%rbp)	# x
L53:
	movq	-40(%rbp), %rax	# pList, pList
	movl	(%rax), %eax	# <variable>.uCount, D.5236
	cmpl	-64(%rbp), %eax	# x, D.5236
	ja	L54	#,
	movl	$0, -68(%rbp)	#, D.5226
L59:
	movl	-68(%rbp), %eax	# D.5226, <result>
	leave
	ret
LFE39:
__AddXRays:
LFB40:
	pushq	%rbp	#
LCFI20:
	movq	%rsp, %rbp	#,
LCFI21:
	subq	$64, %rsp	#,
LCFI22:
	movq	%rdi, -24(%rbp)	# pos, pos
	movl	%esi, -28(%rbp)	# iAttackerColor, iAttackerColor
	movl	%edx, -32(%rbp)	# cTarget, cTarget
	movl	%ecx, -36(%rbp)	# cObstacle, cObstacle
	movq	%r8, -48(%rbp)	# pAttacks, pAttacks
	movq	%r9, -56(%rbp)	# pDefends, pDefends
	movl	-32(%rbp), %edx	# cTarget, cTarget.42
	movl	-36(%rbp), %eax	# cObstacle, cObstacle.43
	movl	%edx, %ecx	# cTarget.42,
	subl	%eax, %ecx	# cObstacle.43,
	movl	%ecx, %eax	#, tmp118
	movl	%eax, -4(%rbp)	# tmp118, iIndex
	movl	-4(%rbp), %eax	# iIndex, iIndex
	cltq
	salq	$2, %rax	#, D.5262
	movq	%rax, %rdx	# D.5262, D.5263
	movq	_g_pVectorDelta@GOTPCREL(%rip), %rax	#, tmp120
	movq	(%rax), %rax	# g_pVectorDelta, g_pVectorDelta.44
	leaq	(%rdx,%rax), %rax	#, D.5265
	movzbl	(%rax), %eax	# <variable>.iVector, D.5266
	movzbl	%al, %eax	# D.5266, D.5267
	shrl	$5, %eax	#, D.5268
	xorl	$1, %eax	#, D.5269
	andl	$1, %eax	#, D.5271
	testb	%al, %al	# D.5272
	jne	L74	#,
	movl	-4(%rbp), %eax	# iIndex, iIndex
	cltq
	salq	$2, %rax	#, D.5274
	movq	%rax, %rdx	# D.5274, D.5275
	movq	_g_pVectorDelta@GOTPCREL(%rip), %rax	#, tmp122
	movq	(%rax), %rax	# g_pVectorDelta, g_pVectorDelta.45
	leaq	(%rdx,%rax), %rax	#, D.5277
	movzbl	2(%rax), %eax	# <variable>.iDelta, D.5278
	movsbl	%al,%eax	# D.5278, D.5278
	movl	%eax, -16(%rbp)	# D.5278, iDelta
	movl	-16(%rbp), %eax	# iDelta, iDelta.46
	addl	-36(%rbp), %eax	# cObstacle, tmp124
	movl	%eax, -12(%rbp)	# tmp124, cIndex
	jmp	L67	#
L68:
	movl	-12(%rbp), %eax	# cIndex, cIndex.47
	movq	-24(%rbp), %rdx	# pos, pos
	mov	%eax, %eax	# cIndex.47, cIndex.47
	movl	(%rdx,%rax,8), %eax	# <variable>.D.2182.pPiece, <variable>.D.2182.pPiece
	movl	%eax, -8(%rbp)	# <variable>.D.2182.pPiece, xPiece
	cmpl	$0, -8(%rbp)	#, xPiece
	je	L69	#,
	movl	-12(%rbp), %edx	# cIndex, cIndex.48
	movl	-32(%rbp), %eax	# cTarget, cTarget.49
	movl	%edx, %ecx	# cIndex.48,
	subl	%eax, %ecx	# cTarget.49,
	movl	%ecx, %eax	#, D.5283
	cltq
	salq	$2, %rax	#, D.5285
	movq	%rax, %rdx	# D.5285, D.5286
	movq	_g_pVectorDelta@GOTPCREL(%rip), %rax	#, tmp128
	movq	(%rax), %rax	# g_pVectorDelta, g_pVectorDelta.50
	addq	%rax, %rdx	# g_pVectorDelta.50, D.5288
	movl	-8(%rbp), %eax	# xPiece, D.5289
	andl	$1, %eax	#, D.5289
	mov	%eax, %eax	# D.5289, D.5289
	movzbl	(%rdx,%rax), %eax	# <variable>.iVector, D.5290
	movzbl	%al, %edx	# D.5290, D.5291
	movl	-8(%rbp), %eax	# xPiece, xPiece
	shrl	%eax	# D.5292
	movl	%eax, %ecx	# D.5292, D.5293
	movl	%edx, %eax	# D.5291, D.5294
	sarl	%cl, %eax	# D.5293, D.5294
	andl	$1, %eax	#, D.5295
	testb	%al, %al	# D.5296
	je	L74	#,
	movl	-8(%rbp), %edx	# xPiece, D.5297
	andl	$1, %edx	#, D.5297
	movl	-28(%rbp), %eax	# iAttackerColor, iAttackerColor.51
	cmpl	%eax, %edx	# iAttackerColor.51, D.5297
	jne	L72	#,
	movq	-48(%rbp), %rax	# pAttacks, pAttacks
	movl	(%rax), %eax	# <variable>.uCount, D.5299
	movq	-48(%rbp), %rcx	# pAttacks, pAttacks
	mov	%eax, %edx	# D.5299, D.5299
	movq	%rdx, %rax	# D.5299, D.5299
	addq	%rax, %rax	# D.5299
	addq	%rdx, %rax	# D.5299, D.5299
	salq	$2, %rax	#, tmp135
	addq	%rcx, %rax	# pAttacks, tmp136
	leaq	4(%rax), %rdx	#, tmp137
	movl	-8(%rbp), %eax	# xPiece, xPiece
	movl	%eax, (%rdx)	# xPiece, <variable>.pPiece
	movq	-48(%rbp), %rax	# pAttacks, pAttacks
	movl	(%rax), %eax	# <variable>.uCount, D.5300
	movq	-48(%rbp), %rcx	# pAttacks, pAttacks
	mov	%eax, %edx	# D.5300, D.5300
	movq	%rdx, %rax	# D.5300, D.5300
	addq	%rax, %rax	# D.5300
	addq	%rdx, %rax	# D.5300, D.5300
	salq	$2, %rax	#, tmp143
	addq	%rcx, %rax	# pAttacks, tmp144
	leaq	8(%rax), %rdx	#, tmp145
	movl	-12(%rbp), %eax	# cIndex, cIndex
	movl	%eax, (%rdx)	# cIndex, <variable>.cLoc
	movq	-48(%rbp), %rax	# pAttacks, pAttacks
	movl	(%rax), %edi	# <variable>.uCount, D.5301
	movl	-8(%rbp), %eax	# xPiece, xPiece
	shrl	%eax	# D.5302
	movq	_g_PieceData@GOTPCREL(%rip), %rcx	#, tmp149
	mov	%eax, %edx	# D.5302, D.5302
	movq	%rdx, %rax	# D.5302, D.5302
	addq	%rax, %rax	# D.5302
	addq	%rdx, %rax	# D.5302, D.5302
	salq	$3, %rax	#, tmp152
	movl	(%rax,%rcx), %ecx	# <variable>.uValue, D.5303
	movq	-48(%rbp), %rsi	# pAttacks, pAttacks
	mov	%edi, %edx	# D.5301, D.5301
	movq	%rdx, %rax	# D.5301, D.5301
	addq	%rax, %rax	# D.5301
	addq	%rdx, %rax	# D.5301, D.5301
	salq	$2, %rax	#, tmp156
	addq	%rsi, %rax	# pAttacks, tmp157
	addq	$12, %rax	#, tmp158
	movl	%ecx, (%rax)	# D.5303, <variable>.uVal
	movq	-48(%rbp), %rax	# pAttacks, pAttacks
	movl	(%rax), %eax	# <variable>.uCount, D.5304
	leal	1(%rax), %edx	#, D.5305
	movq	-48(%rbp), %rax	# pAttacks, pAttacks
	movl	%edx, (%rax)	# D.5305, <variable>.uCount
	movq	-48(%rbp), %rax	# pAttacks, pAttacks
	movl	(%rax), %eax	# <variable>.uCount, D.5306
	leal	-1(%rax), %esi	#, D.5307
	movq	-48(%rbp), %rdi	# pAttacks, pAttacks
	call	__BubbleUp	#
	jmp	L74	#
L72:
	movq	-56(%rbp), %rax	# pDefends, pDefends
	movl	(%rax), %eax	# <variable>.uCount, D.5308
	movq	-56(%rbp), %rcx	# pDefends, pDefends
	mov	%eax, %edx	# D.5308, D.5308
	movq	%rdx, %rax	# D.5308, D.5308
	addq	%rax, %rax	# D.5308
	addq	%rdx, %rax	# D.5308, D.5308
	salq	$2, %rax	#, tmp167
	addq	%rcx, %rax	# pDefends, tmp168
	leaq	4(%rax), %rdx	#, tmp169
	movl	-8(%rbp), %eax	# xPiece, xPiece
	movl	%eax, (%rdx)	# xPiece, <variable>.pPiece
	movq	-56(%rbp), %rax	# pDefends, pDefends
	movl	(%rax), %eax	# <variable>.uCount, D.5309
	movq	-56(%rbp), %rcx	# pDefends, pDefends
	mov	%eax, %edx	# D.5309, D.5309
	movq	%rdx, %rax	# D.5309, D.5309
	addq	%rax, %rax	# D.5309
	addq	%rdx, %rax	# D.5309, D.5309
	salq	$2, %rax	#, tmp175
	addq	%rcx, %rax	# pDefends, tmp176
	leaq	8(%rax), %rdx	#, tmp177
	movl	-12(%rbp), %eax	# cIndex, cIndex
	movl	%eax, (%rdx)	# cIndex, <variable>.cLoc
	movq	-56(%rbp), %rax	# pDefends, pDefends
	movl	(%rax), %edi	# <variable>.uCount, D.5310
	movl	-8(%rbp), %eax	# xPiece, xPiece
	shrl	%eax	# D.5311
	movq	_g_PieceData@GOTPCREL(%rip), %rcx	#, tmp181
	mov	%eax, %edx	# D.5311, D.5311
	movq	%rdx, %rax	# D.5311, D.5311
	addq	%rax, %rax	# D.5311
	addq	%rdx, %rax	# D.5311, D.5311
	salq	$3, %rax	#, tmp184
	movl	(%rax,%rcx), %ecx	# <variable>.uValue, D.5312
	movq	-56(%rbp), %rsi	# pDefends, pDefends
	mov	%edi, %edx	# D.5310, D.5310
	movq	%rdx, %rax	# D.5310, D.5310
	addq	%rax, %rax	# D.5310
	addq	%rdx, %rax	# D.5310, D.5310
	salq	$2, %rax	#, tmp188
	addq	%rsi, %rax	# pDefends, tmp189
	addq	$12, %rax	#, tmp190
	movl	%ecx, (%rax)	# D.5312, <variable>.uVal
	movq	-56(%rbp), %rax	# pDefends, pDefends
	movl	(%rax), %eax	# <variable>.uCount, D.5313
	leal	1(%rax), %edx	#, D.5314
	movq	-56(%rbp), %rax	# pDefends, pDefends
	movl	%edx, (%rax)	# D.5314, <variable>.uCount
	movq	-56(%rbp), %rax	# pDefends, pDefends
	movl	(%rax), %eax	# <variable>.uCount, D.5315
	leal	-1(%rax), %esi	#, D.5316
	movq	-56(%rbp), %rdi	# pDefends, pDefends
	call	__BubbleUp	#
	jmp	L74	#
L69:
	movl	-16(%rbp), %edx	# iDelta, iDelta.52
	leaq	-12(%rbp), %rax	#, tmp196
	addl	%edx, (%rax)	# iDelta.52, cIndex
L67:
	movl	-12(%rbp), %eax	# cIndex, D.5318
	andl	$136, %eax	#, D.5318
	testl	%eax, %eax	# D.5318
	je	L68	#,
L74:
	leave
	ret
LFE40:
	.data
	.align 4
__Table.5343:
	.long	0
	.long	0
	.long	1
	.long	1
	.long	0
	.long	0
	.text
.globl _SEE
_SEE:
LFB41:
	pushq	%rbp	#
LCFI23:
	movq	%rsp, %rbp	#,
LCFI24:
	subq	$592, %rsp	#,
LCFI25:
	movq	%rdi, -584(%rbp)	# pos, pos
	movl	%esi, -588(%rbp)	# mv, mv
	movl	-588(%rbp), %eax	#, tmp194
	shrl	$16, %eax	#, tmp193
	andl	$15, %eax	#, D.5349
	movzbl	%al, %eax	# D.5349, D.5350
	andl	$1, %eax	#, tmp195
	movl	%eax, -20(%rbp)	# tmp195, uWhoseTurn
	movl	-20(%rbp), %eax	# uWhoseTurn, uWhoseTurn
	movl	%eax, -16(%rbp)	# uWhoseTurn, uOrig
	movl	$1, -12(%rbp)	#, iSign
	movzbl	-588(%rbp), %eax	# mv.D.2138.cFrom, D.5351
	movzbl	%al, %eax	# D.5351, D.5352
	movl	%eax, -564(%rbp)	# D.5352, cFrom
	movzbl	-587(%rbp), %eax	# mv.D.2138.cTo, D.5353
	movzbl	%al, %edx	# D.5353, D.5354
	mov	-20(%rbp), %eax	# uWhoseTurn, D.5355
	imulq	$196, %rax, %rax	#, D.5355, D.5356
	leaq	-432(%rbp), %rdi	#, D.5358
	addq	%rax, %rdi	# D.5357, D.5358
	movl	-20(%rbp), %ecx	# uWhoseTurn, uWhoseTurn
	movq	-584(%rbp), %rsi	# pos, pos
	call	_GetAttacks	#
	movl	-20(%rbp), %ecx	# uWhoseTurn, D.5359
	xorl	$1, %ecx	#, D.5359
	movzbl	-587(%rbp), %eax	# mv.D.2138.cTo, D.5360
	movzbl	%al, %edx	# D.5360, D.5361
	movl	-20(%rbp), %eax	# uWhoseTurn, D.5362
	xorl	$1, %eax	#, D.5362
	mov	%eax, %eax	# D.5362, D.5363
	imulq	$196, %rax, %rax	#, D.5363, D.5364
	leaq	-432(%rbp), %rdi	#, D.5366
	addq	%rax, %rdi	# D.5365, D.5366
	movq	-584(%rbp), %rsi	# pos, pos
	call	_GetAttacks	#
	movl	-20(%rbp), %eax	# uWhoseTurn, D.5367
	xorl	$1, %eax	#, D.5367
	mov	%eax, %eax	# D.5367, D.5368
	imulq	$196, %rax, %rax	#, D.5368, D.5369
	leaq	-432(%rbp), %rdi	#, D.5371
	addq	%rax, %rdi	# D.5370, D.5371
	call	__BuildHeap	#
	mov	-20(%rbp), %eax	# uWhoseTurn, D.5372
	imulq	$196, %rax, %rax	#, D.5372, D.5373
	leaq	-432(%rbp), %rdi	#, D.5375
	addq	%rax, %rdi	# D.5374, D.5375
	call	__BuildHeap	#
	movl	-588(%rbp), %eax	#, tmp202
	shrl	$20, %eax	#, tmp201
	andl	$15, %eax	#, D.5376
	movzbl	%al, %eax	# D.5376, D.5377
	sarl	%eax	# D.5378
	movq	_g_PieceData@GOTPCREL(%rip), %rcx	#, tmp203
	movslq	%eax,%rdx	# D.5378, D.5378
	movq	%rdx, %rax	# D.5378, D.5378
	addq	%rax, %rax	# D.5378
	addq	%rdx, %rax	# D.5378, D.5378
	salq	$3, %rax	#, tmp206
	movl	(%rax,%rcx), %esi	# <variable>.uValue, D.5379
	movl	-588(%rbp), %eax	#, tmp209
	shrl	$24, %eax	#, tmp208
	andl	$15, %eax	#, D.5380
	movzbl	%al, %eax	# D.5380, D.5381
	sarl	%eax	# D.5382
	movq	_g_PieceData@GOTPCREL(%rip), %rcx	#, tmp210
	movslq	%eax,%rdx	# D.5382, D.5382
	movq	%rdx, %rax	# D.5382, D.5382
	addq	%rax, %rax	# D.5382
	addq	%rdx, %rax	# D.5382, D.5382
	salq	$3, %rax	#, tmp213
	movl	(%rax,%rcx), %eax	# <variable>.uValue, D.5383
	leal	(%rsi,%rax), %eax	#, D.5384
	movl	%eax, -560(%rbp)	# D.5385, rgiList
	movl	-588(%rbp), %eax	#, tmp216
	shrl	$16, %eax	#, tmp215
	andl	$15, %eax	#, D.5386
	movzbl	%al, %eax	# D.5386, D.5387
	sarl	%eax	# D.5388
	movq	_g_PieceData@GOTPCREL(%rip), %rcx	#, tmp217
	movslq	%eax,%rdx	# D.5388, D.5388
	movq	%rdx, %rax	# D.5388, D.5388
	addq	%rax, %rax	# D.5388
	addq	%rdx, %rax	# D.5388, D.5388
	salq	$3, %rax	#, tmp220
	movl	(%rax,%rcx), %esi	# <variable>.uValue, D.5389
	movl	-588(%rbp), %eax	#, tmp223
	shrl	$24, %eax	#, tmp222
	andl	$15, %eax	#, D.5390
	movzbl	%al, %eax	# D.5390, D.5391
	sarl	%eax	# D.5392
	movq	_g_PieceData@GOTPCREL(%rip), %rcx	#, tmp224
	movslq	%eax,%rdx	# D.5392, D.5392
	movq	%rdx, %rax	# D.5392, D.5392
	addq	%rax, %rax	# D.5392
	addq	%rdx, %rax	# D.5392, D.5392
	salq	$3, %rax	#, tmp227
	movl	(%rax,%rcx), %eax	# <variable>.uValue, D.5393
	leal	(%rsi,%rax), %eax	#, tmp228
	movl	%eax, -28(%rbp)	# tmp228, uInPeril
	movl	$1, -24(%rbp)	#, uListIndex
	movzbl	-588(%rbp), %eax	# mv.D.2138.cFrom, D.5394
	movzbl	%al, %esi	# D.5394, D.5395
	mov	-20(%rbp), %eax	# uWhoseTurn, D.5396
	imulq	$196, %rax, %rax	#, D.5396, D.5397
	leaq	-432(%rbp), %rdi	#, D.5399
	addq	%rax, %rdi	# D.5398, D.5399
	call	__ClearPieceByLocation	#
	movl	-20(%rbp), %eax	# uWhoseTurn, D.5400
	xorl	$1, %eax	#, D.5400
	mov	%eax, %eax	# D.5400, D.5401
	imulq	$196, %rax, %rax	#, D.5401, D.5402
	leaq	-432(%rbp), %rcx	#, D.5404
	addq	%rax, %rcx	# D.5403, D.5404
	mov	-20(%rbp), %eax	# uWhoseTurn, D.5405
	imulq	$196, %rax, %rax	#, D.5405, D.5406
	leaq	-432(%rbp), %rdx	#, D.5408
	addq	%rax, %rdx	# D.5407, D.5408
	movzbl	-588(%rbp), %eax	# mv.D.2138.cFrom, D.5409
	movzbl	%al, %esi	# D.5409, D.5410
	movzbl	-587(%rbp), %eax	# mv.D.2138.cTo, D.5411
	movzbl	%al, %eax	# D.5411, D.5412
	movl	-20(%rbp), %edi	# uWhoseTurn, uWhoseTurn.53
	movq	-584(%rbp), %r10	# pos, pos
	movq	%rcx, %r9	# D.5404, D.5404
	movq	%rdx, %r8	# D.5408, D.5408
	movl	%esi, %ecx	# D.5410, D.5410
	movl	%eax, %edx	# D.5412, D.5412
	movl	%edi, %esi	# uWhoseTurn.53, uWhoseTurn.53
	movq	%r10, %rdi	# pos, pos
	call	__AddXRays	#
	jmp	L76	#
L85:
	movl	-32(%rbp), %eax	# pPiece, D.5425
	andl	$14, %eax	#, D.5425
	cmpl	$2, %eax	#, D.5425
	sete	%al	#, tmp233
	movzbl	%al, %ecx	# tmp233, D.5426
	movzbl	-587(%rbp), %eax	# mv.D.2138.cTo, D.5427
	movzbl	%al, %eax	# D.5427, D.5428
	andl	$240, %eax	#, D.5429
	cmpl	$112, %eax	#, D.5429
	sete	%dl	#, D.5430
	movzbl	-587(%rbp), %eax	# mv.D.2138.cTo, D.5431
	movzbl	%al, %eax	# D.5431, D.5432
	andl	$240, %eax	#, D.5433
	testl	%eax, %eax	# D.5433
	sete	%al	#, D.5434
	orl	%edx, %eax	# D.5430, D.5435
	movzbl	%al, %eax	# D.5435, D.5436
	andl	%ecx, %eax	# D.5426, D.5437
	movl	%eax, -8(%rbp)	# D.5437, uPromValue
	movl	-8(%rbp), %edx	# uPromValue, uPromValue
	movl	%edx, %eax	# uPromValue, uPromValue
	sall	$4, %eax	#, tmp236
	subl	%edx, %eax	# uPromValue, tmp236
	movl	%eax, %edx	# tmp236, tmp237
	sall	$6, %edx	#, tmp237
	addl	%edx, %eax	# tmp237, tmp238
	movl	%eax, -8(%rbp)	# tmp238, uPromValue
	movl	-24(%rbp), %esi	# uListIndex, uListIndex.55
	movl	-24(%rbp), %eax	# uListIndex, D.5439
	decl	%eax	# D.5439
	mov	%eax, %eax	# D.5439, D.5439
	movl	-560(%rbp,%rax,4), %eax	# rgiList, D.5440
	movl	%eax, %ecx	# D.5440, D.5441
	movl	-8(%rbp), %eax	# uPromValue, uPromValue
	movl	-28(%rbp), %edx	# uInPeril, D.5442
	addl	%eax, %edx	# uPromValue, D.5442
	movl	-12(%rbp), %eax	# iSign, iSign.56
	imull	%edx, %eax	# D.5442, D.5444
	leal	(%rcx,%rax), %eax	#, D.5445
	movl	%eax, %edx	# D.5445, D.5446
	mov	%esi, %eax	# uListIndex.55, uListIndex.55
	movl	%edx, -560(%rbp,%rax,4)	# D.5446, rgiList
	leaq	-24(%rbp), %rax	#, tmp274
	incl	(%rax)	# uListIndex
	movl	-32(%rbp), %eax	# pPiece, pPiece
	shrl	%eax	# D.5447
	movq	_g_PieceData@GOTPCREL(%rip), %rcx	#, tmp243
	mov	%eax, %edx	# D.5447, D.5447
	movq	%rdx, %rax	# D.5447, D.5447
	addq	%rax, %rax	# D.5447
	addq	%rdx, %rax	# D.5447, D.5447
	salq	$3, %rax	#, tmp246
	movl	(%rax,%rcx), %eax	# <variable>.uValue, D.5448
	addl	-8(%rbp), %eax	# uPromValue, tmp247
	movl	%eax, -28(%rbp)	# tmp247, uInPeril
	movl	-16(%rbp), %eax	# uOrig, D.5449
	xorl	$1, %eax	#, D.5449
	mov	%eax, %eax	# D.5449, D.5450
	imulq	$196, %rax, %rax	#, D.5450, D.5451
	leaq	-432(%rbp), %rcx	#, D.5453
	addq	%rax, %rcx	# D.5452, D.5453
	mov	-16(%rbp), %eax	# uOrig, D.5454
	imulq	$196, %rax, %rax	#, D.5454, D.5455
	leaq	-432(%rbp), %rdx	#, D.5457
	addq	%rax, %rdx	# D.5456, D.5457
	movl	-564(%rbp), %r10d	# cFrom, cFrom.57
	movzbl	-587(%rbp), %eax	# mv.D.2138.cTo, D.5459
	movzbl	%al, %esi	# D.5459, D.5460
	movl	-588(%rbp), %eax	#, tmp250
	shrl	$16, %eax	#, tmp249
	andl	$15, %eax	#, D.5461
	movzbl	%al, %eax	# D.5461, D.5462
	andl	$1, %eax	#, D.5463
	movq	-584(%rbp), %rdi	# pos, pos
	movq	%rcx, %r9	# D.5453, D.5453
	movq	%rdx, %r8	# D.5457, D.5457
	movl	%r10d, %ecx	# cFrom.57, cFrom.57
	movl	%esi, %edx	# D.5460, D.5460
	movl	%eax, %esi	# D.5463, D.5463
	call	__AddXRays	#
L76:
	leaq	-20(%rbp), %rax	#, tmp270
	xorl	$1, (%rax)	#, uWhoseTurn
	leaq	-12(%rbp), %rax	#, tmp272
	negl	(%rax)	# iSign
	movl	-564(%rbp), %r8d	# cFrom, cFrom.54
	movl	-20(%rbp), %eax	# uWhoseTurn, D.5415
	xorl	$1, %eax	#, D.5415
	mov	%eax, %eax	# D.5415, D.5416
	imulq	$196, %rax, %rax	#, D.5416, D.5417
	leaq	-432(%rbp), %rcx	#, D.5419
	addq	%rax, %rcx	# D.5418, D.5419
	mov	-20(%rbp), %eax	# uWhoseTurn, D.5420
	imulq	$196, %rax, %rax	#, D.5420, D.5421
	leaq	-432(%rbp), %rdx	#, D.5423
	addq	%rax, %rdx	# D.5422, D.5423
	leaq	-564(%rbp), %rax	#, tmp230
	movl	-20(%rbp), %esi	# uWhoseTurn, uWhoseTurn
	movq	-584(%rbp), %rdi	# pos, pos
	movl	%r8d, %r9d	# cFrom.54, cFrom.54
	movq	%rax, %r8	# tmp230,
	call	__MinLegalPiece	#
	movl	%eax, -32(%rbp)	# D.5424, pPiece
	cmpl	$0, -32(%rbp)	#, pPiece
	jne	L85	#,
L77:
	leaq	-24(%rbp), %rax	#, tmp276
	decl	(%rax)	# uListIndex
	jmp	L79	#
L80:
	movl	-24(%rbp), %eax	# uListIndex, tmp252
	andl	$1, %eax	#, tmp252
	movl	%eax, -4(%rbp)	# tmp252, uVal
	movl	-24(%rbp), %eax	# uListIndex, uListIndex.58
	mov	%eax, %eax	# uListIndex.58, uListIndex.58
	movl	-560(%rbp,%rax,4), %edx	# rgiList, D.5465
	movl	-24(%rbp), %eax	# uListIndex, D.5466
	decl	%eax	# D.5466
	mov	%eax, %eax	# D.5466, D.5466
	movl	-560(%rbp,%rax,4), %eax	# rgiList, D.5467
	cmpl	%eax, %edx	# D.5467, D.5465
	setg	%al	#, tmp255
	movzbl	%al, %ecx	# tmp255, D.5468
	movl	-24(%rbp), %eax	# uListIndex, uListIndex.59
	mov	%eax, %eax	# uListIndex.59, uListIndex.59
	movl	-560(%rbp,%rax,4), %edx	# rgiList, D.5470
	movl	-24(%rbp), %eax	# uListIndex, D.5471
	decl	%eax	# D.5471
	mov	%eax, %eax	# D.5471, D.5471
	movl	-560(%rbp,%rax,4), %eax	# rgiList, D.5472
	cmpl	%eax, %edx	# D.5472, D.5470
	setl	%al	#, tmp258
	movzbl	%al, %eax	# tmp258, D.5473
	movl	%ecx, %edx	# D.5468,
	subl	%eax, %edx	# D.5473,
	movl	%edx, %eax	#, D.5474
	incl	%eax	# tmp259
	movl	%eax, -12(%rbp)	# tmp259, iSign
	movl	-4(%rbp), %eax	# uVal, uVal.60
	movl	-12(%rbp), %edx	# iSign, iSign.61
	mov	%eax, %ecx	# uVal.60, uVal.60
	movslq	%edx,%rdx	# iSign.61, iSign.61
	movq	%rcx, %rax	# uVal.60, uVal.60
	addq	%rax, %rax	# uVal.60
	addq	%rcx, %rax	# uVal.60, uVal.60
	addq	%rdx, %rax	# iSign.61, tmp263
	leaq	0(,%rax,4), %rdx	#, tmp264
	leaq	__Table.5343(%rip), %rax	#, tmp265
	movl	(%rdx,%rax), %eax	# _Table, D.5477
	cmpl	$1, %eax	#, D.5477
	jne	L81	#,
	movl	-24(%rbp), %ecx	# uListIndex, D.5478
	decl	%ecx	# D.5478
	movl	-24(%rbp), %eax	# uListIndex, uListIndex.62
	mov	%eax, %eax	# uListIndex.62, uListIndex.62
	movl	-560(%rbp,%rax,4), %edx	# rgiList, D.5480
	mov	%ecx, %eax	# D.5478, D.5478
	movl	%edx, -560(%rbp,%rax,4)	# D.5480, rgiList
L81:
	leaq	-24(%rbp), %rax	#, tmp278
	decl	(%rax)	# uListIndex
L79:
	cmpl	$0, -24(%rbp)	#, uListIndex
	jne	L80	#,
	movl	-560(%rbp), %eax	# rgiList, D.5481
	leave
	ret
LFE41:
	.section __TEXT,__eh_frame,coalesced,no_toc+strip_static_syms+live_support
EH_frame1:
	.set L$set$0,LECIE1-LSCIE1
	.long L$set$0
LSCIE1:
	.long	0x0
	.byte	0x1
	.ascii "zR\0"
	.byte	0x1
	.byte	0x78
	.byte	0x10
	.byte	0x1
	.byte	0x10
	.byte	0xc
	.byte	0x7
	.byte	0x8
	.byte	0x90
	.byte	0x1
	.align 3
LECIE1:
	.globl _SlowGetAttacks.eh
_SlowGetAttacks.eh:
LSFDE1:
	.set L$set$1,LEFDE1-LASFDE1
	.long L$set$1
LASFDE1:
	.long	LASFDE1-EH_frame1
	.quad	LFB33-.
	.set L$set$2,LFE33-LFB33
	.quad L$set$2
	.byte	0x0
	.byte	0x4
	.set L$set$3,LCFI0-LFB33
	.long L$set$3
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$4,LCFI1-LCFI0
	.long L$set$4
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE1:
__PushDown.eh:
LSFDE3:
	.set L$set$5,LEFDE3-LASFDE3
	.long L$set$5
LASFDE3:
	.long	LASFDE3-EH_frame1
	.quad	LFB34-.
	.set L$set$6,LFE34-LFB34
	.quad L$set$6
	.byte	0x0
	.byte	0x4
	.set L$set$7,LCFI2-LFB34
	.long L$set$7
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$8,LCFI3-LCFI2
	.long L$set$8
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE3:
__BuildHeap.eh:
LSFDE5:
	.set L$set$9,LEFDE5-LASFDE5
	.long L$set$9
LASFDE5:
	.long	LASFDE5-EH_frame1
	.quad	LFB35-.
	.set L$set$10,LFE35-LFB35
	.quad L$set$10
	.byte	0x0
	.byte	0x4
	.set L$set$11,LCFI5-LFB35
	.long L$set$11
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$12,LCFI6-LCFI5
	.long L$set$12
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE5:
__BubbleUp.eh:
LSFDE7:
	.set L$set$13,LEFDE7-LASFDE7
	.long L$set$13
LASFDE7:
	.long	LASFDE7-EH_frame1
	.quad	LFB36-.
	.set L$set$14,LFE36-LFB36
	.quad L$set$14
	.byte	0x0
	.byte	0x4
	.set L$set$15,LCFI8-LFB36
	.long L$set$15
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$16,LCFI9-LCFI8
	.long L$set$16
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE7:
__ClearPieceByLocation.eh:
LSFDE9:
	.set L$set$17,LEFDE9-LASFDE9
	.long L$set$17
LASFDE9:
	.long	LASFDE9-EH_frame1
	.quad	LFB38-.
	.set L$set$18,LFE38-LFB38
	.quad L$set$18
	.byte	0x0
	.byte	0x4
	.set L$set$19,LCFI11-LFB38
	.long L$set$19
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$20,LCFI12-LCFI11
	.long L$set$20
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE9:
__RemoveItem.eh:
LSFDE11:
	.set L$set$21,LEFDE11-LASFDE11
	.long L$set$21
LASFDE11:
	.long	LASFDE11-EH_frame1
	.quad	LFB37-.
	.set L$set$22,LFE37-LFB37
	.quad L$set$22
	.byte	0x0
	.byte	0x4
	.set L$set$23,LCFI14-LFB37
	.long L$set$23
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$24,LCFI15-LCFI14
	.long L$set$24
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE11:
__MinLegalPiece.eh:
LSFDE13:
	.set L$set$25,LEFDE13-LASFDE13
	.long L$set$25
LASFDE13:
	.long	LASFDE13-EH_frame1
	.quad	LFB39-.
	.set L$set$26,LFE39-LFB39
	.quad L$set$26
	.byte	0x0
	.byte	0x4
	.set L$set$27,LCFI17-LFB39
	.long L$set$27
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$28,LCFI18-LCFI17
	.long L$set$28
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE13:
__AddXRays.eh:
LSFDE15:
	.set L$set$29,LEFDE15-LASFDE15
	.long L$set$29
LASFDE15:
	.long	LASFDE15-EH_frame1
	.quad	LFB40-.
	.set L$set$30,LFE40-LFB40
	.quad L$set$30
	.byte	0x0
	.byte	0x4
	.set L$set$31,LCFI20-LFB40
	.long L$set$31
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$32,LCFI21-LCFI20
	.long L$set$32
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE15:
	.globl _SEE.eh
_SEE.eh:
LSFDE17:
	.set L$set$33,LEFDE17-LASFDE17
	.long L$set$33
LASFDE17:
	.long	LASFDE17-EH_frame1
	.quad	LFB41-.
	.set L$set$34,LFE41-LFB41
	.quad L$set$34
	.byte	0x0
	.byte	0x4
	.set L$set$35,LCFI23-LFB41
	.long L$set$35
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$36,LCFI24-LCFI23
	.long L$set$36
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE17:
	.subsections_via_symbols
