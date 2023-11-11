#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <mark.h>
#include <bufpool.h>
#include <paging.h>

int write_bs(char *src, bsd_t bs_id, int page) {

  /* write one page of data from src
     to the backing store bs_id, page
     page.
  */
   STATWORD ps;
   disable(ps);

// total number of backing stores = 8
   if (bs_id < 0 || bs_id >= 8){
      restore(ps);
      return SYSERR;
   }

// total number of pages for each backing store = 128
   if (page < 0 || page >= 128){
      restore(ps);
      return SYSERR;
   }

   char * phy_addr = BACKING_STORE_BASE + bs_id*BACKING_STORE_UNIT_SIZE + page*NBPG;
   bcopy((void*)src, phy_addr, NBPG);

   restore(ps);
   return OK;

}