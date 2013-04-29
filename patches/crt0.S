.extern main

.global _start
_start:
call main
 
# loop in case we haven't yet rescheduled
_halt:
hlt
jmp _halt
