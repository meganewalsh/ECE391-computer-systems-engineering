
.data					# section declaration

        # Useful offset constants for accessing members of a 
        # struct mp1_blink_struct structure
        LOCATION   = 0    
        ON_CHAR    = 2
        OFF_CHAR   = 3 
        ON_LENGTH  = 4
        OFF_LENGTH = 6
        COUNTDOWN  = 8
        STATUS     = 10
        NEXT       = 12

        STRUCT_SIZE = 16

# Pointer to head of list (initialized to NULL)
mp1_list_head:
        .long   0

.text					# section declaration

# Export the function symbol names

.global mp1_rtc_tasklet
.global mp1_ioctl

# void mp1_poke(void);
#
# Interface: Register-based arguments (not C-style)
#    Inputs: %cl  - The byte you wish to write
#            %eax - Offset from the start of video memory that you wish
#                   to write to
#   Outputs: Text-mode video screen is written to at location %eax with
#            the byte in %cl
# Registers: Clobbers EDX
mp1_poke:
        pushl %edx 
	movl    vmem_base_addr(,1),%edx
	movb    %cl,(%edx,%eax,1)
	popl %edx
	ret
# ------------------------------------------------------------------------------
# 	This function updates the state of the game.
# void mp1 rtc tasklet (unsigned long);
mp1_rtc_tasklet:

	# save old frame pointer, then point to new frame	
	pushl %ebp
	movl %esp, %ebp	
	# save callee-saved registers
	pushl %ebx
	pushl %edi
	pushl %esi

	movl mp1_list_head, %esi
	cmpl $0, %esi
	je error

loop:
 	# Decrement countdown
	decw COUNTDOWN(%esi)
  	# If countdown is 0, check status
	movw COUNTDOWN(%esi), %dx
	cmpw $0, %dx
	je check_status
	jmp cont_tasklet

check_status:
	movw STATUS(%esi), %bx
	cmpw $1, %bx
	je switch_status_to_zero
	cmpw $0, %bx
	je switch_status_to_one	

switch_status_to_zero:
	movw $0, STATUS(%esi)

  	# Put the opposite character out to video memory with call to poke
	movb OFF_CHAR(%esi), %cl
	movw LOCATION(%esi), %ax
	movzwl %ax, %eax
	imull $2, %eax 
	call mp1_poke

  	# Update countdown by copying value from opposite length field to countdown
	movw OFF_LENGTH(%esi), %dx
	movw %dx, COUNTDOWN(%esi)

	jmp cont_tasklet

switch_status_to_one:
	movw $1, STATUS(%esi)

  	# Put the opposite character out to video memory with call to poke
	movb ON_CHAR(%esi), %cl
	movw LOCATION(%esi), %ax
	movzwl %ax, %eax
	imull $2, %eax 
	call mp1_poke

  	# Update countdown by copying value from opposite length field to countdown
	movw ON_LENGTH(%esi), %dx
	movw %dx, COUNTDOWN(%esi)

	jmp cont_tasklet

cont_tasklet:
	# Move to next list element
	movl NEXT(%esi), %edx
	movl %edx, %esi

 	# Return when reached end of list
	cmpl $0, %esi
	jne loop
	jmp success

# ------------------------------------------------------------------------------
# 	 Serves as a dispatcher to determine which ioctl to execute. Must use
#        a jump table.
# int mp1_ioctl (unsigned long arg, unsigned long cmd)
mp1_ioctl:

	# ecx <-- cmd
	movl 8(%esp), %ecx

	# 0 and 3 are the boundaries for cmd
	cmpl $0, %ecx
	jb DEFAULT
	cmpl $3, %ecx
	ja DEFAULT
	# 4 is the size of the jumptable pointers
  	jmp *jumptable(, %ecx, 4)

 DEFAULT:
	movl $-1, %eax
	ret

 jumptable:
  	.long mp1_ioctl_add, mp1_ioctl_remove, mp1_ioctl_find, mp1_ioctl_sync

