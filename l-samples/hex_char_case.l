/* 0xS will generate an error... */
char a := 0xS;
/* 0xAS will not generate, as the minimum digits to form a hexadecimal number
 * have been written, 0xA will be considered a number, but S will be an identifier. */
char a := 0xAS;
