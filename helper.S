// Settings
#define EXTERN_ASM
#define HAVE_REL_RO 1

// Helper macros
.macro function name, export=0, align=2
.text
.align \align
.if \export
.global EXTERN_ASM\name
EXTERN_ASM\name:
.else
\name:
.endif
.endm

.macro const name, align=2, relocate=0
#if HAVE_REL_RO
.if \relocate
.section .data.rel.ro
.else
.section .rodata
.endif
#else
.const_data
#endif
.align\align
\name:
.endm

// Prevent accidental use of x18
#define x18 do_not_use_x18
#define w18 do_not_use_w18

// The only architecture worth using
.arch armv8-a
