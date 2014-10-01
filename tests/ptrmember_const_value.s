	.file	"pointer_const_value.c"
	.text
.Ltext0:
	.p2align 4,,15
	.globl	foo_wrap
	.type	foo_wrap, @function
foo_wrap:
.LFB1:
	.file 1 "tests/pointer_const_value.c"
	.loc 1 5 0
	.cfi_startproc
.LVL0:
.LBB4:
.LBB5:
	.loc 1 2 0
	movq	$0, (%rdi)
.LBE5:
.LBE4:
	.loc 1 7 0
	ret
	.cfi_endproc
.LFE1:
	.size	foo_wrap, .-foo_wrap
.Letext0:
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.long	0xa4
	.value	0x3
	.long	.Ldebug_abbrev0
	.byte	0x8
	.uleb128 0x1
	.long	.LASF1
	.byte	0x1
	.long	.LASF2
	.long	.LASF3
	.quad	.Ltext0
	.quad	.Letext0
	.long	.Ldebug_line0
	.uleb128 0x2
	.string	"foo"
	.byte	0x1
	.byte	0x1
	.byte	0x1
	.byte	0x1
	.long	0x51
	.uleb128 0x3
	.long	.LASF0
	.byte	0x1
	.byte	0x1
	.long	0x51
	.uleb128 0x4
	.string	"foo"
	.byte	0x1
	.byte	0x1
	.long	0x57
	.byte	0
	.uleb128 0x5
	.byte	0x8
	.long	0x57
	.uleb128 0x6
	.byte	0x8
	.uleb128 0x7
	.byte	0x1
	.long	.LASF4
	.byte	0x1
	.byte	0x5
	.byte	0x1
	.quad	.LFB1
	.quad	.LFE1
	.byte	0x1
	.byte	0x9c
	.uleb128 0x8
	.long	.LASF0
	.byte	0x1
	.byte	0x5
	.long	0x51
	.byte	0x1
	.byte	0x55
	.uleb128 0x9
	.long	0x2d
	.quad	.LBB4
	.quad	.LBE4
	.byte	0x1
	.byte	0x6
	.uleb128 0xa
	.long	0x45
	.byte	0
	.uleb128 0xb
	.long	0x3a
	.byte	0x1
	.byte	0x55
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.uleb128 0x1
	.uleb128 0x11
	.byte	0x1
	.uleb128 0x25
	.uleb128 0xe
	.uleb128 0x13
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x1b
	.uleb128 0xe
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x10
	.uleb128 0x6
	.byte	0
	.byte	0
	.uleb128 0x2
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0xc
	.uleb128 0x20
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x3
	.uleb128 0x5
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x4
	.uleb128 0x5
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x5
	.uleb128 0xf
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x6
	.uleb128 0x1f
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x7
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0xc
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x40
	.uleb128 0xa
	.byte	0
	.byte	0
	.uleb128 0x8
	.uleb128 0x5
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0xa
	.byte	0
	.byte	0
	.uleb128 0x9
	.uleb128 0x1d
	.byte	0x1
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x58
	.uleb128 0xb
	.uleb128 0x59
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0xa
	.uleb128 0x5
	.byte	0
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x1c
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0xb
	.uleb128 0x5
	.byte	0
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0xa
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_aranges,"",@progbits
	.long	0x2c
	.value	0x2
	.long	.Ldebug_info0
	.byte	0x8
	.byte	0
	.value	0
	.value	0
	.quad	.Ltext0
	.quad	.Letext0-.Ltext0
	.quad	0
	.quad	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
.LASF3:
	.string	"/home/petr/proj/dwgrep"
.LASF0:
	.string	"valp"
.LASF2:
	.string	"tests/pointer_const_value.c"
.LASF4:
	.string	"foo_wrap"
.LASF1:
	.string	"GNU C 4.6.3 20120306 (Red Hat 4.6.3-2)"
	.ident	"GCC: (GNU) 4.6.3 20120306 (Red Hat 4.6.3-2)"
	.section	.note.GNU-stack,"",@progbits
