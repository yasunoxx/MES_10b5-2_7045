#include <string.h>
#include <ctype.h>
int s_write(int, char*, int);
int s_read(int, char*, int);
#define FILEDEF		0x200000
#define SCANDEF		0x100000
#define BARDEF		 0x80000
#define DOTDEF		 0x40000
#define ZERODEF	 	 0x20000
#define SIZEDEF		 0x10000
#define ESCAPE 		1
static const unsigned char hextbl[] = "0123456789ABCDEF";

static int atohex(char *str) {
	int	val, i, mi;
	char	c;
	
	val = i = 0;
	while(str[i] <= ' ') i++;
	mi = i;
	while(str[i] > ' ' && (i - mi) < 8) {
		val <<= 4;
		c = str[i];
		if(c >= '0' && c <= '9') {
			val += c - '0';
		} else if(c >= 'a' && c <= 'f') {
			val += c - 'a' + 10;
		} else if(c >= 'A' && c <= 'F') {
			val += c - 'A' + 10;
		} else {
			return -1;
		}
		i++;
	}
	return val;
}

static int atodec(unsigned char *str) {
	int		val, i, mi;
	unsigned char	c;

	val = i = 0;
	while(str[i] <= ' ') i++;
	mi = i;
	if(str[i] == '-') i++;
	while(str[i] > ' ' && (i - mi) < 12) {
		val *= 10;
		c = str[i];
		if(c >= '0' && c <= '9') {
			val += c - '0';
		} else {
			return -1;
		}
		i++;
	}
	if(str[mi] == '-') val = val * (-1);
	return val;
}

static int outchar(char c, char *ptr, int off, int opt) {
	int	fd;

	if(opt & FILEDEF) {
		fd = (int)ptr;
		s_write(fd, &c, 1);
	} else {
		if(ptr > (char*)255) ptr[off] = c;
	}
	return off + 1;
}

static int outstring(char *data, int len, char *ptr, int offset, int _num) {
	int	i, num1, siz, off, fd;

	off = offset;
	num1 = _num & 0xffff;
	siz = len;
	if(_num & FILEDEF) fd = (int)ptr;
	if((_num & (SIZEDEF | BARDEF | DOTDEF)) == SIZEDEF)
		for(i = num1;i > len;i--)
			off = outchar(' ', ptr, off, _num);
	if((_num & SIZEDEF) && (_num & DOTDEF) && (siz > num1)) siz = num1;
	if(_num & FILEDEF) s_write(fd, data, siz);
	else if(ptr > (char*)255) memcpy(&(ptr[off]), data, siz);
	off += siz;
	if((_num & (SIZEDEF | BARDEF | DOTDEF)) == (SIZEDEF | BARDEF))
		for(i = num1;i > len;i--)
			off = outchar(' ', ptr, off, _num);
	return off;
}

static int out_space(int _num, char* ptr, int off) {
	if(_num & ZERODEF)
		return outchar('0', ptr, off, _num);
	else
		return outchar(' ', ptr, off, _num);
}

static int input_number(int *value, char *ptr, int offset, int _num, int sel) {
	char	buffer[16], *inptr;
	int	off, fd;

	off = offset;
	if(_num & FILEDEF) {
		fd = (int)ptr;
		s_read(fd, buffer, 15);
		inptr = buffer;
	} else if(ptr > (char*)255) {
		while(ptr[off] <= ' ') off++;
		inptr = &(ptr[off]);
		while(ptr[off] > ' ') off++;
	}
	if(sel == 0) *value = atodec(inptr);
	else *value = atohex(inptr);
	return off;
}

static int number(unsigned int value, char *ptr, int offset, int _num) {
	unsigned int	a, b, dec;
	int		f, i, off, k, num1;

	if(_num & SCANDEF)
		return input_number((int*)value, ptr, offset, _num, 0);
	dec = value;
	off = offset;
	num1 = _num & 0xffff;
	for(k = 10, i = 1000000000;i >= 10;i /= 10, k--) if(value / i) break;
	if((_num & (SIZEDEF | BARDEF)) == SIZEDEF)
		for(i = num1;i > k;i--)
			off = out_space(_num, ptr, off);
	f = 0;
	for(i = 1000000000;i >= 100;i /= 10) {
		a = dec % i;
		b = i / 10;
		a = a / b;
		if(a > 0) f = 1;
		if(f == 1) off = outchar(hextbl[a], ptr, off, _num);
	}
	a = dec % 10;
	off = outchar(hextbl[a], ptr, off, _num);
	if((_num & (SIZEDEF | BARDEF)) == (SIZEDEF | BARDEF))
		for(i = num1;i > k;i--)
			off = out_space(_num, ptr, off);
	return off;
}

static int decimal(int value, char *ptr, int offset, int _num) {
	int	num, off;

	if(_num & SCANDEF)
		return input_number((int*)value, ptr, offset, _num, 0);
	num = value;
	off = offset;
	if(num < 0) {
		off = outchar('-', ptr, off, _num);
		num = ~num + 1;
	}
	return number(num, ptr, off, _num);
}

