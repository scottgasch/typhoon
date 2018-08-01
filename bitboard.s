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

.globl _BBSQUARE
	.data
	.align 5
_BBSQUARE:
	.quad	1
	.quad	2
	.quad	4
	.quad	8
	.quad	16
	.quad	32
	.quad	64
	.quad	128
	.quad	256
	.quad	512
	.quad	1024
	.quad	2048
	.quad	4096
	.quad	8192
	.quad	16384
	.quad	32768
	.quad	65536
	.quad	131072
	.quad	262144
	.quad	524288
	.quad	1048576
	.quad	2097152
	.quad	4194304
	.quad	8388608
	.quad	16777216
	.quad	33554432
	.quad	67108864
	.quad	134217728
	.quad	268435456
	.quad	536870912
	.quad	1073741824
	.quad	2147483648
	.quad	4294967296
	.quad	8589934592
	.quad	17179869184
	.quad	34359738368
	.quad	68719476736
	.quad	137438953472
	.quad	274877906944
	.quad	549755813888
	.quad	1099511627776
	.quad	2199023255552
	.quad	4398046511104
	.quad	8796093022208
	.quad	17592186044416
	.quad	35184372088832
	.quad	70368744177664
	.quad	140737488355328
	.quad	281474976710656
	.quad	562949953421312
	.quad	1125899906842624
	.quad	2251799813685248
	.quad	4503599627370496
	.quad	9007199254740992
	.quad	18014398509481984
	.quad	36028797018963968
	.quad	72057594037927936
	.quad	144115188075855872
	.quad	288230376151711744
	.quad	576460752303423488
	.quad	1152921504606846976
	.quad	2305843009213693952
	.quad	4611686018427387904
	.quad	-9223372036854775808
.globl _BBWHITESQ
	.align 3
_BBWHITESQ:
	.quad	-6172840429334713771
.globl _BBBLACKSQ
	.align 3
_BBBLACKSQ:
	.quad	6172840429334713770
.globl _BBFILE
	.align 5
_BBFILE:
	.quad	72340172838076673
	.quad	144680345676153346
	.quad	289360691352306692
	.quad	578721382704613384
	.quad	1157442765409226768
	.quad	2314885530818453536
	.quad	4629771061636907072
	.quad	-9187201950435737472
.globl _BBRANK
	.align 5
_BBRANK:
	.quad	0
	.quad	-72057594037927936
	.quad	71776119061217280
	.quad	280375465082880
	.quad	1095216660480
	.quad	4278190080
	.quad	16711680
	.quad	65280
	.quad	255
.globl _BBROOK_PAWNS
	.align 3
_BBROOK_PAWNS:
	.quad	-9114861777597660799
	.text
.globl _SlowCountBits
_SlowCountBits:
LFB33:
	pushq	%rbp	#
LCFI0:
	movq	%rsp, %rbp	#,
LCFI1:
	movq	%rdi, -24(%rbp)	# bb, bb
	movl	$0, -4(%rbp)	#, uCount
	jmp	L2	#
L3:
	leaq	-4(%rbp), %rax	#, tmp63
	incl	(%rax)	# uCount
	movq	-24(%rbp), %rdx	# bb, D.5028
	decq	%rdx	# D.5028
	leaq	-24(%rbp), %rax	#, tmp65
	andq	%rdx, (%rax)	# D.5028, bb
L2:
	cmpq	$0, -24(%rbp)	#, bb
	jne	L3	#,
	movl	-4(%rbp), %eax	# uCount, D.5029
	leave
	ret
LFE33:
	.const
	.align 5
