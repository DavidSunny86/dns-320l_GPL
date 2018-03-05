/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005,2007 Oracle.  All rights reserved.
 *
 * $Id: os_abort.c,v 1.1.1.1 2009/04/20 07:15:26 jack Exp $
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __os_abort --
 *
 * PUBLIC: void __os_abort __P((void));
 */
void
__os_abort()
{
#ifdef HAVE_ABORT
	abort();			/* Try and drop core. */
#else
#ifdef SIGABRT
	(void)raise(SIGABRT);		/* Try and drop core. */
#endif
	exit(1);			/* Quit anyway. */
	/* NOTREACHED */
#endif
}
