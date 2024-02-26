/*	This file is part of the software similarity tester SIM.
	Written by Dick Grune, VU, Amsterdam; dick@dickgrune.com
	$Id: sim.c,v 2.84 2020-08-14 16:56:53 dick Exp $
*/

//#include	"global.h"
//#include	"newargs.h"
#include	"token.h"
#include	"tokenarray.h"
#include	"text.h"
#include	"compare.h"
#include	"pass1.h"
//#include	"pass2.h"
//#include	"pass3.h"
#include	"percentages.h"
//#include	"stream.h"
//#include	"lang.h"

//#include	"Malloc.h"
//#include	"any_int.h"

int main(int argc, const char* argv[]) {
	Init_Language();
	
	Read_Input_Files(argc - 1, argv + 1);	/* turns files into texts */
	Compare_Files();		/* turns texts into runs */

	Print_Percentages();

	Free_Text();
	Free_Token_Array();

	return 0;
}