_foldedTable:
	.long	63
	.long	30
	.long	3
	.long	32
	.long	59
	.long	14
	.long	11
	.long	33
	.long	60
	.long	24
	.long	50
	.long	9
	.long	55
	.long	19
	.long	21
	.long	34
	.long	61
	.long	29
	.long	2
	.long	53
	.long	51
	.long	23
	.long	41
	.long	18
	.long	56
	.long	28
	.long	1
	.long	43
	.long	46
	.long	27
	.long	0
	.long	35
	.long	62
	.long	31
	.long	58
	.long	4
	.long	5
	.long	49
	.long	54
	.long	6
	.long	15
	.long	52
	.long	12
	.long	40
	.long	7
	.long	42
	.long	45
	.long	16
	.long	25
	.long	57
	.long	48
	.long	13
	.long	10
	.long	39
	.long	8
	.long	44
	.long	20
	.long	47
	.long	38
	.long	22
	.long	17
	.long	37
	.long	36
	.long	26
	.text
.globl _DeBruijnFirstBit
_DeBruijnFirstBit:
LFB34:
	pushq	%rbp	#
LCFI2:
	movq	%rsp, %rbp	#,
LCFI3:
	movq	%rdi, -24(%rbp)	# bb, bb
	cmpq	$0, -24(%rbp)	#, bb
	jne	L7	#,
	movl	$0, -28(%rbp)	#, D.5036
	jmp	L9	#
L7:
	movq	-24(%rbp), %rdx	# bb, D.5037
	decq	%rdx	# D.5037
	leaq	-24(%rbp), %rax	#, tmp77
	xorq	%rdx, (%rax)	# D.5037, bb
	movq	-24(%rbp), %rax	# bb, bb
	movl	%eax, %edx	# bb, D.5038
	movq	-24(%rbp), %rax	# bb, bb
	shrq	$32, %rax	#, D.5039
	xorl	%edx, %eax	# D.5038, tmp70
	movl	%eax, -4(%rbp)	# tmp70, folded
	movl	-4(%rbp), %eax	# folded, folded
	imull	$2015959759, %eax, %eax	#, folded, D.5041
	sarl	$26, %eax	#, D.5042
	cltq
	leaq	0(,%rax,4), %rdx	#, tmp73
	leaq	_foldedTable(%rip), %rax	#, tmp74
	movl	(%rdx,%rax), %eax	# foldedTable, D.5043
	incl	%eax	# D.5044
	movl	%eax, -28(%rbp)	# D.5044, D.5036
L9:
	movl	-28(%rbp), %eax	# D.5036, <result>
	leave
	ret
LFE34:
	.data
	.align 5
_uTable.5051:
	.long	0
	.long	1
	.long	2
	.long	1
	.long	3
	.long	1
	.long	2
	.long	1
	.long	4
	.long	1
	.long	2
	.long	1
	.long	3
	.long	1
	.long	2
	.long	1
	.text
.globl _SlowFirstBit
_SlowFirstBit:
LFB35:
	pushq	%rbp	#
LCFI4:
	movq	%rsp, %rbp	#,
LCFI5:
	movq	%rdi, -24(%rbp)	# bb, bb
	movl	$0, -8(%rbp)	#, uShifts
	jmp	L12	#
L13:
	movq	-24(%rbp), %rax	# bb, bb
	andl	$15, %eax	#, tmp65
	movl	%eax, -4(%rbp)	# tmp65, u
	cmpl	$0, -4(%rbp)	#, u
	je	L14	#,
	movl	-4(%rbp), %eax	# u, u.5
	mov	%eax, %eax	# u.5, u.5
	leaq	0(,%rax,4), %rdx	#, tmp67
	leaq	_uTable.5051(%rip), %rax	#, tmp68
	movl	(%rdx,%rax), %edx	# uTable, D.5060
	movl	-8(%rbp), %eax	# uShifts, uShifts
	sall	$2, %eax	#, D.5061
	addl	%eax, %edx	# D.5061,
	movl	%edx, -28(%rbp)	#, D.5058
	jmp	L16	#
L14:
	leaq	-24(%rbp), %rax	#, tmp72
	shrq	$4, (%rax)	#, bb
	leaq	-8(%rbp), %rax	#, tmp74
	incl	(%rax)	# uShifts
