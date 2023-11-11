#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

int get_bs(bsd_t bs_id, unsigned int npages) {

  STATWORD ps;
  disable(ps);

  if (bs_id < 0 || bs_id > 7){
    restore(ps);
    return SYSERR;
  }

  if (npages <= 0 || npages > 256){
    restore(ps);
    return SYSERR;
  }

  if (bsm_tab[bs_id].bs_pvt_heap == 1){
    restore(ps);
    return SYSERR;
  }

  if (bsm_tab[bs_id].bs_status == BSM_UNMAPPED){
    bsm_tab[bs_id].bs_status = BSM_MAPPED;
    bsm_tab[bs_id].bs_pid = currpid;

    restore(ps);
    return npages;
  }

  else{
    restore(ps);
    return bsm_tab[bs_id].bs_npages;
  }

  restore(ps);
  return SYSERR;
   				
}