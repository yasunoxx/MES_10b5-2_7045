typedef struct {
	uint16_t f_magic;		/* magic number			*/
	uint16_t f_nscns;		/* number of sections		*/
	uint32_t f_timdat;		/* time & date stamp		*/
	uint32_t f_symptr;		/* file pointer to symtab	*/
	uint32_t f_nsyms;		/* number of symtab entries	*/
	uint16_t f_opthdr;		/* sizeof(optional hdr)		*/
	uint16_t f_flags;		/* flags			*/
} FILHDR;

#define F_RELFLG	(0x0001)
#define F_EXEC		(0x0002)
#define F_LNNO		(0x0004)
#define F_LSYMS		(0x0008)

#define	SH2MAGIC	0x0500
#define	FILHSZ		sizeof(FILHDR)

typedef struct {
	Byte	 s_name[8];	/* section name			*/
	uint32_t s_paddr;	/* physical address, aliased s_nlib */
	uint32_t s_vaddr;	/* virtual address		*/
	uint32_t s_size;		/* section size			*/
	uint32_t s_scnptr;	/* file ptr to raw data for section */
	uint32_t s_relptr;	/* file ptr to relocation	*/
	uint32_t s_lnnoptr;	/* file ptr to line numbers	*/
	uint16_t s_nreloc;	/* number of relocation entries	*/
	uint16_t s_nlnno;	/* number of line number entries*/
	uint32_t s_flags;	/* flags			*/
} SCNHDR;

#define	SCNHSZ	sizeof(SCNHDR)

#define _TEXT	".text"
#define _DATA	".data"
#define _BSS	".bss"
#define _COMMENT ".comment"
#define _LIB ".lib"

#define STYP_TEXT	(0x0020)	/* section contains text only */
#define STYP_DATA	(0x0040)	/* section contains data only */
#define STYP_BSS	(0x0080)	/* section contains bss only */

#define E_SYMNMLEN	8	/* # characters in a symbol name	*/

typedef struct {
	union {
		Byte e_name[E_SYMNMLEN];
		struct {
			uint32_t e_zeroes __attribute__((packed));
			uint32_t e_offset __attribute__((packed));
		} e;
	} e;
	uint32_t e_value __attribute__((packed));
	int16_t	 e_scnum;
	uint16_t e_type;
	Byte	 e_sclass;
} SYMENT;

#define	SYMESZ	sizeof(SYMENT)

typedef struct {
	uint32_t r_vaddr;
	uint32_t r_symndx;
	uint16_t r_type;
	uint16_t r_pad1;
	uint32_t r_pad2;
} RELOC;

#define RELSZ sizeof(RELOC)

#define N_UNDEF	0	/* undefined symbol */
#define N_ABS	-1	/* value of symbol is absolute */