L12:
	cmpq	$0, -24(%rbp)	#, bb
	jne	L13	#,
	movl	$0, -28(%rbp)	#, D.5058
L16:
	movl	-28(%rbp), %eax	# D.5058, <result>
	leave
	ret
LFE35:
	.data
	.align 5
_uTable.5068:
	.long	0
	.long	1
	.long	2
	.long	2
	.long	3
	.long	3
	.long	3
	.long	3
	.long	4
	.long	4
	.long	4
	.long	4
	.long	4
	.long	4
	.long	4
	.long	4
	.text
.globl _SlowLastBit
_SlowLastBit:
LFB36:
	pushq	%rbp	#
LCFI6:
	movq	%rsp, %rbp	#,
LCFI7:
	movq	%rdi, -24(%rbp)	# bb, bb
	movl	$15, -8(%rbp)	#, uShifts
	jmp	L20	#
L21:
	movabsq	$-1152921504606846976, %rax	#, tmp65
	andq	-24(%rbp), %rax	# bb, D.5074
	shrq	$60, %rax	#, D.5075
	movl	%eax, -4(%rbp)	# D.5075, u
	cmpl	$0, -4(%rbp)	#, u
	je	L22	#,
	movl	-4(%rbp), %eax	# u, u.6
	mov	%eax, %eax	# u.6, u.6
	leaq	0(,%rax,4), %rdx	#, tmp67
	leaq	_uTable.5068(%rip), %rax	#, tmp68
	movl	(%rdx,%rax), %edx	# uTable, D.5078
	movl	-8(%rbp), %eax	# uShifts, uShifts
	sall	$2, %eax	#, D.5079
	addl	%eax, %edx	# D.5079,
	movl	%edx, -28(%rbp)	#, D.5076
	jmp	L24	#
L22:
	leaq	-24(%rbp), %rax	#, tmp72
	salq	$4, (%rax)	#, bb
	leaq	-8(%rbp), %rax	#, tmp74
	decl	(%rax)	# uShifts
L20:
	cmpq	$0, -24(%rbp)	#, bb
	jne	L21	#,
	movl	$0, -28(%rbp)	#, D.5076
L24:
	movl	-28(%rbp), %eax	# D.5076, <result>
	leave
	ret
LFE36:
.globl _CoorFromBitBoardRank8ToRank1
_CoorFromBitBoardRank8ToRank1:
LFB37:
	pushq	%rbp	#
LCFI8:
	movq	%rsp, %rbp	#,
LCFI9:
	subq	$32, %rsp	#,
LCFI10:
	movq	%rdi, -24(%rbp)	# pbb, pbb
	movl	$136, -8(%rbp)	#, c
	movq	-24(%rbp), %rax	# pbb, pbb
	movq	(%rax), %rdi	#* pbb, D.5088
	call	_FirstBit	#
	movl	%eax, -4(%rbp)	# D.5089, uFirstBit
	cmpl	$0, -4(%rbp)	#, uFirstBit
	je	L28	#,
	leaq	-4(%rbp), %rax	#, tmp76
	decl	(%rax)	# uFirstBit
	movl	-4(%rbp), %eax	# uFirstBit, D.5090
	andl	$248, %eax	#, D.5090
	leal	(%rax,%rax), %edx	#, D.5091
	movl	-4(%rbp), %eax	# uFirstBit, D.5092
	andl	$7, %eax	#, D.5092
	orl	%edx, %eax	# D.5091, tmp70
	movl	%eax, -8(%rbp)	# tmp70, c
	movq	-24(%rbp), %rax	# pbb, pbb
	movq	(%rax), %rdx	#* pbb, D.5093
	movq	-24(%rbp), %rax	# pbb, pbb
	movq	(%rax), %rax	#* pbb, D.5094
	decq	%rax	# D.5095
	andq	%rax, %rdx	# D.5095, D.5096
	movq	-24(%rbp), %rax	# pbb, pbb
	movq	%rdx, (%rax)	# D.5096,* pbb