static int hex(unsigned int value, char *ptr, int offset, int _num) {
	int	off, sft, index, num1, nm, f, i;
	unsigned int bits;
	char	c;

	if(_num & SCANDEF)
		return input_number((int*)value, ptr, offset, _num, 1);
	off = offset;
	num1 = _num & 0xffff;
	for(nm = 36,bits = 0xffffffff;bits > 0;bits >>= 1, nm--)
		if(~bits & value) break;
	nm >>= 2;
	if((_num & (SIZEDEF | BARDEF)) == SIZEDEF)
		for(i = num1;i > nm;i--)
			off = out_space(_num, ptr, off);
	f = 0;
	for(sft = 28;sft >= 0;sft -= 4) {
		index = (value >> sft) & 0xf;
		if(index > 0) f = 1;
		if(index > 0 || f == 1 || sft == 0)
			off = outchar(hextbl[index], ptr, off, _num);
	}
	if((_num & (SIZEDEF | BARDEF)) == (SIZEDEF | BARDEF))
		for(i = num1;i > nm;i--)
			off = out_space(_num, ptr, off);
	return off;
}

static int vprint(char *p, char *form, int a, int b, int *arg, int _opt) {
	int	i, n, off, argc, value, num, numindex, opt, inoff, fd;
	char	flag, numbuf[5], *ptr, *inptr;

	flag = 0;
	off = 0;
	argc = -2;
	n = strlen(form);
	if(_opt & FILEDEF) fd = (int)p;
	for(i = 0;i < n;i++) {
		if(flag & ESCAPE) {
			if(argc == -2) value = a;
			else if(argc == -1) value = b;
			else value = arg[argc];
		}
		if(form[i] == '%') {
			if(i > 0 && form[i - 1] == '%') {
				flag &= ~ESCAPE;
				if(!(_opt & SCANDEF)) off = outchar(form[i], p, off, _opt); 
			} else {
				flag |= ESCAPE;
				numindex = -1;
				num = 0;
				opt = _opt;
			}
		} else if((flag & ESCAPE) && (form[i] == 'd' || form[i] == 'i')) {
			argc++;
			off = decimal(value, p, off, num | opt);
			flag &= ~ESCAPE;
		} else if((flag & ESCAPE) && form[i] == 'u') {
			argc++;
			off = number(value, p, off, num | opt);
			flag &= ~ESCAPE;
		} else if((flag & ESCAPE) && (form[i] == 'x' || form[i] == 'X')) {
			argc++;
			off = hex(value, p, off, num | opt);
			flag &= ~ESCAPE;
		} else if((flag & ESCAPE) && form[i] == 's') {
			argc++;
			ptr = (char*)value;
			if(opt & SCANDEF) {
				if(_opt & FILEDEF) {
					if(!(opt & SIZEDEF)) num = -1;
					s_read(fd, ptr, num);
				} else if(p > (char*)255) {
					while(p[off] <= ' ') off++;
					inptr = &(p[off]);
					inoff = off;
					while(p[off] > ' ') off++;
					memcpy(ptr, inptr, off - inoff);
					ptr[off - inoff] = 0;
				}
			} else {
				off = outstring(ptr, strlen(ptr), p, off, num | opt);
			}
			flag &= ~ESCAPE;
		} else if((flag & ESCAPE) && form[i] == 'c') {
			argc++;
			if(opt & SCANDEF) {
				ptr = (char*)value;
				if(_opt & FILEDEF) {
					*ptr = 0;
					s_read(fd, ptr, 1);
				} else {
					while(p[off] <= ' ') off++;
					*ptr = p[off++];
				}
			} else {
				off = outchar((char)value, p, off, _opt);
			}
			flag &= ~ESCAPE;
		} else if((flag & ESCAPE) && isdigit(form[i])) {
			if(numindex > 3) flag &= ~ESCAPE;
			else {
				numbuf[++numindex] = form[i];
				numbuf[numindex + 1] = 0;
				num = atodec(numbuf);
				opt |= SIZEDEF;
				if(form[i] == '0') opt |= ZERODEF;
			}
		} else if((flag & ESCAPE) && 
			  (form[i] == '.' || form[i] == ' ' ||
			   form[i] == 'l' || form[i] == '-')) {
			if(form[i] == '.') opt |= DOTDEF;
			if(form[i] == '-') opt |= BARDEF;
		} else {
			flag &= ~ESCAPE;
			if(!(_opt & SCANDEF)) off = outchar(form[i], p, off, _opt); 
		}
	}
	return off;
}

void __printf(char *form, int a, int b, int c) {
	vprint((char*)0, form, a, b, &c, FILEDEF);
}

void __fprintf(int fd, char *form, int a, int b, int c) {
	vprint((char*)fd, form, a, b, &c, FILEDEF);
}

void __sprintf(char *buf, char *form, int a, int b, int c) {
	int	n;

	n = vprint(buf, form, a, b, &c, 0);
	buf[n] = 0;
}

void __scanf(char *form, int a, int b, int c) {
	vprint((char*)1, form, a, b, &c, SCANDEF | FILEDEF);
}

void __fscanf(int fd, char *form, int a, int b, int c) {
	vprint((char*)fd, form, a, b, &c, SCANDEF | FILEDEF);
}

void __sscanf(char *buf, char *form, int a, int b, int c) {
	vprint(buf, form, a, b, &c, SCANDEF);
}

int __fputc(int fd, int c) {
	char	buf;

	buf = c;
	s_write(fd, &buf, 1);
	return c;
}

int __fgetc(int fd) {
	char	buf;

	while(s_read(fd, &buf, 1) == 0);
	return buf & 0xff;
}

int __putchar(int c) {
	char	buf;

	buf = c;
	s_write(0, &buf, 1);
	return c;
}

int __getchar() {
	char	buf;

	while(s_read(1, &buf, 1) == 0);
	return buf & 0xff;
}