# ------------------------------------------------------------------------------
#	 Add a blinking location. Be sure to handle errors. Must fill in the
#        newly allocated structure, including the next field, before changing
#        the head of the list to point to the new structure.
# int mp1 ioctl add(unsigned long arg).	 
mp1_ioctl_add:

	# save old frame pointer, then point to new frame	
	pushl %ebp
	movl %esp, %ebp	
	# save callee-saved registers
	pushl %ebx
	pushl %edi
	pushl %esi

	# ebx <-- arg (original structure)
	movl 8(%ebp), %ebx
	cmpl $0, %ebx
	je error

  	# Dynamically allocate memory to store a copy of the structure
	pushl %eax
	pushl %ecx
	pushl %edx
	pushl $STRUCT_SIZE	# void* mp1 malloc(unsigned long size)
	call mp1_malloc		# returns address of new struct
	addl $4, %esp		# pop argument
	cmpl $0, %eax		# check malloc error (NULL address)
	je malloc_error
	movl %eax, %esi		# esi <-- new struct memory
	popl %edx
	popl %ecx
	popl %eax

	# verify valid location (0 and 1999 are the boundaries for location)
	movl LOCATION(%esi), %edx
	cmpl $0, %edx
	jb error
	cmpl $1999, %edx
	ja error

  	# Copy the structure
	pushl %eax
	pushl %ecx
	pushl %edx
	pushl $STRUCT_SIZE	# unsigned long mp1_copy_from_user (void *to, const void *from, unsigned long n)
	pushl 8(%ebp)
	pushl %esi
	call mp1_copy_from_user	# returns number of bytes failed to copy
	addl $12, %esp		# pop 3 arguments
	cmpl $0, %eax		# check copy error
	jne copy_free
	movl %eax, %edi
	popl %edx
	popl %ecx
	popl %eax
	movl %edi, %eax

  	# Set countdown equal to on_length field, set status to 1
	movl ON_LENGTH(%esi), %edx
	movl %edx, COUNTDOWN(%esi)
	movl $1, STATUS(%esi) 	

 	# Insert structure at head of linked list
	cmpl $0, mp1_list_head
	je insert_into_empty
	movl mp1_list_head, %edx
	movl %edx, NEXT(%esi)
	movl %esi, mp1_list_head

cont_add:
  	# Call poke -  Inputs: %cl (the byte you wish to write) & %eax (offset from the start of video memory that you wish to write to)
	movb ON_CHAR(%esi), %cl
	movw LOCATION(%esi), %ax
	movzwl %ax, %eax
	imull $2, %eax 
	call mp1_poke
	jmp success

insert_into_empty:
	movl %esi, mp1_list_head
	movl $0, NEXT(%esi)
	jmp cont_add

copy_free:
	pushl %esi
	call mp1_free
	addl $4, %esp		# pop 1 argument
	popl %edx
	popl %ecx
	popl %eax
	jmp error

malloc_error:
	popl %edx
	popl %ecx
	popl %eax
	popl %esi
	popl %edi
	popl %ebx
	movl $-1, %eax
	leave
	ret

success:
	popl %esi
	popl %edi
	popl %ebx
	movl $0, %eax
	leave
	ret

error:
	popl %esi
	popl %edi
	popl %ebx
	movl $-1, %eax
	leave
	ret

# ------------------------------------------------------------------------------
# 	 Removes a blinking location. Handle errors. Must remove the element
#        from the list before freeing it. Copying the next pointer into a
#        register is not sufficient since the tasklet could try to read the
#        structure after the call to fee.
# int mp1 ioctl remove(unsigned long arg)
mp1_ioctl_remove:

	# save old frame pointer, then point to new frame	
	pushl %ebp
	movl %esp, %ebp	
	# save callee-saved registers
	pushl %ebx
	pushl %edi
	pushl %esi

	cmpl $0, mp1_list_head
	je error

  	# Traverse the list. Find an element whose location field matches argument
	movl 8(%ebp), %edx
	call mp1_search
	cmpl $-1, %eax
	je error

	cmpl %eax, mp1_list_head
	je remove_head
	
  	# If exists, remove it, free its memory, return 0
	movl NEXT(%eax), %edx
	movl %edx, NEXT(%ecx)

