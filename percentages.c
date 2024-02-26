/*	This file is part of the software similarity tester SIM.
	Written by Dick Grune, VU, Amsterdam; dick@dickgrune.com
	$Id: percentages.c,v 1.41 2020-08-14 16:56:52 dick Exp $
*/

#include	"percentages.h"

#include	"global.h"
#include	"Malloc.h"

struct match {
	struct match *ma_next;
	const char *ma_fname0;
	const char *ma_fname1;
	size_t ma_size;			/* # tokens of file 0 found in file 1 */
	size_t ma_size0;		/* # tokens in file 0 */
};

static struct match *match_list = 0;	/* to be allocated by new() */

static void
do_add_to_percentages(
    int rec_level, const struct text *txt0, const struct text *txt1,
    size_t size);
static void print_perc_info(const struct match *m);

static float
match_percentage(const struct match *m) {
	return (((float)m->ma_size)/((float)m->ma_size0));
}

void
Add_To_Percentages(
	const struct text *txt0, const struct text *txt1, size_t size
) {
	/* percentages are only meaningful between different files */
	if (txt0 == txt1) return;
	do_add_to_percentages(0, txt0, txt1, size);
}

/* The important point to notice is that when a certain combination of two
   texts turns up as a run, all runs with that combination will follow
   immediately, and once a different combination has appeared the first one
   will never return.
*/

static void
print_and_remove_match_list(void) {
	while (match_list) {
		struct match *m = match_list;
		match_list = m->ma_next;

		print_perc_info(m);
		Free(m);
	}
}

static void
do_add_to_percentages(
    int rec_level, const struct text *txt0, const struct text *txt1, size_t size
) {
	/* look up the (txt0, txt1) combination in the match list */
	struct match *m = match_list;
	while (m) {
		if (	m->ma_fname0 == txt0->tx_fname
		&&	m->ma_fname1 == txt1->tx_fname
		) {
			/* found it; now update it */
			m->ma_size += size;

			return;
		}
		m = m->ma_next;
	}

	{	/* it's not there; create a new entry, but tread carefully */
		m = try_new(struct match);
		if (m == 0) {
			/* Normally this should not happen. Freeing the
			   forward_reference[] and the last_index[] tables
			   should free ample memory to store the administration
			   of the runs found.  But the tables are linear in the
			   input length and the number of runs is more or less
			   quadratic, so for massive comparisons the runs may
			   overtake the tables.
			   We then free memory by flushing collected results,
			   sacrificing the sorting.
			*/
			/* try to recover */
			if (rec_level > 0) {
				/* but only one chance, to prevent looping */
				Out_Of_Memory_Exit(0);
				/*NOTREACHED*/
			}
			static int message_given;
			if (!message_given) {
				printf(">>>> OUTPUT NOT SORTED <<<<\n");
				message_given = 1;
			}
			print_and_remove_match_list();
			/* and try again */
			do_add_to_percentages(1, txt0, txt1, size);
			return;
		}

		m->ma_next = match_list;
		match_list = m;

		m->ma_fname0 = txt0->tx_fname;
		m->ma_fname1 = txt1->tx_fname;
		m->ma_size = size;
		m->ma_size0 = txt0->tx_limit - txt0->tx_start;
	}

}

static int Threshold_Percentage = 1;

static void print_perc_info(const struct match *m) {
	int mp = (int)(match_percentage(m)*100.0 + 0.5 /* rounding */);

	if (mp >= Threshold_Percentage) {
		printf("%s consists for %d %% of %s material\n", m->ma_fname0, mp, m->ma_fname1);
	}
}

static void
print_and_remove_perc_info_for_top_file(struct match **m_hook) {
	struct match *m = *m_hook;
	const char *fname = m->ma_fname0;

	print_perc_info(m);		/* always print main contributor */
	*m_hook = m->ma_next;
	Free(m);

	/* This is a horrible piece of code that should be rewritten.
	   It works only because initially m_hook points to match_list,
	   so removing entries with the same file name updates match_list,
	   whereas as soon as another file name is met, m_hook starts pointing
	   to the next field in the previous link,
	   so removing entries with the same file name just removes the entry.
	   Sneaky.
	*/
	while ((m = *m_hook)) {
		if (m->ma_fname0 == fname) {
			/* print subsequent contributors only if not
			   suppressed by -P
			*/
			print_perc_info(m);
			/* remove the struct */
			*m_hook = m->ma_next;
			Free(m);
		} else {
			/* skip the struct */
			m_hook = &m->ma_next;
		}
	}
}

static void
print_match_list(void) {
	/* destroys the match list while printing */
	while (match_list) {
		print_and_remove_perc_info_for_top_file(&match_list);
	}
}

void Print_Percentages(void) {
	print_match_list();
}
