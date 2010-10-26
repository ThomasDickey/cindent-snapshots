const char *
_nc_altcharset_name(attr_t attr, chtype ch)
{
    const char *result = 0;

    if (attr & A_ALTCHARSET) {
	char *cp;
	char *found = 0;
	static const struct {
	    unsigned int val;
	    const char *name;
	} names[] =
	{
	    /* *INDENT-OFF* */
	    { 'l', "ACS_ULCORNER" },
	    { 'm', "ACS_LLCORNER" },
	    { 'k', "ACS_URCORNER" },
	    { 'j', "ACS_LRCORNER" },
	    { 't', "ACS_LTEE" },
	    { 'u', "ACS_RTEE" },
	    { 'v', "ACS_BTEE" },
	    { 'w', "ACS_TTEE" },
	    { 'q', "ACS_HLINE" },
	    { 'x', "ACS_VLINE" },
	    { 'n', "ACS_PLUS" },
	    { 'o', "ACS_S1" },
	    { 's', "ACS_S9" },
	    { '\0', (char *) 0 }
	/* *INDENT-OFF* */
	},
	    *sp;

	for (cp = acs_chars; cp[0] && cp[1]; cp += 2) {
	    if (ChCharOf(cp[1]) == ChCharOf(ch)) {
		found = cp;
		/* don't exit from loop - there may be redefinitions */
	    }
	}

	if (found != 0) {
	    ch = ChCharOf(*found);
	    for (sp = names; sp->val; sp++)
		if (sp->val == ch) {
		    result = sp->name;
		    break;
		}
	}
    }
    return result;
}
