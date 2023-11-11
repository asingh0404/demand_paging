#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>
#include <stdbool.h>

extern int page_replace_policy;

/* 
   Implements the page replacement policy based on either Second-Chance (SC)
   or Aging. Disables interrupts for atomicity during the policy execution.
   Parameters:
   None.
   Returns:
   The frame ID selected for replacement according to the policy.
*/
int pr_policy() {
    STATWORD ps;    // Save the current interrupt state
    disable(ps);    // Disable interrupts for atomicity

    int frameid = -1;     // Initialize the frame ID to -1 (indicating no frame selected yet)
    int tmpprev = -1;   // Initialize temporary previous frame ID to -1
    int current = pr_qhead;  // Start traversal from the head of the page replacement queue
    int prev = -1;      // Initialize previous frame ID to -1

    // Iterate through the page replacement queue
    while (current != -1) {
        // Calculate the virtual address using the current frame's virtual page number
        unsigned long vaddr = frm_tab[current].fr_vpno * NBPG;
        virt_addr_t *virtual_add = (virt_addr_t*)&vaddr;
        unsigned int vpd_offset = virtual_add->pd_offset;
        unsigned int vpt_offset = virtual_add->pt_offset;

        // Access the page directory and page table entries for the virtual address
        pd_t *pgdir_entry = proctab[currpid].pdbr + vpd_offset * sizeof(pd_t);
        pt_t *pgtbl_entry = (pt_t*)(pgdir_entry->pd_base * NBPG + vpt_offset * sizeof(pt_t));

        // Check the page replacement policy
        if (page_replace_policy == SC) {
            // Second-Chance policy: Check and update the access bit of the page table entry

            if (pgtbl_entry->pt_acc == 1) {
                pgtbl_entry->pt_acc = 0;  // Reset the access bit
            } else if (pgtbl_entry->pt_acc == 0) {
                // If the access bit is not set, remove the current frame from the queue
                if (prev == -1) {
                    pr_qhead = pr_qtab[current].next;
                } else {
                    pr_qtab[prev].next = pr_qtab[current].next;
                }
                frameid = current;  // Set the selected frame ID for replacement
                break;
            }
        } else {  // Aging policy
            // Update the frame's age based on the access bit of the page table entry

            pr_qtab[current].fr_age = (pr_qtab[current].fr_age >> 1) + (pgtbl_entry->pt_acc << 7);
            
            // If the frame's age is less than the current selected frame's age, update the selection
            if (pr_qtab[current].fr_age < pr_qtab[frameid].fr_age) {
                tmpprev = prev;
                frameid = current;  // Update the selected frame ID
            }
        }

        // Update traversal pointers
        prev = current;
        current = pr_qtab[current].next;
    }

    // If no frame was selected, choose the head of the queue
    if (frameid == -1) {
        frameid = pr_qhead;
        pr_qhead = pr_qtab[pr_qhead].next;
    } else if (tmpprev != -1) {
        // Remove the selected frame from the middle of the queue
        pr_qtab[tmpprev].next = pr_qtab[frameid].next;
    }
    pr_qtab[frameid].next = -1;  // Set the next pointer of the selected frame to -1

    restore(ps);    // Restore interrupts to their previous state
    return frameid;   // Return the selected frame ID for replacement
}

/* 
Appends a frame to the end of the page replacement queue.
If the queue is empty, the new frame becomes the head.
Disables interrupts to ensure atomicity while modifying the queue.
Parameters:
  - frameid: Pointer to the frame identifier to be appended to the queue. 
*/
void append_pr_queue(int *frameid) {
    STATWORD ps;
    disable(ps);  // Disable interrupts to ensure atomicity

    // If the queue is empty, set the head to the new frame
    if (pr_qhead == -1) {
        pr_qhead = *frameid;
    } else {
        int current = pr_qhead;

        // Traverse the queue to find the end
        while (pr_qtab[current].next != -1) {
            current = pr_qtab[current].next;
        }

        // Append the new frame to the end of the queue
        pr_qtab[current].next = *frameid;
        pr_qtab[*frameid].next = -1;
    }

    restore(ps);  // Restore interrupts
}


/* 
   Initializes the page replacement queue (pr_queue) data structure. 
   Assigns initial values to each entry in the queue.
   Parameters:
   None.
   Returns:
   None.
*/

void init_page_replace() {
    int i = 0;

    // Iterate through the page replacement queue
    for (i = 0; i < NFRAMES; i++) {
        pr_qtab[i].frameid = i;   // Assign the frame ID to the current queue entry
        pr_qtab[i].fr_age = 0;  // Initialize the age of the frame to 0
        pr_qtab[i].next = -1;   // Initialize the next pointer to -1 (end of the queue)
    }
}