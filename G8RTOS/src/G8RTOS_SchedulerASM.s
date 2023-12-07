; G8RTOS_SchedulerASM.s
; Created: 2022-07-26
; Updated: 2022-07-26
; Contains assembly functions for scheduler.

	; Functions Defined
	.def G8RTOS_Start, PendSV_Handler

	; Dependencies
	.ref CurrentlyRunningThread, G8RTOS_Scheduler

	.thumb		; Set to thumb mode
	.align 2	; Align by 2 bytes (thumb mode uses allignment by 2 or 4)
	.text		; Text section

; Need to have the address defined in file
; (label needs to be close enough to asm code to be reached with PC relative addressing)
RunningPtr: .field CurrentlyRunningThread, 32

; G8RTOS_Start
;	Sets the first thread to be the currently running thread
;	Starts the currently running thread by setting Link Register to tcb's Program Counter
G8RTOS_Start:

	.asmfunc
	PUSH {R4 - R11} ;  push registers
	;Load the address of RunningPtr
	LDR R4, RunningPtr
	;Load the currently running pointer
	LDR R5, [R4] ; load r4 with the running pointer into r5
	LDR SP, [R5] ; load the running pointre address into stack pointer
	LDR LR, [SP, #56] ; loading lr with PC value at 14 (14 *4 word allignment)

	POP {R4 - R11}  ; pop all registers

	CPSIE I

	BX LR				;Branches to the first thread

	.endasmfunc

; PendSV_Handler
; - Performs a context switch in G8RTOS
; 	- Saves remaining registers into thread stack
;	- Saves current stack pointer to tcb
;	- Calls G8RTOS_Scheduler to get new tcb
;	- Set stack pointer to new stack pointer from new tcb
;	- Pops registers from thread stack
PendSV_Handler:

	.asmfunc
	; put your assembly code here!
	CPSID I
	PUSH {R4 - R11}

	; do stuff here
	; i believe i need to call the scheduler here, then use that stack pointer and update the current one
	; Call the scheduler to update CurrentlyRunningThread
	LDR R4, RunningPtr ; load r4 with the address of the running pointer
	LDR R5, [R4] ; load r5 with the address
	STR SP, [R5] ; save the context into the stack pointer
	PUSH {LR} ; push the link register so we save necessary registers and know where to return
	BL G8RTOS_Scheduler ; branch and link to the schedule to get to the next thread
	; Load the new stack pointer from the updated TCB
	POP {LR} ; renew the link return address
	LDR R5, [R4] ; load r5 with the running pointer
	LDR SP, [R5] ; load the running pointer address into the stack

	POP {R4 - R11}
	CPSIE I
	BX LR
	.endasmfunc

	; end of the asm file
	.align
	.end
