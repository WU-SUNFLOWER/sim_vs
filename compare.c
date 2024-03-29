/*	This file is part of the software similarity tester SIM.
	Written by Dick Grune, VU, Amsterdam; dick@dickgrune.com
	$Id: compare.c,v 2.48 2020-08-14 16:56:50 dick Exp $
*/

#include	"compare.h"

#include	"global.h"
#include	"text.h"
#include	"token.h"
#include	"tokenarray.h"
#include	"hash.h"
#include	"properties.h"
#include	"add_run.h"
#include	"debug.par"
#include    "settings.h"
							/* LOCAL VARIABLES */
static size_t beginning_of_text;
static size_t beginning_of_old_text;
static size_t end_of_text;

							/* RANGE HANDLING */
struct range {
	size_t rg_start;
	size_t rg_limit;
	int rg_sticky;
};

static int
in_range(size_t i, const struct range *rg) {
	if (rg->rg_start <= rg->rg_limit) {
		/* one single range */
		return (rg->rg_start <= i && i < rg->rg_limit);
	} else {
		/* looped-around range */
		return (rg->rg_start <= i && i < end_of_text
			|| beginning_of_text <= i && i < rg->rg_limit);
	}
}

static int
is_empty_range(const struct range *rg) {
	return rg->rg_start == rg->rg_limit;
}

							/* COMPARE FILES */
static void compare_one_text(int n, struct range *rg);
static void compare_one_on_one(int n, int m, struct range *rg);
static size_t lcs(
	struct text *txt0, size_t i0, struct range *rg,
	struct text **tx_bp, size_t *i_bp
);

/*	The overall structure of the routine compare_texts() is:

	for all new files
		for all texts it must be compared to
			for all positions in the new file
				for all positions in the text
					for ever increasing sizes
						try to match and keep the best
*/

static void compare_texts(void) {
	/*
	int n;
	//all new texts
	for (
		n = 0; n < Number_of_New_Texts; n++
	) {
		struct range range;

		// construct default range
		beginning_of_text = Text[0].tx_start;
		beginning_of_old_text = Text[Number_of_New_Texts-1].tx_limit;
		end_of_text = Text[Number_of_Texts-1].tx_limit;
		range.rg_start = Text[n].tx_start + 1;
		range.rg_limit = end_of_text;
		range.rg_sticky = 0;

		if (is_empty_range(&range)) continue;

		// compare the files
		if (0) {
			// over the range in steps of one
			int m;

			for (m = n; m < Number_of_Texts; m++) {
				compare_one_on_one(n, m, &range);
			}
			for (m = 0; m < n; m++) {
				compare_one_on_one(n, m, &range);
			}
		}
		else {
			// the whole range in one action
			compare_one_text(n, &range);
		}
	}*/
	struct range range;
	end_of_text = Text[1].tx_limit;
	range.rg_start = Text[0].tx_start + 1;
	range.rg_limit = end_of_text;
	range.rg_sticky = 0;
	compare_one_on_one(0, 1, &range);
}

static void
compare_one_on_one(
	int n,				/* 被查重的新文本 */
	int m,				/* 已有的文本 */
	struct range *rg		/* pointer to search range */
) {
	const struct text *txt1 = &Text[m];
	if (!in_range(txt1->tx_start+1, rg)) return;

	/* construct private range consisting of Text[m] */
	struct range range_m;
	range_m.rg_start = txt1->tx_start;
	range_m.rg_limit = txt1->tx_limit;
	range_m.rg_sticky = 0;

	/* compare Text[n] and Text[m] */
	compare_one_text(n, &range_m);
}

static void
compare_one_text(
	int n,				/* index of text to be compared */
	struct range *rg		/* pointer to search range */
) {
	struct text *txt0 = &Text[n];
	size_t i0 = txt0->tx_start;

	while (	/* there is room for a run */
		i0 + Min_Run_Size <= txt0->tx_limit
	) {
		if (!May_Be_Start_Of_Run(Token_Array[i0])) {
			/* no point in looking; try the next token */
			i0++;
		} else {
			/* see if there really is a run */
			struct text *txt_run;
			size_t i_run;
			size_t run_size =
				lcs(txt0, i0, rg, &txt_run, &i_run);

			if (run_size) {
				/* run found; enter it */
#ifdef	DB_COMP
				fprintf(Debug_File,
					"Add_Run(%s, %d, %s, %d, %d)\n",
					txt0->tx_fname, i0,
					txt_run->tx_fname, i_run, run_size);
#endif
				Add_Run(txt0, i0, txt_run, i_run, run_size);
				/* and skip it */
				i0 += run_size;
			}
			else {
				/* we try our luck at the next token */
				i0++;
			}
		}
		if (rg->rg_sticky) {
			/* drag rg->rg_start along */
			rg->rg_start = i0 - 1;
		}
	}
}

