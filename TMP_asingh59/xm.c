/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>
#define bs_check(bs_id) (bs_id < 0 || bs_id >= 8)
#define virtno_check(virt_no) (virt_no < 4096)
#define page_check(page_no) (page_no < 1 || page_no > 256)


/*-------------------------------------------------------------------------
 * xmmap - xmmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
  STATWORD        ps;
  disable(ps);

  if(bs_check((int)source) || virtno_check(virtpage) || page_check(npages)){
    restore(ps);
    return SYSERR;
  }

  if(bsm_tab[source].bs_pvt_heap == 1){
    restore(ps);
    return SYSERR;
  }

  int bsm_map_status = bsm_map(currpid,virtpage,source,npages);
	if (bsm_map_status == SYSERR){
      restore(ps);
      return SYSERR;
	}	
  else{
    restore(ps);
    return OK;
  }	

}



/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage)
{
  STATWORD        ps;
  disable(ps);

	if(!virtno_check(virtpage))
	{
 	  int unmap_status = bsm_unmap(currpid,virtpage,0);
    if (unmap_status != SYSERR){
      restore(ps);
      return(OK);	
    }	
	}

	restore(ps);
	return SYSERR;
 
}