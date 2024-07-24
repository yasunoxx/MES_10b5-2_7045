#define printf __printf
#define fprintf __fprintf
#define sprintf __sprintf
#define scanf __scanf
#define fscanf __fscanf
#define sscanf __sscanf
#define fputc __fputc
#define fgetc __fgetc
#define putchar __putchar
#define getchar __getchar
#define read	__read
#define write	__write
#define open	__open
#define close	__close
#define seek	__seek
#define ioctl	__ioctl

int atohex(char*);
int atodec(unsigned char*);
int __printf();
int __fprintf();
void __sprintf();
void __scanf();
void __fscanf();
void __sscanf();
int __fputc(int, int);
int __fgetc(int);
int __putchar(int);
int __getchar();
