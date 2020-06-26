#ifndef _LANG_ASM_RV32I_HEADER_
#define _LANG_ASM_RV32I_HEADER_

;FX_METADATA(({ interface: [LANG_ASM, GCC_RV32I] }))

#define ASM_ENTRY1(fname) .global fname; fname:
#define ASM_ENTRY2(fname)
#define ENDF
#define ENDFILE
#define EXTERN_FUNC(name)
#define label(name) name:

#endif
