/****************************************/
/* CoreOS/Copyleft Yukio Mituiwa,2003	*/
/*					*/
/*  2003/11/11 first release		*/
/*					*/
/****************************************/
#include "macro.h"
#include "pubtype.h"
#include "function.h"
#include "coffdef.h"


static void add(STRING ptr, int32_t address) {
	int32_t	i, data;

	data = 0;
	for(i = 0;i < 4;i++) {
		data <<= 8;
		data += (int32_t)ptr[i] & 0xff;
	}
	data += address;
	for(i = 3;i >= 0;i--) {
		ptr[i] = data & 0xff;
		data >>= 8;
	}
}

int32_t stack_size(STRING coffarea) {
	uint16_t	secnum, optlen;
	int32_t		c, ret;
	FILHDR		*filehdr;
	SCNHDR		*section;

	ret = 0x800;
	filehdr = (FILHDR*)coffarea;
	secnum = filehdr->f_nscns;
	optlen = filehdr->f_opthdr;
	section = (SCNHDR*)&(coffarea[FILHSZ + optlen]);
	for(c = 0;c < secnum;c++) {
		if(memcmp(section[c].s_name, ".stack", 6) == 0) {
			ret = section[c].s_vaddr;
			break;
		}
	}
	return ret;
}

int32_t get_program_size(STRING coffarea) {
	uint16_t magic, secnum, optlen, fileflag, relnum;
	uint32_t c, pgmsize, textsize, datasize, bsssize;
	FILHDR	 *filehdr;
	SCNHDR	 *section;

	filehdr = (FILHDR*)coffarea;
	secnum = filehdr->f_nscns;
	magic = filehdr->f_magic;
	optlen = filehdr->f_opthdr;
	fileflag = filehdr->f_flags;
	section = (SCNHDR*)&(coffarea[FILHSZ + optlen]);
	if((magic != SH2MAGIC) || fileflag & F_RELFLG) return 0;
	for(c = 0;c < secnum;c++) {
		switch(section[c].s_flags) {
		case STYP_TEXT:
			textsize = section[c].s_size;
			break;
		case STYP_DATA:
			datasize = section[c].s_size;
			break;
		case STYP_BSS:
			bsssize = section[c].s_size;
			break;
		}
	}
	pgmsize = textsize + datasize + bsssize;
	return pgmsize;
}

int32_t coff2bin(STRING binarea, STRING coffarea, STRING symlist[], void* ptrlist[]) {
	uint16_t	magic, secnum, optlen, fileflag;
	uint16_t	t_relnum, d_relnum, relnum, relscnum;
	int16_t		relflag, relvalue;
	uint32_t	i, c, s, n, symptr, symnum;
	uint32_t	textsize, datasize, bsssize, pgmsize;
	uint32_t	textadr, dataadr, bssadr;
	uint32_t	textptr, dataptr, bssptr;
	uint32_t	t_relptr, d_relptr, relptr;
	uint32_t	relindex, reladdr, reloff, relflag2, addend;
	int32_t		ret, symbolp;
	STRING		symbolname;
	FILHDR		*filehdr;
	SCNHDR		*section;
	RELOC		*relinfo;
	SYMENT		*syminfo;

	ret = 0;
	filehdr = (FILHDR*)coffarea;
	magic = filehdr->f_magic;
	secnum = filehdr->f_nscns;
	symptr = filehdr->f_symptr;
	symnum = filehdr->f_nsyms;
	optlen = filehdr->f_opthdr;
	fileflag = filehdr->f_flags;
	section = (SCNHDR*)&(coffarea[FILHSZ + optlen]);
	if((magic != SH2MAGIC) || fileflag & F_RELFLG) return -1;
	for(c = 0;c < secnum;c++) {
		switch(section[c].s_flags) {
		case STYP_TEXT:
			textadr = section[c].s_vaddr;
			textsize = section[c].s_size;
			textptr = section[c].s_scnptr;
			t_relptr = section[c].s_relptr;
			t_relnum = section[c].s_nreloc;
			break;
		case STYP_DATA:
			dataadr = section[c].s_vaddr;
			datasize = section[c].s_size;
			dataptr = section[c].s_scnptr;
			d_relptr = section[c].s_relptr;
			d_relnum = section[c].s_nreloc;
			break;
		case STYP_BSS:
			bssadr = section[c].s_vaddr;
			bsssize = section[c].s_size;
			bssptr = section[c].s_scnptr;
			break;
		}
	}
	pgmsize = textsize + datasize + bsssize;
	bzero(binarea, pgmsize);
	memcpy(&(binarea[0]), &(coffarea[textptr]), textsize);
	memcpy(&(binarea[textsize]), &(coffarea[dataptr]), datasize);
	for(s = 0;s <= 1;s++) {
		if(s == 0) {
			relptr = t_relptr;
			relnum = t_relnum;
		} else if(s == 1) {
			relptr = d_relptr;
			relnum = d_relnum;
		} else {
			break;
		}
		relinfo = (RELOC*)&(coffarea[relptr]);
		syminfo = (SYMENT*)&(coffarea[symptr]);
		for(c = 0;c < relnum;c++) {
			reladdr = relinfo[c].r_vaddr;
			relindex = relinfo[c].r_symndx;
			relflag = relinfo[c].r_type;
			addend = (int32_t)(relinfo[c].r_pad1) & 0xffff;
			relflag2 = relinfo[c].r_pad2;
			if(relindex >= 0 && relindex < symnum) {
				reloff = syminfo[relindex].e_value;
				relscnum = syminfo[relindex].e_scnum;
				switch(relflag) {
				case N_UNDEF:
					if(relscnum == 0) {
						if(syminfo[relindex].e.e.e_zeroes == 0) {
							symbolp = symnum * sizeof(SYMENT) + symptr + syminfo[relindex].e.e.e_offset;
							symbolname = &(coffarea[symbolp]);
						} else {
							symbolname = syminfo[relindex].e.e_name;
						}
						n = strlen(symbolname);
						for(i = 0;symlist[i] != 0;i++) {
							if(memcmp(symbolname, symlist[i], n + 1) == 0) {
								add(&(binarea[reladdr]), (int32_t)ptrlist[i]);
								break;
							}
						}
						if(symlist[i] == 0) ret = -2;
					} else {
						switch(magic) {
							case SH2MAGIC:
							add(&(binarea[reladdr]), (int32_t)binarea);
							break;
						}
					}
					break;
				case N_ABS:
					if(relflag2 & 0x10000) {
						relvalue = reloff - reladdr - 2;
						binarea[reladdr] = relvalue >> 8;
						binarea[reladdr + 1] = relvalue;
					} else {
						binarea[reladdr] = reloff - reladdr - 1;
					}
					break;
				}
			}
		}
	}
	return ret;
}
