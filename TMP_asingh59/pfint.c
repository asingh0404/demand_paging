#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

SYSCALL pfint() {
    STATWORD ps;
    disable(ps);

    // Read the virtual address that caused the page fault
    unsigned long faulted_addr = read_cr2();
    virt_addr_t *virt_addr = (virt_addr_t*)&faulted_addr;

    // Extract the page directory and page table offsets from the virtual address
    unsigned int pd_offset = virt_addr->pd_offset;
    unsigned int pt_offset = virt_addr->pt_offset;

    // Get the current process's page directory base register (pdbr)
    pd_t *pd_entry = proctab[currpid].pdbr + pd_offset * sizeof(pd_t);

    // Calculate the address of the page table entry
    pt_t *pt_entry = (pt_t*)(pd_entry->pd_base * NBPG + pt_offset * sizeof(pt_t));

    // Handle the page directory entry
    handle_page_directory(pd_entry);

    // Handle the page table entry
    handle_page_table(pt_entry, faulted_addr);

    // Update the page directory base register and restore interrupts
    write_cr3(proctab[currpid].pdbr);
    restore(ps);
    return OK;
}

void handle_page_directory(pd_t *pd_entry) {
    // Check if the page directory entry is not present
    if (!pd_entry->pd_pres) {
        int new_fr_num;
        get_frm(&new_fr_num);

        // Update information in the frame table for the new page directory

        frm_tab[new_fr_num].fr_status = FRM_MAPPED;
        frm_tab[new_fr_num].fr_type = FR_TBL;
        frm_tab[new_fr_num].fr_pid = currpid;

        // Define a structure for page directory entry initialization values
        pd_t pd_entry_init = {
            .pd_pres = 1,
            .pd_write = 1,
            .pd_user = 0,
            .pd_pwt = 0,
            .pd_pcd = 0,
            .pd_acc = 0,
            .pd_mbz = 0,
            .pd_fmb = 0,
            .pd_global = 0,
            .pd_avail = 0,
            .pd_base = FRAME0 + new_fr_num
        };

        // Assign the initialization values to the new page directory entry
        *pd_entry = pd_entry_init;

        // Define a structure for page table entry initialization values
        pt_t pt_entry_init = {
            .pt_pres = 0,
            .pt_write = 0,
            .pt_user = 0,
            .pt_pwt = 0,
            .pt_pcd = 0,
            .pt_acc = 0,
            .pt_dirty = 0,
            .pt_mbz = 0,
            .pt_global = 0,
            .pt_avail = 0,
            .pt_base = 0
        };

        // Initialize the new page directory entries
        int i;
        pt_t *pt_entry = (pt_t*)(pd_entry->pd_base * NBPG);
        for (i = 0; i < NFRAMES; i++) {
            pt_entry[i] = pt_entry_init;
        }
    }
}

void handle_page_table(pt_t *pt_entry, unsigned long vaddr) {
    // Check if the page table entry is not present
    if (!pt_entry->pt_pres) {
        int new_pt_num;
        get_frm(&new_pt_num);
        int *p = &new_pt_num;
        append_pr_queue(p);

        // Update information in the frame table for the new page table

        frm_tab[new_pt_num].fr_status = FRM_MAPPED;
        frm_tab[new_pt_num].fr_type = FR_PAGE;
        frm_tab[new_pt_num].fr_pid = currpid;
        frm_tab[new_pt_num].fr_vpno = vaddr / NBPG;

        // Increment the reference count of the page directory entry's frame
        frm_tab[pt_entry->pt_base - FRAME0].fr_refcnt++;

        // Get information about the backing store and read the page from it
        int bs_id, pageth;
        bsm_lookup(currpid, vaddr, &bs_id, &pageth);
        read_bs((char*)((FRAME0 + new_pt_num) * NBPG), bs_id, pageth);

        // Update information in the page table entry for the new page
        pt_entry->pt_pres = 1;
        pt_entry->pt_write = 1;
        pt_entry->pt_base = FRAME0 + new_pt_num;
    }
}