/*	This file is part of the software similarity tester SIM.
	Written by Dick Grune, VU, Amsterdam; dick@dickgrune.com
	$Id: pass1.c,v 2.45 2020-08-14 16:56:52 dick Exp $
*/

#include	<string.h>

#include	"pass1.h"

#include	"debug.par"
#include	"global.h"
#include	"text.h"
#include	"token.h"
#include	"tokenarray.h"
#include	"lang.h"

static void
do_separator(const struct text *txt, const char *fname) {
	printf("File %s: new/old separator\n", fname);
	if (Number_of_New_Texts == Number_of_Texts) {
		/* no new Number_of_New_Texts set yet */
		Number_of_New_Texts = txt - &Text[0];
	} else {
		Fatal("more than one new/old separator");
	}
}

static int
read_text(const char *fname, struct text *txt) {
	int file_opened = 0;
	if (Open_Text(txt)) {
		file_opened = 1;
	} else {
		/* print a warning */
		printf("File %s: >>>> cannot open <<<<\n", fname);
		/* the file has still been opened with a null file
		   for uniformity
		*/
	}

	while (Next_Text_Token_Obtained()) {
		if (!Token_EQ(lex_token, End_Of_Line)) {
			Store_In_Token(lex_token);
		}
	}
	Close_Text();
	txt->tx_limit = Token_Array_Length();
	txt->tx_EOL_terminated =
		Token_EQ(lex_token, End_Of_Line);

	return file_opened
		;
}

static void print_count(size_t cnt, const char *unit) {
	/*	Prints a grammatically correct string "%u %s[s]"
		for units that form their plural by suffixing -s.
	*/
	printf("%s %s%s", size_t2string(cnt), unit, (cnt == 1 ? "" : "s"));
}

static void report_file(const char *fname, struct text *txt) {
	printf("File %s: ", fname);
	// 打印token数
	char tokenFilename[100];
	strcpy(tokenFilename, txt->tx_fname);
	strcat(tokenFilename, ".tk");
	size_t length_token = txt->tx_limit - txt->tx_start;
	fprint_Tokens(tokenFilename, Token_Array + txt->tx_start, length_token);
	printf("start=%lld limit=%lld\n", txt->tx_start, txt->tx_limit);
	print_count(length_token, Token_Name);
	printf(", ");
	// 打印行数
	print_count(lex_nl_cnt - 1 + (!txt->tx_EOL_terminated ? 1 : 0), "line");
	if (!txt->tx_EOL_terminated) {
		printf(" (not NL-terminated)");
	}
	if (lex_non_ASCII_cnt) {
		printf(", ");
		print_count(lex_non_ASCII_cnt, "non-ASCII character");
	}
	printf("\n");
}

static void read_file(const char *fname, struct text *txt) {
	txt->tx_fname = fname;
	txt->tx_pos = 0;
	txt->tx_start = Token_Array_Length();
	txt->tx_limit = Token_Array_Length();

	if (Is_New_Old_Separator(fname)) {
		do_separator(txt, fname);
	}
	else {	/* it is a real file */
		int ok = read_text(fname, txt);
		if (ok) {
			report_file(fname, txt);
		}
	}
}

void Read_Input_Files(int argc, const char *argv[]) {
	int n;

	Init_Text(argc);
	Init_Token_Array();

	/* Initially assume all texts to be new */
	Number_of_New_Texts = Number_of_Texts;

	/* Read the files */
	for (n = 0; n < Number_of_Texts; n++) {
		/* do one argument/file name */
		read_file(argv[n],&Text[n]);
	}

	int sep_present = (Number_of_Texts != Number_of_New_Texts);
	printf("Total input: ");
	print_count((!sep_present ? Number_of_Texts : Number_of_Texts - 1), "file");
	printf(" (%d new, %d old), ",
		Number_of_New_Texts,
		(!sep_present ? 0 :  Number_of_Texts - Number_of_New_Texts - 1)
	);
	print_count(Token_Array_Length() - 1, Token_Name);
	printf("\n\n");
}
