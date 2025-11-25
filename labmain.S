# main.S
# Written by W Szczerek
# Copyright abandonded - this file is in the public domain.

######################################################################################
# This file is only a "hub" for your code to run in. You can reference the functions #
# that you wrote in the other files here.                                            #
######################################################################################

.globl handle_interrupt
.text

# Below is unused in lab1
handle_interrupt:
    jr ra
	
