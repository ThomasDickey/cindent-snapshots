#ifdef HAVE_PUTENV
int
set_envvar(int f GCC_UNUSED, int n GCC_UNUSED)
{
    static TBUFF *var, *val;

    char *both;
    int rc;

    if ((rc = mlreply2("Environment variable: ", &var)) == ABORT) {
	/* EMPTY */ ;
    } else if ((rc = mlreply2("Value: ", &val)) == ABORT) {
	/* EMPTY */ ;
    } else if ((both = typeallocn(char, tb_length(var) + tb_length(val))) == 0) {
	rc = no_memory("set_envvar");
    } else {
	lsprintf(both, "%s=%s", tb_values(var), tb_values(val));
	putenv(both);	/* this will leak.  i think it has to. */
    }

    return rc;
}
#endif

static LINEPTR
alloc_LINE(BUFFER *bp)
{
    LINEPTR lp;

    if ((lp = bp->b_freeLINEs) != NULL) {
	bp->b_freeLINEs = lp->l_nxtundo;
    } else if ((lp = typealloc(LINE)) == NULL) {
	(void) no_memory("LINE");
    }
    return lp;
}