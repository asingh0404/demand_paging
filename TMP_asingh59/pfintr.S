/* pfintr.S - pfintr */


    	   .text
pferrcode: .long 0
           .globl  pfintr,pferrcode 
pfintr:
	/* Step 1: Store error code in a global variable */
	popl pferrcode

    /* Step 2: Store flag register */
	pushfl 

    /* Step 3: Clear all interrupts */
    cli

    /* Step 4: Save all general registers */
	pushal

    /* Step 5: Page Fault Handling (Call pfint or your page fault handler) */
	call pfint

    /* Step 6: Restore general registers */
	popal

    /* Step 7: Restore flag register */
	popfl 

    /* Step 8: Return from interrupt */
	iret