L28:
	movl	-8(%rbp), %eax	# c, D.5097
	leave
	ret
LFE37:
.globl _CoorFromBitBoardRank1ToRank8
_CoorFromBitBoardRank1ToRank8:
LFB38:
	pushq	%rbp	#
LCFI11:
	movq	%rsp, %rbp	#,
LCFI12:
	subq	$32, %rsp	#,
LCFI13:
	movq	%rdi, -24(%rbp)	# pbb, pbb
	movq	-24(%rbp), %rax	# pbb, pbb
	movq	(%rax), %rdi	#* pbb, D.5106
	call	_LastBit	#
	movl	%eax, -4(%rbp)	# D.5107, uLastBit
	movl	$136, -8(%rbp)	#, c
	cmpl	$0, -4(%rbp)	#, uLastBit
	je	L32	#,
	leaq	-4(%rbp), %rax	#, tmp76
	decl	(%rax)	# uLastBit
	movq	-24(%rbp), %rax	# pbb, pbb
	movq	(%rax), %rdx	#* pbb, D.5108
	movq	-24(%rbp), %rax	# pbb, pbb
	movq	(%rax), %rax	#* pbb, D.5109
	decq	%rax	# D.5110
	andq	%rax, %rdx	# D.5110, D.5111
	movq	-24(%rbp), %rax	# pbb, pbb
	movq	%rdx, (%rax)	# D.5111,* pbb
	movl	-4(%rbp), %eax	# uLastBit, D.5112
	andl	$248, %eax	#, D.5112
	leal	(%rax,%rax), %edx	#, D.5113
	movl	-4(%rbp), %eax	# uLastBit, D.5114
	andl	$7, %eax	#, D.5114
	orl	%edx, %eax	# D.5113, tmp73
	movl	%eax, -8(%rbp)	# tmp73, c
L32:
	movl	-8(%rbp), %eax	# c, D.5115
	leave
	ret
LFE38:
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
	.globl _SlowCountBits.eh
_SlowCountBits.eh:
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
	.globl _DeBruijnFirstBit.eh
_DeBruijnFirstBit.eh:
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
	.globl _SlowFirstBit.eh
_SlowFirstBit.eh:
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
	.set L$set$11,LCFI4-LFB35
	.long L$set$11
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$12,LCFI5-LCFI4
	.long L$set$12
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE5:
	.globl _SlowLastBit.eh
_SlowLastBit.eh:
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
	.set L$set$15,LCFI6-LFB36
	.long L$set$15
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$16,LCFI7-LCFI6
	.long L$set$16
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE7:
	.globl _CoorFromBitBoardRank8ToRank1.eh
_CoorFromBitBoardRank8ToRank1.eh:
LSFDE9:
	.set L$set$17,LEFDE9-LASFDE9
	.long L$set$17
LASFDE9:
	.long	LASFDE9-EH_frame1
	.quad	LFB37-.
	.set L$set$18,LFE37-LFB37
	.quad L$set$18
	.byte	0x0
	.byte	0x4
	.set L$set$19,LCFI8-LFB37
	.long L$set$19
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$20,LCFI9-LCFI8
	.long L$set$20
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE9:
	.globl _CoorFromBitBoardRank1ToRank8.eh
_CoorFromBitBoardRank1ToRank8.eh:
LSFDE11:
	.set L$set$21,LEFDE11-LASFDE11
	.long L$set$21
LASFDE11:
	.long	LASFDE11-EH_frame1
	.quad	LFB38-.
	.set L$set$22,LFE38-LFB38
	.quad L$set$22
	.byte	0x0
	.byte	0x4
	.set L$set$23,LCFI11-LFB38
	.long L$set$23
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$24,LCFI12-LCFI11
	.long L$set$24
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE11:
	.subsections_via_symbols
