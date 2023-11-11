#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>
#include <stdbool.h>

extern bool debug_option;
extern int page_replace_policy;

/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */

// Define a structure to hold the initialization values for frm_tab
struct FrameInitValues {
    int fr_status;  // Status of the frame (e.g., MAPPED, UNMAPPED)
    int fr_pid;     // Process ID associated with the frame
    int fr_vpno;    // Virtual page number mapped to the frame
    int fr_refcnt;  // Reference count indicating the number of references to the frame
    int fr_type;    // Type of the frame (e.g., PAGE, TABLE)
    int fr_dirty;   // Flag indicating whether the frame has been modified

};

/* Function: init_frm
   -------------------
   Initializes the frame table (frm_tab) data structure with default values.
   Parameters:
   None.
   Returns:
   OK if the initialization is successful.
*/

SYSCALL frame_table_map() {
    STATWORD ps;
    disable(ps);

    // Define initialization values for frm_tab
    struct FrameInitValues initValues = {
        .fr_status = FRM_UNMAPPED,  // Initial status is set to UNMAPPED
        .fr_pid = -1,               // Initial process ID is set to -1 (no process)
        .fr_vpno = 0,               // Initial virtual page number is set to 0
        .fr_refcnt = 0,             // Initial reference count is set to 0
        .fr_type = FR_PAGE,         // Initial frame type is set to PAGE
        .fr_dirty = 0               // Initial dirty flag is set to 0
    };

    int i;
    for (i = 0; i < NFRAMES; i++) {
        // Copy initialization values to frm_tab[i]
        frm_tab[i].fr_status = initValues.fr_status;
        frm_tab[i].fr_pid = initValues.fr_pid;
        frm_tab[i].fr_vpno = initValues.fr_vpno;
        frm_tab[i].fr_refcnt = initValues.fr_refcnt;
        frm_tab[i].fr_type = initValues.fr_type;
        frm_tab[i].fr_dirty = initValues.fr_dirty;
    }

    restore(ps);
    return OK;
}



/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
/* Function: get_frm
   ------------------
   Gets a free frame from the frame table (frm_tab) or invokes the page replacement policy
   to obtain a frame when no free frames are available.
   Parameters:
   - int* avail: Pointer to an integer where the index of the obtained frame will be stored.
   Returns:
   OK if a frame is successfully obtained, SYSERR otherwise.
*/

SYSCALL get_frm(int* avail) {
    STATWORD ps;
    disable(ps);

    int i;

    // Iterate through the frame table to find an unmapped frame
    for (i = 0; i < NFRAMES; i++) {
        if (frm_tab[i].fr_status == FRM_UNMAPPED) {
            *avail = i;  // Store the index of the available frame
            restore(ps);
            return OK;   // Return success
        }
    }

    // If no unmapped frame is found, invoke the page replacement policy
    int frame_id = pr_policy();

    // If the page replacement policy returns a valid frame and freeing is successful
    if (frame_id > -1 && free_frm(frame_id) == OK) {
        *avail = frame_id;  // Store the index of the obtained frame
        restore(ps);
        return OK;          // Return success
    }

    restore(ps);
    return SYSERR;  // Return system error if no frame can be obtained
}


/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
/* Function: free_frm
   -------------------
   Frees a frame in the frame table (frm_tab) and performs necessary cleanup.
   Parameters:
   - int i: Index of the frame to be freed.
   Returns:
   OK if the frame is successfully freed, SYSERR otherwise.
*/

SYSCALL free_frm(int i) {
    STATWORD ps;
    disable(ps);

    // Validate frame index and type
    if (i < 0 || i >= NFRAMES || frm_tab[i].fr_type != FR_PAGE) {
        restore(ps);
        return SYSERR;  // Return system error if the frame is invalid
    }

    // Calculate virtual address using the frame's vpno
    unsigned long vaddr = frm_tab[i].fr_vpno * NBPG;
    virt_addr_t *virtual_add = (virt_addr_t*)&vaddr;
    unsigned int vpd_offset = virtual_add->pd_offset;
    unsigned int vpt_offset = virtual_add->pt_offset;

    // Get pointers to the page directory and page table entries
    pd_t *pgdir_entry = proctab[frm_tab[i].fr_pid].pdbr + vpd_offset * sizeof(pd_t);
    pt_t *pgtbl_entry = (pt_t*)(pgdir_entry->pd_base * NBPG + vpt_offset * sizeof(pt_t));

    // Write the frame content back to the backing store
    write_bs((i + FRAME0) * NBPG, proctab[frm_tab[i].fr_pid].store, frm_tab[i].fr_vpno - proctab[frm_tab[i].fr_pid].vhpno);

    // Reset the present bit of the page table entry
    pgtbl_entry->pt_pres = 0;

    // Decrement the reference count of the corresponding page table frame
    frm_tab[pgdir_entry->pd_base - FRAME0].fr_refcnt--;

    // Unmap the page table frame if the reference count becomes zero
    if (frm_tab[pgdir_entry->pd_base - FRAME0].fr_refcnt == 0) {
        pgdir_entry->pd_pres = 0;

        // Reset the frame table entry for the page table frame
        frm_tab[pgdir_entry->pd_base - FRAME0] = (fr_map_t){FRM_UNMAPPED, FR_PAGE, -1, 4096};
    }

    restore(ps);
    return OK;  // Return success
}