free:
	pushl %eax
	call mp1_free
	addl $4, %esp
	jmp success

remove_head:
	movl NEXT(%eax), %edx
	movl %edx, mp1_list_head
	jmp free
        
# ------------------------------------------------------------------------------
# 	Get information about a location. Handle errors.
# int mp1 ioctl find(unsigned long arg) 
 mp1_ioctl_find:

	# save old frame pointer, then point to new frame	
	pushl %ebp
	movl %esp, %ebp	
	# save callee-saved registers
	pushl %ebx
	pushl %edi
	pushl %esi

	# esi <-- arg (8 is offset for argument)
	movl 8(%ebp), %edi
	# Validate pointer refers to valid stucture before reading from it
	cmpl $0, %edi
	je error

  	# Extract location, and search list for element with matching location
	movw LOCATION(%edi), %dx
	call mp1_search
	cmpl $-1, %eax
	je error
	movl %eax, %esi	

 	# Copy entire element to parameter's structure
	pushl $STRUCT_SIZE
	pushl %esi
	pushl %edi
	call mp1_copy_to_user	# unsigned long mp1_copy_to_user (void *to, const void *from, unsigned long n);
	addl $12, %esp		# pop 3 arguments
	cmpl $0, %eax		# check copy error
	jne error

  	# If no matching location, return -1. Else, return 0.
	jmp success

# -----------------------------------------------------------------------------
#	 Helper function for remove/find/sync. Searches a linked list for a
#        specific element that matches a particular location.
# dx <-- LOCATION
 mp1_search:

	# current in eax
	movl mp1_list_head, %eax
	# keep track of previous in ecx
	movl $0, %ecx
	jmp fLOOP

LOOP:	movl %eax, %ecx
	movl NEXT(%eax), %eax
fLOOP:	cmpl $0, %eax
	je DEFAULT
	movw LOCATION(%eax), %bx
	cmpw %bx, %dx
	jne LOOP
	ret

# ------------------------------------------------------------------------------
# 	Synchronize a new blink location with an existing one. Handle errors
# int mp1 ioctl sync(unsigned long arg)
mp1_ioctl_sync:

	# save old frame pointer, then point to new frame	
	pushl %ebp
	movl %esp, %ebp	
	# save callee-saved registers
	pushl %ebx
	pushl %edi
	pushl %esi

  	# Extract two integers from argument
	movl 8(%ebp), %edi	# second integer is cx
	movl 8(%ebp), %esi	# first integer is bx
	shrl $16, %esi		# shift 16 bits to get the 8 most sig bits	
	
  	# Search the list, looking for elements with locations that match integers
	movw %di, %dx
	call mp1_search
	cmpl $-1, %eax
	je error
	movl %eax, %edi

	movw %si, %dx
	call mp1_search
	cmpl $-1, %eax
	je error
	movl %eax, %esi

  	# Copy the timing information from first element to second element
	# on_length, off_length, countdown, status
	movw ON_LENGTH(%esi), %dx
	movw %dx, ON_LENGTH(%edi) 

	movw OFF_LENGTH(%esi), %dx
	movw %dx, OFF_LENGTH(%edi) 

	movw COUNTDOWN(%esi), %dx
	movw %dx, COUNTDOWN(%edi) 

	movw STATUS(%esi), %dx
	movw %dx, STATUS(%edi)

	# Call poke
	movw STATUS(%edi), %dx
	cmpw $0, %dx
	je status_zero
	cmpw $1, %dx
	je status_one

status_zero:
	movb OFF_CHAR(%edi), %cl
	jmp cont_sync
	
status_one:
	movb ON_CHAR(%edi), %cl
	jmp cont_sync

cont_sync:
	movw LOCATION(%edi), %ax
	movzwl %ax, %eax
	imull $2, %eax 
	call mp1_poke

  	# Return 0 on success, -1 on failure
	jmp success

# ------------------------------------------------------------------------------

.end
