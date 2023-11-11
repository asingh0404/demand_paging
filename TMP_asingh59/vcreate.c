/* vcreate.c - vcreate */
    
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>

/*
static unsigned long esp;
*/

LOCAL	newpid();
/*------------------------------------------------------------------------
 *  create  -  create a process to start running a procedure
 *------------------------------------------------------------------------
 */
SYSCALL vcreate(procaddr,ssize,hsize,priority,name,nargs,args)
	int	*procaddr;		/* procedure address		*/
	int	ssize;			/* stack size in words		*/
	int	hsize;			/* virtual heap size in pages	*/
	int	priority;		/* process priority > 0		*/
	char	*name;			/* name (for debugging)		*/
	int	nargs;			/* number of args that follow	*/
	long	args;			/* arguments (treated like an	*/
					/* array in the code)		*/
{
	struct mblock *freemem_block;
	STATWORD 	ps;
	disable(ps);

	if (hsize <= 0 || hsize > 16*128){
		// 16 backing stores and each has 128 pages
		restore(ps);
		return(SYSERR);
	}

	int bs_num;
	int pid;
	pid = create(procaddr,ssize,priority,name,nargs,args);
	
	/* sanity check to get backing store */
	if (get_bsm(&bs_num) == SYSERR)
	{
		restore(ps);
		return SYSERR;
	}
	
	int map_status = bsm_map(pid,4096,bs_num,hsize);

	if (map_status == SYSERR)
	{
		restore(ps);
		return SYSERR;
	}

	bsm_tab[bs_num].bs_pvt_heap = 1;

	freemem_block = BACKING_STORE_BASE + (bs_num * BACKING_STORE_UNIT_SIZE); 	
	freemem_block->mlen = hsize * NBPG;
	freemem_block->mnext = NULL;

	// proctab[pid].store = bs_num;
	// proctab[pid].vhpno = 4096;
	proctab[pid].vhpnpages = hsize;
	proctab[pid].vmemlist->mnext = 4096 * NBPG;
	
	restore(ps);	
	return pid;
}

/*------------------------------------------------------------------------
 * newpid  --  obtain a new (free) process id
 *------------------------------------------------------------------------
 */
LOCAL	newpid()
{
	int	pid;			/* process id to return		*/
	int	i;

	for (i=0 ; i<NPROC ; i++) {	/* check all NPROC slots	*/
		if ( (pid=nextproc--) <= 0)
			nextproc = NPROC-1;
		if (proctab[pid].pstate == PRFREE)
			return(pid);
	}
	return(SYSERR);
}