static size_t
first_forward_ref_for(size_t i0, const struct range *rg) {
	size_t res = Forward_Reference(i0, i0);
	while (res && !in_range(res, rg)) {
		res = Forward_Reference(res, i0);
	}
	return res;
}

static size_t
lcs(	struct text *txt0,		/* 被查重的文本 */
	size_t i0,			/* starting pos. in txt0 */
	struct range *rg,		/* search range */
	/* two output parameters, set if return value > 0: */
	struct text **tx_bp,		/* output, text of best run */
	size_t *i_bp			/* starting pos. in text of best run */
) {
	/*	Finds the longest common substring (not subsequence) in:
			txt0, starting precisely at i0 and  
			all the text in the search range rg.
		Writes the position in tx_bp and i_bp and returns the size.
		Returns 0 if no common substring is found.
	*/
	size_t i1;
	size_t size_best = 0;

	if (!(txt0->tx_start <= i0 && i0 < txt0->tx_limit))
		Fatal("i0 not inside txt0");

	for (	i1 = first_forward_ref_for(i0, rg);
		i1 && in_range(i1, rg);
		i1 = Forward_Reference(i1, i0)
	) {
		/* i1 is always on the forward reference chain of i0 */

		/* Find the text txt1 into which i1 points. */
		/* by binary search */
		struct text *txt1;
		{	struct text *txt_b = &Text[0];
			struct text *txt_e = &Text[Number_of_Texts-1];
			while (txt_b < txt_e) {
				struct text *txt_m = txt_b + (txt_e-txt_b) / 2;
				if (i1 < txt_m->tx_limit) {
					txt_e = txt_m;
				} else {
					txt_b = txt_m + 1;
				}
			}
			txt1 = txt_b;
		}

		if (!(txt1->tx_start <= i1 && i1 < txt1->tx_limit))
			Fatal("i1 not inside txt1");

		size_t better_size = (size_best ? size_best+1 : Min_Run_Size);

		/* Are we looking at something better than we have got? */
		{	/* we compare backwards from the end of
			   a putative better match starting at i1
			*/
			size_t j0 = i0 + better_size - 1;
			size_t j1 = i1 + better_size - 1;

			/* would there be room for a better match? */
			if (	/* j0 still inside txt0 */
				j0 < txt0->tx_limit
			&&	/* j1 still inside txt1 */
				j1 < txt1->tx_limit
			&&	/* better 0 and better 1 don't overlap */
				(j0 < i1 || j1 < i0)
			) {
				/* yes, there is room enough for a match */

				/* see if the text matches for at least
				   better_size tokens
				*/
				/* since we have perfect forward references and
				   check backwards, we do not have to check the
				   last Min_Run_Size tokens:
				*/
				size_t cnt = better_size - Min_Run_Size;

				while (	cnt
				&&	Token_EQ(Token_Array[j0],
						 Token_Array[j1])
				) {
					cnt--, j0--, j1--;
				}

				if (cnt) {
					/* not all tokens matched,
					   so forget it */
					continue;
				}
			} else {
				/* no, there is not enough room for a better
				   match, so forget it */
				   continue;
			}
		}

		/* Yes, we are looking at a better match;
		   how long can we make it?
		*/
		size_t new_size = better_size;
		{	/* extending forwards */
			size_t j0 = i0 + better_size;
			size_t j1 = i1 + better_size;

			while (	/* j0 still inside txt0 */
				j0 < txt0->tx_limit
			&&	/* j1 still inside txt1 */
				j1 < txt1->tx_limit
			&&	/* j0 and j1 don't overlap */
				(j0 < i1 || j1 < i0)
			&&	/* tokens are the same */
				Token_EQ(Token_Array[j0], Token_Array[j1])
			) {
				j0++, j1++, new_size++;
			}
		}

		/* Offer the run to the Language module which may
		   reject it or may cut its tail.
		*/
		new_size = Best_Run_Size(&Token_Array[i0], new_size);

		if (	/* we still have an acceptable run */
			new_size >= Min_Run_Size
		&&	/* it is still better than what we had */
			new_size > size_best
		) {
			/* record it */

			*tx_bp = txt1;
			*i_bp = i1;
			size_best = new_size;
		}
		/* and see if it can be improved with a different i1 */
	}

	return size_best;
}

void
Compare_Files(void) {
	Make_Forward_References();
	compare_texts();
	Free_Forward_References();
}
