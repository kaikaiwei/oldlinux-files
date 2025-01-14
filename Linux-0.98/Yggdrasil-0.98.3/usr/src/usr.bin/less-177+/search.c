/*
 * Routines to search a file for a pattern.
 */

#include "less.h"
#include "position.h"
#include "cmd.h"
#include "regexp.h"

extern int sigs;
extern int how_search;
extern int caseless;
extern int linenums;
extern int jump_sline;
extern int mca;

regexp *Last_regexp = NULL;		/* last search pattern */
int Last_pattern_is_caseless;

/*
 * Search for the n-th occurrence of a specified pattern, 
 * either forward or backward.
 * Return the number of matches not yet found in this file
 * (that is, n minus the number of matches found).
 * Return -1 if the search should be aborted.
 * Caller may continue the search in another file 
 * if less than n matches are found in this file.
 */
	public int
search(search_type, pattern, n)
	int search_type;
	char *pattern;
	int n;
{
	POSITION pos, linepos, oldpos;
	register char *p;
	register char *q;
	register int goforw;
	register int want_match;
	char *line;
	int linenum;
	int line_match;
	static regexp *regexp = NULL;
	static char lpbuf[100];

	/*
	 * Extract flags and type of search.
	 */
	goforw = (SRCH_DIR(search_type) == SRCH_FORW);
	want_match = !(search_type & SRCH_NOMATCH);

	if (pattern != NULL && *pattern != '\0' &&
	    (Last_pattern_is_caseless = caseless))
	{
		/*
		 * Search will ignore case, unless
		 * there are any uppercase letters in the pattern.
		 */
		for (p = pattern;  *p != '\0';  p++)
			if (*p >= 'A' && *p <= 'Z')
			{
				Last_pattern_is_caseless = 0;
				break;
			}
	}

	if (pattern == NULL || *pattern == '\0') {
		/*
		 * A null pattern means use the previous pattern.
		 */
		if (regexp == NULL) {
			error("No previous regular expression", NULL_PARG);
			return (-1);
		}
	}
	else	{
		if (regexp) free (regexp);
		if ((regexp = regcomp (pattern)) == NULL) {
			error("bad regular expression", NULL_PARG);
			return (-1);
		}
		Last_regexp = regexp;
	}

	/*
	 * Save a copy so that we can use it later to highlight the pattern
	 * on the screen.
	 */
	if (pattern != NULL && *pattern != '\0')
	{
		strcpy(lpbuf, pattern);
	}

	/*
	 * Figure out where to start the search.
	 */
	if (empty_screen())
	{
		/*
		 * Start at the beginning (or end) of the file.
		 * (The empty_screen() case is mainly for 
		 * command line initiated searches;
		 * for example, "+/xyz" on the command line.)
		 */
		if (goforw)
			pos = ch_zero();
		else 
		{
			pos = ch_length();
			if (pos == NULL_POSITION)
				pos = ch_zero();
		}
	} else 
	{
		if (how_search)
		{
			if (goforw)
				linenum = BOTTOM_PLUS_ONE;
			else
				linenum = TOP;
			pos = position(linenum);
		} else
		{
			linenum = adjsline(jump_sline);
			pos = position(linenum);
			if (goforw)
				pos = forw_raw_line(pos, (char **)NULL);
		}
	}

	if (pos == NULL_POSITION)
	{
		/*
		 * Can't find anyplace to start searching from.
		 */
		error("Nothing to search", NULL_PARG);
		return (-1);
	}

	linenum = find_linenum(pos);
	oldpos = pos;
	for (;;)
	{
		/*
		 * Get lines until we find a matching one or 
		 * until we hit end-of-file (or beginning-of-file 
		 * if we're going backwards).
		 */
		if (sigs)
			/*
			 * A signal aborts the search.
			 */
			return (-1);

		if (goforw)
		{
			/*
			 * Read the next line, and save the 
			 * starting position of that line in linepos.
			 */
			linepos = pos;
			pos = forw_raw_line(pos, &line);
			if (linenum != 0)
				linenum++;
		} else
		{
			/*
			 * Read the previous line and save the
			 * starting position of that line in linepos.
			 */
			pos = back_raw_line(pos, &line);
			linepos = pos;
			if (linenum != 0)
				linenum--;
		}

		if (pos == NULL_POSITION)
		{
			/*
			 * We hit EOF/BOF without a match.
			 */
			return (n);
		}

		/*
		 * If we're using line numbers, we might as well
		 * remember the information we have now (the position
		 * and line number of the current line).
		 * Don't do it for every line because it slows down
		 * the search.  Remember the line number only if
		 * we're "far" from the last place we remembered it.
		 */
		if (linenums && abs(pos - oldpos) > 1024)
		{
			add_lnum(linenum, pos);
			oldpos = pos;
		}

		if (Last_pattern_is_caseless)
		{
			/*
			 * If this is a caseless search, convert 
			 * uppercase in the input line to lowercase.
			 * While we're at it, remove any backspaces
			 * along with the preceding char.
			 * This allows us to match text which is 
			 * underlined or overstruck.
			 */
			for (p = q = line;  *p != '\0';  p++, q++)
			{
				if (*p >= 'A' && *p <= 'Z')
					/* Convert uppercase to lowercase. */
					*q = *p + 'a' - 'A';
				else if (q > line && *p == '\b')
					/* Delete BS and preceding char. */
					q -= 2;
				else
					/* Otherwise, just copy. */
					*q = *p;
			}
		}

		/*
		 * Test the next line to see if we have a match.
		 */
		line_match = regexec (regexp, line);

		/*
		 * We are successful if want_match and line_match are
		 * both true (want a match and got it),
		 * or both false (want a non-match and got it).
		 */
		if (((want_match && line_match) || (!want_match && !line_match)) &&
		      --n <= 0)
			/*
			 * Found the line.
			 */
			break;
	}

	if (mca == A_F_SEARCH || mca == A_B_SEARCH) {
		/*
		 * force refresh for new search. This will make the found
		 * string displayed with proper attibutes.
		 */
		pos_clear();
	}
	jump_loc(linepos, jump_sline);
	return (0);
}
