#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

SYSCALL release_bs(bsd_t bs_id) {

  STATWORD ps;
  disable(ps);

  /* release the backing store with ID bs_id */

  /* if a process other than the current process tries to free the backing store or 
     if a process doesn't have a private heap tries to free the backing store 
     return system error as these operations are not allowed. */

  if (bsm_tab[bs_id].bs_pid != currpid || bsm_tab[bs_id].bs_pvt_heap == 0){

    restore(ps);
    return SYSERR;
  }

  /* If the backing store has already been unmapped, we return OK as there is no need to free the backing store */
  // if (bsm_tab[bs_id].bs_status == BSM_UNMAPPED){
  //   return OK;
  // }

  free_bsm(bs_id);
  restore(ps);
  return OK;

}