.global _start
.extern main
_start:
 
## here you might want to get the argc/argv pairs somehow and then push
## them onto the stack...
 
# call the user's function
call main
 
# loop in case we haven't yet rescheduled
lp:
hlt
jmp lp
