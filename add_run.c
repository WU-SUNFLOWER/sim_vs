/*	This file is part of the software similarity tester SIM.
	Written by Dick Grune, VU, Amsterdam; dick@dickgrune.com
	$Id: add_run.c,v 2.22 2020-08-14 16:56:50 dick Exp $
*/

#include	"add_run.h"

#include	"global.h"
#include	"text.h"
#include	"runs.h"
#include	"percentages.h"

/* Sends the run info to Add_To_Percentages or to Add_To_Runs. */
void
Add_Run(struct text *txt0, size_t i0,
	struct text *txt1, size_t i1,
	size_t size
) {
	Add_To_Percentages(txt0, txt1, size);
}
