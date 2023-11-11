#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL backing_store_map()
{
    STATWORD ps;
    disable(ps);

    int id;

    for(id = 0; id < 8; id++){

        bs_map_t bs_num = bsm_tab[id];
        bs_num.bs_status = BSM_UNMAPPED;
        bs_num.bs_pid = -1;
        bs_num.bs_vpno = 4096;
        bs_num.bs_npages = 0;
        bs_num.bs_sem = 0;
        bs_num.bs_pvt_heap = 0;

    }

    restore(ps);
    return OK;
			
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm(int* avail)
{
    STATWORD ps;
    disable(ps);

    int id;

    for(id = 0; id < 8; id++){
        if(bsm_tab[id].bs_status == BSM_UNMAPPED){
            *avail = id;
            restore(ps);
            return OK;
        }
    }
    kprintf("Backing Store Unavailable");
    restore(ps);
    return SYSERR;
	
}


/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
    STATWORD ps;
    disable(ps);

    if(i < 0 || i >= 8){
        restore(ps);
        return SYSERR;
    }

    if(bsm_tab[i].bs_status == BSM_UNMAPPED){
        restore(ps);
        return SYSERR;
    }

    if(bsm_tab[i].bs_pid != currpid){
        restore(ps);
        return SYSERR;
    }

    bs_map_t bs_num = bsm_tab[i];
    bs_num.bs_status = BSM_UNMAPPED;
    bs_num.bs_pid = -1;
    bs_num.bs_vpno = 4096;
    bs_num.bs_npages = 0;
    bs_num.bs_sem = 0;
    bs_num.bs_pvt_heap = 0;

    restore(ps);
    return OK;
	
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth)
{
	STATWORD 	ps;
	disable(ps);

    if (pid <= 0 || pid >= NPROC){
        restore(ps);
        return SYSERR;
    }

    // Define constants for bit positions
    const unsigned long PG_OFFSET_MASK = 0xFFF;      // Bits 0 to 11
    const unsigned long PT_OFFSET_MASK = 0x3FF000;   // Bits 12 to 21
    const unsigned long PD_OFFSET_MASK = 0xFFC00000; // Bits 22 to 31

    virt_addr_t virtual_addr;
    // Calculate page offset
    virtual_addr.pg_offset = vaddr & PG_OFFSET_MASK;

    // Calculate page table offset
    virtual_addr.pt_offset = (vaddr & PT_OFFSET_MASK) >> 12;

    // Calculate page directory offset
    virtual_addr.pd_offset = (vaddr & PD_OFFSET_MASK) >> 22;

    // Extracting page directory offset and page table offset
    int pd_offset = (int)virtual_addr.pd_offset;
    int pt_offset = (int)virtual_addr.pt_offset;

    // Combining page directory and page table offsets to get the virtual page number
    int vpno = (pd_offset << 10) | pt_offset;

    int id;
    for(id = 0; id < 8; id++){

        bs_map_t bs_num = bsm_tab[id];

        if( bs_num.bs_status == BSM_MAPPED && bs_num.bs_pid == pid){

            if( vpno >= bs_num.bs_vpno){
                *store = id;
                *pageth = vpno - bs_num.bs_vpno; 
                restore(ps); 
			    return OK;
            }
		}				
	}
	
	restore(ps);
	return SYSERR;
	
}


/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
    STATWORD ps;
    disable(ps);

    if(source < 0 || source > 7){
        restore(ps);
        return SYSERR;
    }

    if (pid <= 0 || pid >= NPROC){
        restore(ps);
        return SYSERR;
    }

    bs_map_t bs_num = bsm_tab[source];

    if (bs_num.bs_pid != pid && bs_num.bs_pvt_heap == 1){
        restore(ps);
        return SYSERR;
    }

    else{
		bs_num.bs_status = BSM_MAPPED;
		bs_num.bs_pid = pid;
		bs_num.bs_vpno = vpno;
		bs_num.bs_npages = npages;
		proctab[currpid].vhpno = vpno;
		proctab[currpid].store = source;
	
		restore(ps);
		return(OK);
    }

    restore(ps);
    return SYSERR;
}



/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno, int flag)
{
	STATWORD 	ps;
	disable(ps);
	
	int bs_id;
	int pageth;
    int mapping_found = 0;
	int i;

	for (i = 0; i < NFRAMES; i++)
	{
		if (frm_tab[i].fr_status == FRM_MAPPED && frm_tab[i].fr_pid == pid && frm_tab[i].fr_type == FR_PAGE)
		{
            bsm_lookup(pid, vpno*NBPG, &bs_id, &pageth);
			write_bs((i + NFRAMES) * NBPG, bs_id, pageth);
            mapping_found = 1;
			break;	
		}				
	}

    if (mapping_found == 1){
        bs_map_t bs_num = bsm_tab[bs_id]; 
        bs_num.bs_status = BSM_UNMAPPED;
        bs_num.bs_pid = -1;
        bs_num.bs_vpno = 4096;
        bs_num.bs_npages = 0;
        bs_num.bs_pvt_heap = 0;
    }

    restore(ps);
	return(OK);	
			
}
