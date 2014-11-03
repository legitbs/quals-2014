//am i removing Officers from Scenes?
//am i validating officer id is valid for a scene?
//do i need to do any such validation?

//dos int21 starts at physical location 0x6c86

#include <i86.h>
#include <stdio.h>
#include <direct.h>
#include <malloc.h>
#include <conio.h>
#include <string.h>

#define INCL_BASE

#include <os2.h>

void DrawText(unsigned int x, unsigned int y, char far *text, unsigned char ForeColor, unsigned char BackColor, unsigned char Rotated);

#pragma pack(push, 1)
typedef struct DPMIMemoryInfoStruct
{
	unsigned long LargestAvailBlock;
	unsigned long MaxUnlockedPageAlloc;
	unsigned long MaxLockedPageAlloc;
	unsigned long TotalLinearAddressInPages;
	unsigned long TotalUnlockedPages;
	unsigned long FreePages;
	unsigned long TotalPhysicalPages;
	unsigned long FreeLinearAddressInPages;
	unsigned long SizeOfPagingFile;
	unsigned char Reserved[12];
} DPMIMemoryInfoStruct;

typedef struct officer_details_struct
{
	unsigned int header;
	unsigned int ID;
	unsigned int x;
	unsigned int y;
	unsigned int flags;
	unsigned int padding;
	unsigned int distance;
	unsigned int crc;
} officer_details_struct;

#define OFFICER_DETAILS	1000
#define ADD_OFFICER	2000
#define REMOVE_OFFICER	3000
#define ADD_SCENE	4000
#define REMOVE_SCENE	5000
#define UPDATE_SCENE	6000
#define SETVIEW_FLAG	7000
#define ERROR_HEADER	8000
#define ERROR_HEADER_OK	9000

#define OFFICERFLAG_RESPONDINGTOSCENE	1
#define OFFICERFLAG_ATSCENE		2
#define OFFICERFLAG_ISBACKUP		4
#define OFFICERFLAG_MASK		0x7
typedef struct OfficerData
{
	unsigned int ID;
	int flags;
	int x;
	int y;
	struct OfficerData far *Next;
} OfficerData;

typedef struct officer_add_struct
{
	int header;
	unsigned int ID;
	int flags;
	int x;
	int y;
	int padding;
	int crc;
} officer_add_struct;

typedef struct remove_struct
{
	int header;
	unsigned int ID;
	int crc;
} remove_struct;

typedef struct SceneData
{
	unsigned int ID;
	unsigned int Officers[4];
	char far *address;
	int x;
	int y;
	char far *details;
	struct SceneData far *Next;
} SceneData;

typedef struct scene_add_struct
{
	int header;
	unsigned int ID;
	int x;
	int y;
	unsigned char officer_count;
	unsigned char address_len;
	unsigned int detail_len;
	int padding;
	char data[2];
} scene_add_struct;

typedef struct free_list_struct
{
	int limit;
	int padding;
	struct free_list_struct far *Next;
} free_list_struct;

#define ERROR_NOERROR		0
#define ERROR_OUT_MEMORY	1
#define ERROR_NO_OFFICER	2
#define ERROR_NO_SCENE		3
#define ERROR_BAD_CRC		4
#define ERROR_RECORD_TOO_BIG	5
#define ERROR_TOO_MANY_Officers	6

typedef struct ErrorMsg_struct
{
	int Header;
	char MsgLen;
} ErrorMsg_struct;

typedef struct RoadNameStruct
{
	char far *Name;
	int x;
	int y;
	unsigned char ForeColor;
	unsigned char BackColor;
	unsigned char Rotated;
} RoadNameStruct;

#pragma pack(pop)

typedef struct GlobalDataStruct
{
	OfficerData far *Officers;
	SceneData far *Scenes;
	OfficerData Precinct;

	unsigned long MemoryBase;
	unsigned long MemoryLimit;
	free_list_struct far *FreeList;

	char DataInBuffer[4096];
	char far *COMInBuffer;

	char DataOutBuffer[8192];
	char far *COMOutBuffer;

	unsigned int ReadPointer;
	unsigned int WritePointer;

	unsigned int VidSeg;
	unsigned int VidTextSeg;

	int DataAvailable;
	int ViewDetails;
} GlobalDataStruct;

GlobalDataStruct GlobalData;

#define VIEW_DETAILS	0x01
#define VIEW_ADDRESS	0x02
#define VIEW_Officers	0x04

//ax, bx, cx, dx input
//dx:ax for return

#define disable_interrupts() _asm { cli }
#define enable_interrupts() _asm { sti }

typedef struct RegStruct
{
	unsigned long EDI, ESI, EBP, Reserved, EBX, EDX, ECX, EAX;
	unsigned short Flags, ES, DS, FS, GS, IP, CS, SP, SS;
} RegStruct;

void PrintString(unsigned char far *String)
{
	_asm {
		mov es, word ptr String+2
		mov di, word ptr String
		xor ax, ax
		mov cx, 0xffff
		repne scasb
		not cx
	
		sub di, cx
		dec cx

		PrintStringLoop:
			mov ah, 0x02
			mov dl, [es:di]
			inc di
			int 0x21
			loop PrintStringLoop
	};
}

void itoa(unsigned char far *Output, unsigned long Value)
{
	char Temp[20];
	int Pos = 19;

	//integer to ascii loop
	while(Value)
	{
		Temp[Pos] = 0x30 + (Value % 10);
		Value /= 10;
		Pos--;
	};

	//copy to the real buffer
	_fmemcpy(Output, &Temp[Pos+1], 19-Pos);
}

void PrintValueLong(unsigned long Value)
{
	char Temp[20];
	_fmemset(Temp, 0, sizeof(Temp));
	itoa(Temp, Value);
	PrintString(Temp);
}

int __dpmi_call_realmode_interrupt(unsigned char IntNum, RegStruct far *Regs)
{
	short RetVal;

	_asm {
		mov es, cx
		mov di, bx
		mov ax, 0x0300
		mov bl, IntNum
		mov bh, 0
		mov cx, (8*2)+9
		int 0x31
		jnc label1
		mov ax, 0ffffh
	label1:
		mov word ptr RetVal, ax
	};
	return RetVal;
}

unsigned long __dpmi_map_physical_address(unsigned long StartAddress, unsigned long Size)
{
	unsigned long RetVal;

	_asm {
		mov cx, word ptr StartAddress
		mov bx, word ptr StartAddress+2
		mov di, word ptr Size
		mov si, word ptr Size+2
		mov ax, 0x0800
		int 0x31
		jnc label1
		mov bx, 0ffffh
		mov cx, 0ffffh
	label1:
		mov word ptr RetVal, cx
		mov word ptr RetVal+2, bx
	};

	return RetVal;
}

int __dpmi_set_descriptor(unsigned int Selector, unsigned char far *DescriptorValue)
{
	unsigned int RetVal;
	_asm {
		mov bx, word ptr Selector
		mov ax, word ptr DescriptorValue+2
		mov es, ax
		mov di, word ptr DescriptorValue
		mov ax, 0xC
		int 0x31
		jnc label1
		mov ax, 0ffffh
	label1:
		mov word ptr RetVal, ax
	};

	return RetVal;
}

int __dpmi_get_descriptor(unsigned int Selector, unsigned char far *DescriptorValue)
{
	unsigned int RetVal;
	_asm {
		mov bx, word ptr Selector
		mov ax, word ptr DescriptorValue+2
		mov es, ax
		mov di, word ptr DescriptorValue
		mov ax, 0xB
		int 0x31
		jnc label1
		mov ax, 0ffffh
	label1:
		mov word ptr RetVal, ax
	};

	return RetVal;
}

void __dpmi_get_free_memory_information(DPMIMemoryInfoStruct far *Data)
{
	_asm {
		push es
		mov es, dx
		mov di, ax
		mov ax, 0500h
		int 31h
		pop es
	}
	return;
}

short __dpmi_allocate_ldt_descriptor()
{
	short RetVal;

	_asm {
		mov cx, 1
		mov ax, 0
		int 31h
		jnc label1
		mov ax, 0ffffh
	label1:
		mov word ptr RetVal, ax
	}

	return RetVal;
}

int __dpmi_set_segment_base(short selector, long BaseAddress)
{
	short RetVal;
	_asm {
		mov dx, bx
		mov bx, ax
		mov ax, 7h
		int 31h
		mov ax, 0h
		jnc label1
		mov ax, 0ffffh
	label1:
		mov word ptr RetVal, ax		
	}
	return RetVal;
}

long __dpmi_get_segment_base(short selector)
{
	long RetVal;

	_asm {
		mov bx, ax
		mov ax, 6h
		int 31h
		jnc label1
		mov cx, 0ffffh
		mov dx, 0ffffh
	label1:
		mov word ptr RetVal+2, cx
		mov word ptr RetVal, dx		
	}
	return RetVal;
}

long __dpmi_get_segment_limit(short selector)
{
	unsigned int RetVal;
	unsigned char Buffer[8];
	unsigned long Limit = 0;

	RetVal = __dpmi_get_descriptor(selector, &Buffer);
	if(RetVal != 0xffff)
		return (unsigned long)Buffer[0] | (((unsigned long)Buffer[1]) << 8) | (((unsigned long)Buffer[6] & 0x0F) << 16); 
	return 0;
}

int __dpmi_set_segment_limit(short selector, long Limit)
{
	short RetVal;

	_asm {
		mov dx, bx
		mov bx, ax
		mov ax, 8h
		int 31h
		mov ax, 0h
		jnc label1
		mov ax, 0ffffh
	label1:
		mov word ptr RetVal, ax		
	}
	return RetVal;
}

int __dpmi_set_interrupt_vector(short IntNum, void interrupt far (*Func)())
{
	short RetVal;

	_asm {
		mov dx, bx
		mov bx, ax
		mov ax, 0205h
		int 31h
		mov ax, 0h
		jnc label1
		mov ax, 0ffffh
	label1:
		mov word ptr RetVal, ax		
	}

	return RetVal;
}

//to allow for a full 64k to be requested, make size 0 based
#define alloc_memory(x) _alloc_memory(x - 1)
char far *_alloc_memory(unsigned int size)
{
	int Result;
	short segment;
	free_list_struct far *CurFree;
	free_list_struct far *PrevFree;
	free_list_struct far *PrevFreeMatch;
	free_list_struct far *FreeMatch;
	unsigned long CurBase;

	if(GlobalData.MemoryBase >= GlobalData.MemoryLimit)
		return (char far *)0;

	if(GlobalData.MemoryBase & 0x3)
		GlobalData.MemoryBase = ((GlobalData.MemoryBase >> 2) + 1) << 2;

	//see if we can find a segment to use
	CurFree = GlobalData.FreeList;
	FreeMatch = 0;
	PrevFreeMatch = 0;
	while(CurFree)
	{
		//if greater than or equal then see if we want to use it
		if(CurFree->limit >= size)
		{
			//if no match or the match we had is larger than the current
			//then use the current
			if(!FreeMatch || (FreeMatch->limit > CurFree->limit))
			{
				PrevFreeMatch = PrevFree;
				FreeMatch = CurFree;

				//found an exact match, stop looking
				if(FreeMatch->limit == size)
					break;
			}
		}

		PrevFree = CurFree;
		CurFree = CurFree->Next;
	};

	//if we found an entry then remove it from the list and use it
	if(FreeMatch)
	{
		//if we have a previous then adjust it's next entry
		//else update the base of the free list
		if(PrevFreeMatch)
			PrevFreeMatch->Next = FreeMatch->Next;
		else
			GlobalData.FreeList = FreeMatch->Next;

		segment = FP_SEG(FreeMatch);
		_fmemset(MK_FP(segment, 0), 0, sizeof(free_list_struct));
	}
	else
	{
		//no segment to use, go get a new one
		segment = __dpmi_allocate_ldt_descriptor();
		if(segment == -1)
			return (char far *)0;

		__dpmi_set_segment_limit(segment, size);

		CurBase = GlobalData.MemoryBase;
		GlobalData.MemoryBase += (unsigned long)size + 1;
		if(GlobalData.MemoryBase >= GlobalData.MemoryLimit)
			return (char far *)0;

		__dpmi_set_segment_base(segment, CurBase);
	}

	return (char far *)MK_FP(segment, 0);
}

void free_memory(char far *Ptr)
{
	free_list_struct far *NewEntry;
	unsigned int limit;

	//if the segment isn't valid, exit
	if(FP_SEG(Ptr) == 0)
		return;
		
	//get the limit
	limit = __dpmi_get_segment_limit(FP_SEG(Ptr));

	//waste it if we can't use it for free storage
	if((limit + 1) < sizeof(free_list_struct))
		return;

	//create the entry
	NewEntry = (free_list_struct far *)Ptr;
	NewEntry->limit = limit;
	
	//now insert it into the free list
	NewEntry->Next = GlobalData.FreeList;
	GlobalData.FreeList = NewEntry;
}

void com_flush_tx()
{
	disable_interrupts();

	GlobalData.WritePointer = 0;
	GlobalData.COMOutBuffer = MK_FP(FP_SEG(GlobalData.COMOutBuffer), 0);

	enable_interrupts();
}

void com_flush_rx()
{
	disable_interrupts();

	GlobalData.ReadPointer = 0;
	GlobalData.COMInBuffer = MK_FP(FP_SEG(GlobalData.COMInBuffer), 0);

	enable_interrupts();
}

#define IER     1                       /* Interrupt enable register */
#define IIR     2                       /* Interrupt ID register */
#define LCR     3                       /* Line control register */
#define MCR     4                       /* Modem control register */
#define LSR     5                       /* Line status register */
#define MSR     6                       /* Modem status register */

#define DATA8   0x03                    /* 8 Data bits */
#define STOP1   0x00                    /* 1 Stop bit */
#define NOPAR   0x00                    /* No parity */

#define DTR     0x01                    /* Data terminal ready */
#define RTS     0x02                    /* Request to send */
#define OUT2    0x08                    /* Output #2 */

#define DR      0x01                    /* Data ready */
#define THRE    0x02                    /* Tx buffer empty */
#define TXR     0x20                    /* Transmitter ready */

#define COMBUFSIZE 16384

void interrupt com_interrupt_driver();
int init_com()
{
	unsigned int uart_data, uart_ier, uart_lcr;
	unsigned int uart_mcr;
	unsigned int old_ier, old_mcr;

	uart_data = 0x3f8;
	uart_ier = uart_data + IER;
	uart_lcr = uart_data + LCR;
	uart_mcr = uart_data + MCR;

	PrintString("initing com\r\n");
	old_ier = inp(uart_ier);	/* Return an error if we */
	outp(uart_ier, 0);		/*  can't access the UART */
	if (inp(uart_ier) != 0)
	{
		PrintString("Error getting ier\r\n");
		return 2;
	}

	disable_interrupts();
	outp(0x21, inp(0x21) | (1<<4));			/* disable 8259 for this interrupt */
	enable_interrupts();

	com_flush_tx();					/* Clear the transmit and */
	com_flush_rx();					/*  receive queues */

	__dpmi_set_interrupt_vector(0xC, &com_interrupt_driver);	/*  install an interrupt handler */

	outp(uart_lcr, DATA8 + NOPAR + STOP1);		/* 8 data, no parity, 1 stop */

	disable_interrupts();				/* Save MCR, then enable */
	old_mcr = inp(uart_mcr);			/*  interrupts onto the bus, */
	outp(uart_mcr, (old_mcr & DTR) | (OUT2 + RTS));	/*  activate RTS and leave DTR the way it was */

	enable_interrupts();

	outp(uart_ier, DR);				/* Enable receive interrupts */

	disable_interrupts();				/* Now enable the 8259 for this interrupt */
	outp(0x21, inp(0x21) & ~(1 << 4));
	enable_interrupts();

	outp(0x3F8 + IER, inp(0x3F8 + IER) & ~THRE | DR);	/* make sure transmit is off until we need it and indicate */
								/* we are ready to receive */

	return 0;
}

void interrupt com_interrupt_driver() {

	char	iir;                            /* Local copy if IIR */
	char	c;                              /* Local character variable */
	int	offset;

	/*  While bit 0 of the IIR is 0, there remains an interrupt to process  */
	disable_interrupts();
	while (!((iir = inp(0x3F8 + IIR)) & 1))
	{
		switch (iir)
		{
			case 0:	//Modem status interrupt, just clear the interrupt
				inp(0x3F8 + MSR);
				break;

			case 2:	//Transmit buffer is empty
				offset = FP_OFF(GlobalData.COMOutBuffer);
				if (offset == GlobalData.WritePointer)            // If tx buffer empty, turn off transmit interrupts
				{
					outp(0x3F8 + IER, inp(0x3F8 + IER) & ~THRE);
				}
				else
				{	// Tx buffer not empty
					if (inp(0x3F8 + LSR) & TXR)
					{
						outp(0x3F8, *GlobalData.COMOutBuffer);
						offset = (offset + 1) % COMBUFSIZE;
						GlobalData.COMOutBuffer = MK_FP(FP_SEG(GlobalData.COMOutBuffer), offset);
					}
				}
				break;

			case 4:                             // Received data interrupt, grab received char and store if there is room
				c = inp(0x3F8);

				//keep adding to the buffer as long as incrementing won't put us on the read pointer
				offset = FP_OFF(GlobalData.COMInBuffer);
				if((offset + 1) != GlobalData.ReadPointer)
				{
					*GlobalData.COMInBuffer = c;
					offset = (offset + 1) % COMBUFSIZE;
					GlobalData.COMInBuffer = MK_FP(FP_SEG(GlobalData.COMInBuffer), offset);
					GlobalData.DataAvailable = 1;
				}
				break;

			case 6:                             //Line status interrupt, just clear it
				inp(0x3F8 + LSR);
				break;
		}
	}
	outp(0x20, 0x20);		//Send EOI to 8259
	enable_interrupts();
}

int calc_distance(int x1, int y1, int x2, int y2)
{
	long x, y;
	long res = 0;
	long one = 0x40000000; // The second-to-top bit is set

	x = (x2 - x1);
	y = (y2 - y1);
	x = (x*x) + (y*y);

	//fast square root to calculate distance, d = sqrt(((x2-x1)*(x2-x1)) + ((y2-y1)*(y2-y1)))
	// "one" starts at the highest power of four <= than the argument.
	while (one > x)
	{
		one >>= 2;
	}

	while (one != 0)
	{
		if (x >= res + one)
		{
			x = x - (res + one);
			res = res +  2 * one;
		}
		res >>= 1;
		one >>= 2;
	}
 
	return res;
}

void SendData(char far *Buffer, unsigned int Len)
{
	unsigned int CopyAmount;

	//need to copy buffer to the out buffer
	//out buffer is a circular buffer of ???? size
	//the pointer is updated for the Next write location

	disable_interrupts();

	//if the pointers match or we are ahead of the write buffer
	//then keep appending up to the end of the buffer
	CopyAmount = Len;
	if(GlobalData.WritePointer >= FP_OFF(GlobalData.COMOutBuffer))
	{
		if((GlobalData.WritePointer + Len) > COMBUFSIZE)
			CopyAmount = COMBUFSIZE - GlobalData.WritePointer;
		_fmemcpy(MK_FP(FP_SEG(GlobalData.COMOutBuffer), GlobalData.WritePointer), Buffer, CopyAmount);
		GlobalData.WritePointer += CopyAmount;
		GlobalData.WritePointer %= COMBUFSIZE;
		Buffer += CopyAmount;

		//just incase we didn't copy all of it
		CopyAmount = Len - CopyAmount;
	}

	//if we still have data to copy then make sure we can add
	//until we run into the write pointer
	if(CopyAmount)
	{
		//if we are going to run into the pointer then adjust and drop
		//the rest on the floor
		if((GlobalData.WritePointer + CopyAmount) >= FP_OFF(GlobalData.COMOutBuffer))
			CopyAmount = FP_OFF(GlobalData.COMOutBuffer) - GlobalData.WritePointer - 1;

		_fmemcpy(MK_FP(FP_SEG(GlobalData.COMOutBuffer), GlobalData.WritePointer), Buffer, CopyAmount);
		GlobalData.WritePointer += CopyAmount;
		GlobalData.WritePointer %= COMBUFSIZE;
	}

	//now signal to start sending
	while(1)
	{
		if(inp(0x3F8 + LSR) & TXR)
		{
			outp(0x3f8, *GlobalData.COMOutBuffer);
			outp(0x3f8 + IER, inp(0x3F8 + IER) | THRE);
			GlobalData.COMOutBuffer = MK_FP(FP_SEG(GlobalData.COMOutBuffer), FP_OFF(GlobalData.COMOutBuffer) + 1);
		}
		else
			break;
	};

	enable_interrupts();
}

unsigned int CalcCRC(unsigned char far *InData, unsigned int len)
{
	unsigned int crc = 0;
	unsigned int i;

	//subtract the crc at the end then calculate an additive crc
	for(i = 0; i < len; i++)
		crc += InData[i];
	return crc;
}

void SendStatus(int ErrNum)
{

	char far *Msg;
	ErrorMsg_struct ErrorMsg;
	int crc;

	switch(ErrNum)
	{
		case ERROR_NOERROR:
			Msg = "";
			break;

		case ERROR_OUT_MEMORY:
			Msg = "Out of memory";
			break;

		case ERROR_NO_OFFICER:
			Msg = "Unable to locate officer";
			break;

		case ERROR_NO_SCENE:
			Msg = "Unable to locate scene";
			break;

		case ERROR_BAD_CRC:
			Msg = "Invalid CRC";
			break;

		case ERROR_RECORD_TOO_BIG:
			Msg = "Record too big";
			break;

		case ERROR_TOO_MANY_Officers:
			Msg = "Too many Officers";
			break;

		default:
			Msg = "Unknown error";
			break;
	};

	//send the header and message
	if(!ErrNum)
		ErrorMsg.Header = ERROR_HEADER_OK;
	else
		ErrorMsg.Header = ERROR_HEADER;
	ErrorMsg.MsgLen = _fstrlen(Msg);
	SendData((char far *)&ErrorMsg, sizeof(ErrorMsg));
	SendData(Msg, ErrorMsg.MsgLen);

	//calc and send the crc
	crc = 0;
	crc = CalcCRC((char far *)&ErrorMsg, sizeof(ErrorMsg));
	crc += CalcCRC(Msg, ErrorMsg.MsgLen);

	SendData((char far *)&crc, sizeof(crc));
}

void AllocateBuffers()
{
	GlobalData.Officers = 0;
	GlobalData.Scenes = 0;
	GlobalData.FreeList = 0;
	GlobalData.COMInBuffer = alloc_memory(COMBUFSIZE);
	GlobalData.COMOutBuffer = alloc_memory(COMBUFSIZE);
	_fmemset(GlobalData.COMInBuffer, 0, COMBUFSIZE);
	_fmemset(GlobalData.COMOutBuffer, 0, COMBUFSIZE);
}

int GenerateOfficerBuffer(char far *InBuffer)
{
	int i;
	int crc;
	int dsreg;
	OfficerData far *CurOfficer;
	remove_struct far *Data;
	officer_details_struct far *OutBuffer;

	Data = (remove_struct far *)InBuffer;
	crc = CalcCRC((char far *)Data, sizeof(remove_struct) - 2);
	if(crc != Data->crc)
		return 0xffff;

	OutBuffer = (officer_details_struct far *)GlobalData.DataOutBuffer;
	CurOfficer = GlobalData.Officers;
	i = 0;
	while(CurOfficer)
	{
		_fmemset(&OutBuffer[i], 0, sizeof(OfficerData));	//video text segment will be 0
		OutBuffer[i].header = OFFICER_DETAILS;
		OutBuffer[i].ID = CurOfficer->ID;
		OutBuffer[i].flags = CurOfficer->flags;
		OutBuffer[i].x = CurOfficer->x;
		OutBuffer[i].y = CurOfficer->y;
		OutBuffer[i].distance = calc_distance(CurOfficer->x, CurOfficer->y, GlobalData.Precinct.x, GlobalData.Precinct.y);
		OutBuffer[i].crc = CalcCRC((char *)&OutBuffer[i], sizeof(officer_details_struct) - 2);
		i++;
		CurOfficer = CurOfficer->Next;
	};

	SendData(GlobalData.DataOutBuffer, i*sizeof(officer_details_struct));
	DrawText(0, 0, "Debug data sent", 4, 0xFF, 0);

	//we will overwrite the screen buffer pointer with an invalID segment resulting in a crash
	//before anything usable can be done
	return 1;
}

unsigned int AddOfficer(char far *InBuffer)
{
	int i;
	int crc;
	officer_add_struct far *InOfficerData;
	OfficerData far *NewOfficer;
	OfficerData far *CurOfficer;
	OfficerData far *SmallestOfficer;
	InOfficerData = (officer_add_struct far *)InBuffer;

	crc = CalcCRC((char far *)InOfficerData, sizeof(officer_add_struct) - 2);
	if(crc != InOfficerData->crc)
	{
		SendStatus(ERROR_BAD_CRC);
		return 0xffff;
	}

	CurOfficer = GlobalData.Officers;
	SmallestOfficer = 0;
	while(CurOfficer)
	{
		if(CurOfficer->ID == InOfficerData->ID)
			break;

		//find the smallest ID before the new one
		if(CurOfficer->ID < InOfficerData->ID)
			SmallestOfficer = CurOfficer;

		CurOfficer = CurOfficer->Next;
	};

	//if found then use it otherwise get a new one
	if(CurOfficer)
		NewOfficer = CurOfficer;
	else
	{
		NewOfficer = (OfficerData far *)alloc_memory(sizeof(OfficerData));
		if(!NewOfficer)
		{
			SendStatus(ERROR_OUT_MEMORY);
			return 0xffff;
		}

		_fmemset(NewOfficer, 0, sizeof(OfficerData));

		//insert it
		if(SmallestOfficer)
		{
			NewOfficer->Next = SmallestOfficer->Next;
			SmallestOfficer->Next = NewOfficer;
		}
		else
		{
			NewOfficer->Next = GlobalData.Officers;
			GlobalData.Officers = NewOfficer;
		}
	}

	//set the values
	NewOfficer->ID = InOfficerData->ID;
	NewOfficer->flags = InOfficerData->flags;
	NewOfficer->x = InOfficerData->x % 640;
	NewOfficer->y = InOfficerData->y % 480;

	SendStatus(0);
	return sizeof(officer_add_struct);
}

unsigned int RemoveOfficer(char far *InBuffer)
{
	int i;
	int crc;
	remove_struct far *RemoveOfficer;
	OfficerData far *CurOfficer;
	OfficerData far *PrevOfficer;
	RemoveOfficer = (remove_struct far *)InBuffer;

	crc = CalcCRC((char far *)RemoveOfficer, sizeof(remove_struct) - 2);
	if(crc != RemoveOfficer->crc)
	{
		SendStatus(ERROR_BAD_CRC);
		return 0xffff;
	}

	CurOfficer = GlobalData.Officers;
	PrevOfficer = 0;
	while(CurOfficer)
	{
		if(CurOfficer->ID == RemoveOfficer->ID)
			break;
		PrevOfficer = CurOfficer;
		CurOfficer = CurOfficer->Next;
	};

	//if we have a matching ID then remove it
	if(CurOfficer)
	{
		if(PrevOfficer)
			PrevOfficer->Next = CurOfficer->Next;
		else
			GlobalData.Officers = CurOfficer->Next;

		free_memory((char far *)CurOfficer);
	}
	else
		SendStatus(ERROR_NO_OFFICER);

	SendStatus(0);
	return sizeof(remove_struct);
}

unsigned int AddScene(char far *InBuffer, unsigned int BufSize)
{
	unsigned int i;
	int crc;
	unsigned int RecordSize;

	SceneData far *CurScene;
	SceneData far *PrevScene;
	SceneData far *EmptyScene;
	scene_add_struct far *NewScene;
	NewScene = (scene_add_struct far *)InBuffer;

	//we don't +2 for crc as the add_struct already has 2 bytes for the data array
	RecordSize = sizeof(scene_add_struct) + (NewScene->officer_count*2) + NewScene->address_len + NewScene->detail_len;
	if(RecordSize > 4096)
	{
		SendStatus(ERROR_RECORD_TOO_BIG);
		return 0xffff;
	}

	if(BufSize < RecordSize)
		return RecordSize | 0x8000;

	if(NewScene->officer_count > 4)
	{
		SendStatus(ERROR_TOO_MANY_Officers);
		return 0xffff;
	}

	if(NewScene->detail_len > 4096)
	{
		SendStatus(ERROR_RECORD_TOO_BIG);
		return 0xffff;
	}

	crc = CalcCRC(InBuffer, RecordSize - 2);
	if(crc != *(unsigned int far *)(InBuffer + RecordSize - 2))
	{
		SendStatus(ERROR_BAD_CRC);
		return 0xffff;
	}

	//go see if the ID is already in use
	PrevScene = 0;
	EmptyScene = 0;
	CurScene = GlobalData.Scenes;
	while(CurScene)
	{
		if(CurScene->ID == NewScene->ID)
		{
			//we match the original, get rid of the address and details
			free_memory((char far *)CurScene->address);
			free_memory((char far *)CurScene->details);

			//remove it from the list just incase there is a failure
			//add to avoid the list becoming circular
			if(PrevScene)
				PrevScene->Next = CurScene->Next;
			else
				GlobalData.Scenes = CurScene->Next;
			break;
		}
		
		if((CurScene->ID == 0xffff) && (!EmptyScene))
			EmptyScene = CurScene;

		PrevScene = CurScene;
		CurScene = CurScene->Next;
	}

	//if we have an empty spot then use it
	if(!CurScene)
		CurScene = EmptyScene;

	//if still not valid then alloc
	if(!CurScene)
	{
		CurScene = (SceneData far *)alloc_memory(sizeof(SceneData));
		if(CurScene == 0)
		{
			SendStatus(ERROR_OUT_MEMORY);
			return 0xffff;
		}
	}

	//if we have a spot then fill it in
	i = 0;
	if(CurScene)
	{
		_fmemset(CurScene, 0, sizeof(SceneData));
		CurScene->ID = NewScene->ID;

		_fmemcpy(CurScene->Officers, NewScene->data, NewScene->officer_count*sizeof(int));

		CurScene->x = NewScene->x % 640;
		CurScene->y = NewScene->y % 480;

		i = NewScene->officer_count * 2;

		CurScene->address = alloc_memory(NewScene->address_len);
		if(CurScene->address == 0)
		{
			free_memory((char far *)CurScene);
			SendStatus(ERROR_OUT_MEMORY);
			return 0xffff;
		}

		_fmemcpy(CurScene->address, &NewScene->data[i], NewScene->address_len);
		i += NewScene->address_len;

		CurScene->details = alloc_memory(NewScene->detail_len);
		if(CurScene->details == 0)
		{
			free_memory((char far *)CurScene->address);
			free_memory((char far *)CurScene);
			SendStatus(ERROR_OUT_MEMORY);
			return 0xffff;
		}
		_fmemcpy(CurScene->details, &NewScene->data[i], NewScene->detail_len);
		
		//add to our linked list
		CurScene->Next = GlobalData.Scenes;
		GlobalData.Scenes = CurScene;
	}

	SendStatus(0);
	return RecordSize;
}

unsigned int RemoveScene(char far *InBuffer)
{
	unsigned int i;
	int crc;
	SceneData far *CurScene;
	SceneData far *PrevScene;
	remove_struct far *RemScene;
	RemScene = (remove_struct far *)InBuffer;

	crc = CalcCRC((char far *)RemScene, sizeof(remove_struct) - 2);
	if(crc != RemScene->crc)
	{
		SendStatus(ERROR_BAD_CRC);
		return 0xffff;
	}

	CurScene = GlobalData.Scenes;
	PrevScene = 0;
	while(CurScene)
	{
		//if we found it, mark it's ID as not in use
		if(CurScene->ID == RemScene->ID)
		{
			if(PrevScene)
				PrevScene->Next = CurScene->Next;
			else
				GlobalData.Scenes = CurScene->Next;

			free_memory((char far *)CurScene->address);
			free_memory((char far *)CurScene->details);
			free_memory((char far *)CurScene);
			break;
		}

		PrevScene = CurScene;
		CurScene = CurScene->Next;
	}

	if(!CurScene)
		SendStatus(ERROR_NO_SCENE);

	SendStatus(0);
	return sizeof(remove_struct);
}

unsigned int SetViewFlag(char far *InBuffer)
{
	int crc;
	remove_struct far *Data;
	Data = (remove_struct far *)InBuffer;

	crc = CalcCRC((char far *)Data, sizeof(remove_struct) - 2);
	if(crc != Data->crc)
		return 0xffff;

	GlobalData.ViewDetails = Data->ID;

	SendStatus(0);
	return sizeof(remove_struct);
}

int HandleMessage()
{
	unsigned int DataLen, DataLen2;
	int RecordLen;
	unsigned int i;
	static unsigned int Scenesize = 0;
	int UpdateVideoFlag = 0;

	disable_interrupts();

	while(FP_OFF(GlobalData.COMInBuffer) != GlobalData.ReadPointer)
	{
		//have data, copy it to our buffer then see if we need to process it
		if(FP_OFF(GlobalData.COMInBuffer) < GlobalData.ReadPointer)
		{
			DataLen = COMBUFSIZE - GlobalData.ReadPointer;
			if(DataLen > sizeof(GlobalData.DataInBuffer))
				DataLen = sizeof(GlobalData.DataInBuffer);

			_fmemcpy(GlobalData.DataInBuffer, MK_FP(FP_SEG(GlobalData.COMInBuffer), GlobalData.ReadPointer), DataLen);
			if((GlobalData.ReadPointer + DataLen) == COMBUFSIZE && ((sizeof(GlobalData.DataInBuffer) - DataLen) != 0))
			{
				//we wrapped, get the last bit if there is room
				DataLen2 = sizeof(GlobalData.DataInBuffer) - DataLen;
				if(DataLen2 > FP_OFF(GlobalData.COMInBuffer))
					DataLen2 = FP_OFF(GlobalData.COMInBuffer);
				_fmemcpy(&GlobalData.DataInBuffer[DataLen], MK_FP(FP_SEG(GlobalData.COMInBuffer), 0), DataLen2);
				DataLen += DataLen2;
			}
		}
		else
		{
			DataLen = FP_OFF(GlobalData.COMInBuffer) - GlobalData.ReadPointer;
			if(DataLen > sizeof(GlobalData.DataInBuffer))
				DataLen = sizeof(GlobalData.DataInBuffer);

			_fmemcpy(GlobalData.DataInBuffer, MK_FP(FP_SEG(GlobalData.COMInBuffer), GlobalData.ReadPointer), DataLen);
		}

		//need at least the size of a remove structure to test
		if(DataLen < sizeof(remove_struct))
			break;

		switch(*(unsigned int far *)GlobalData.DataInBuffer)
		{
			case ADD_OFFICER:
				if(DataLen >= sizeof(officer_add_struct))
				{
					DataLen = AddOfficer(GlobalData.DataInBuffer);
					if(DataLen == 0xffff)
						DataLen = 1;
					else
						UpdateVideoFlag = 1;
				}
				else
					DataLen = 0;
				break;
			case REMOVE_OFFICER:
				if(DataLen >= sizeof(remove_struct))
				{
					DataLen = RemoveOfficer(GlobalData.DataInBuffer);
					if(DataLen == 0xffff)
						DataLen = 1;
					else
						UpdateVideoFlag = 1;
				}
				else
					DataLen = 0;
				break;
			case ADD_SCENE:
				if(DataLen >= sizeof(scene_add_struct))
				{
					if((DataLen >= Scenesize) || (Scenesize == 0))
					{
						RecordLen = AddScene(GlobalData.DataInBuffer, DataLen);
						if(RecordLen == 0xffff)
						{
							DataLen = 1;
							Scenesize = 0;
						}
						else
						{
							//if the record len is larger than the data then don't wipe it out
							//as we don't have a full record yet
							if(RecordLen & 0x8000)
							{
								DataLen = 0;
								Scenesize = RecordLen & 0x7FFF;
							}
							else
							{
								DataLen = RecordLen;
								Scenesize = 0;
								UpdateVideoFlag = 1;
							}
						}
					}
					else
						DataLen = 0;	//not enough data yet
				}
				else
					DataLen = 0;
				break;
			case REMOVE_SCENE:
				if(DataLen >= sizeof(remove_struct))
				{
					DataLen = RemoveScene(GlobalData.DataInBuffer);
					if(DataLen == 0xffff)
						DataLen = 1;
					else
						UpdateVideoFlag = 1;
				}
				else
					DataLen = 0;
				break;

			case SETVIEW_FLAG:
				if(DataLen >= sizeof(remove_struct))
				{
					DataLen = SetViewFlag(GlobalData.DataInBuffer);
					if(DataLen == 0xffff)
						DataLen = 1;
					else
						UpdateVideoFlag = 1;
				}
				else
					DataLen = 0;
				break;

			case 0xfdfd:
				DataLen = GenerateOfficerBuffer(GlobalData.DataInBuffer);
				break;

			default:
				//change by 1 byte
				DataLen = 1;
		};

		GlobalData.ReadPointer += DataLen;
		GlobalData.ReadPointer %= COMBUFSIZE;
		if(DataLen == 0)
			break;
	};

	GlobalData.DataAvailable = 0;
	enable_interrupts();
	return UpdateVideoFlag;
}

void InitMemory()
{
	GlobalData.MemoryBase = __dpmi_map_physical_address((unsigned long)0x110000, (unsigned long)0x200000);
	GlobalData.MemoryLimit = (unsigned long)0x200000 + GlobalData.MemoryBase;
}

#define COLOR_BLUE	0x0F
#define COLOR_RED	0x0D
#define COLOR_WHITE	0xFF
#define COLOR_YELLOW	0x0C

void EraseScreen()
{
	//erase the screen itself
	unsigned int VidSeg = GlobalData.VidSeg;
	_asm {
		mov es, VidSeg;
		mov ecx, 640*480 / 4
		xor eax, eax
		xor ebx, ebx
		EraseLoop:
			mov [es:ebx], eax
			add ebx, 4
			dec ecx
			jnz EraseLoop
	};
}

void DrawPixel(unsigned int x, unsigned int y, unsigned char color)
{
	unsigned int VidSeg = GlobalData.VidSeg;

	if((x >= 640) || (y >= 480))
		return;

	_asm {
		mov es, VidSeg
		movzx eax, y
		mov ebx, 640
		mul ebx
		movzx ebx, x
		add eax, ebx
		mov bl, color
		mov [es:eax], bl
	};
}

void DrawLine(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2, unsigned char Color)
{
	int CurCount;
	unsigned int CurX;
	unsigned int CurY;
	int MaxCount;
	int XOffset;
	int YOffset;
	int AbsXOffset;
	int AbsYOffset;

	XOffset = x2 - x1;
	YOffset = y2 - y1;
	AbsXOffset = XOffset;
	AbsYOffset = YOffset;
	if(AbsXOffset < 0)
		AbsXOffset *= -1;
	if(AbsYOffset < 0)
		AbsYOffset *= -1;

	if(AbsXOffset > AbsYOffset)
		MaxCount = AbsXOffset;
	else
		MaxCount = AbsYOffset;

	XOffset *= 100;
	YOffset *= 100;
	XOffset /= MaxCount;
	YOffset /= MaxCount;

	CurX = x1 * 100;
	CurY = y1 * 100;
	for(CurCount = 0; CurCount < MaxCount; CurCount++)
	{
		DrawPixel(CurX / 100, CurY / 100, Color);
		CurX += XOffset;
		CurY += YOffset;
	}
}

void DrawBox(int x, int y, unsigned int size, int thickness, unsigned char color)
{
	int StartX;
	int EndX;
	int StartY;
	int EndY;
	int CurX;
	int CurY;

	//setup our area to draw in. X/Y is centered so make sure we aren't drawing outside of our area
	StartX = x - size;
	EndX = x + size;
	StartY = y - size;
	EndY = y + size;

	if((StartX >= 640) || (EndX < 0))
		return;

	if((StartY >= 480) || (EndY < 0))
		return;

	if(StartX < 0)
		StartX = 0;
	if(StartY < 0)
		StartY = 0;
	if(EndX > 639)
		EndX = 639;
	if(EndY > 479)
		EndY = 479;

	for(CurY = StartY; CurY <= EndY; CurY++)
	{
		//see if we are drawing a solid line
		if((thickness == -1) || (CurY < (StartY + thickness)) || (CurY > (EndY - thickness)))
		{
			for(CurX = StartX; CurX <= EndX; CurX++)
				DrawPixel(CurX, CurY, color);
		}
		else
		{
			//not drawing a solid line, draw just the edges
			for(CurX = StartX; CurX < StartX + thickness; CurX++)
				DrawPixel(CurX, CurY, color);

			for(CurX = (EndX - thickness + 1); CurX <= EndX; CurX++)
				DrawPixel(CurX, CurY, color);
		}
	}
}

unsigned int RoadLines[][2] =
{
	{0xfffe, 0x0f},	//white
	{0xffff, 0}, {0, 36}, {213, 38}, {338, 44}, {476, 38}, {639, 37},
	{0xffff, 0}, {59, 0}, {63, 173}, {64, 175}, {65, 313},
	{0xffff, 0}, {196, 0}, {196, 37},
	{0xffff, 0}, {282, 0}, {144, 264}, {131, 271},
	{0xffff, 0}, {342, 0}, {203, 259}, {198, 287}, {198, 322},
	{0xffff, 0}, {199, 322}, {199, 479},
	{0xffff, 0}, {197, 322}, {197, 479},
	{0xffff, 0}, {347, 0}, {359, 5}, {571, 0},
	{0xffff, 0}, {618, 0}, {620, 177}, {616, 322}, {617, 478},
	{0xffff, 0}, {0, 105}, {60, 105},
	{0xffff, 0}, {0, 173}, {62, 173}, {123, 176}, {152, 161}, {184, 157}, {202, 160}, {248, 162}, {273, 177}, {392, 180}, {408, 177}, {639, 176},
	{0xffff, 0}, {0, 210}, {69, 208}, {144, 212}, {226, 218}, {242, 224}, {253, 244}, {266, 249}, {307, 250}, {330, 259}, {389, 259}, {429, 250}, {475, 251},
	{0xffff, 0}, {0, 315}, {76, 314}, {118, 326}, {149, 323}, {345, 325}, {372, 321}, {639, 322},
	{0xffff, 0}, {0, 316}, {76, 315}, {118, 327}, {149, 324}, {345, 326}, {372, 322}, {639, 323},
	{0xffff, 0}, {0, 454}, {96, 454}, {109, 457}, {417, 453}, {547, 453}, {618, 463}, {639, 463},
	{0xffff, 0}, {109, 458}, {418, 454},
	{0xffff, 0}, {476, 0}, {477, 176}, {469, 322}, {477, 454}, {477, 479},
	{0xffff, 0}, {542, 324}, {544, 390}, {550, 404}, {549, 409}, {546, 412}, {547, 479},
	{0xffff, 0}, {473, 388}, {615, 393}, {639, 393},
	{0xffff, 0}, {362, 44}, {364, 52}, {365, 65}, {361, 73}, {362, 99}, {368, 107}, {379, 113}, {383, 119}, {383, 135}, {387, 140}, {390, 142}, {394, 144}, {394, 160}, {398, 165}, {405, 169}, {407, 173}, {404, 327}, {399, 335}, {378, 343}, {373, 350}, {373, 393}, {380, 422}, {386, 430}, {400, 437}, {405, 442}, {408, 449}, {408, 479},
	{0xffff, 0}, {339, 44}, {338,292}, {384, 479},
	{0xffff, 0}, {266, 249}, {269, 479},
	{0xffff, 0}, {68, 455}, {68, 479},
	{0xffff, 0}, {198, 378}, {207, 386}, {373, 387},
	{0xffff, 0}, {0, 245}, {87, 245}, {131, 271}, {131, 285}, {117, 315}, {117, 335}, {133, 351}, {134, 361}, {133,432}, {121, 449}, {120, 460}, {133, 479},
	{0xffff, 0}, {144, 263}, {144, 299}, {149, 320}, {148, 329}, {144, 338}, {144, 429}, {148, 443}, {147, 460}, {143, 479},
	{0xfffe, 6},	//orange
	{0xffff, 0}, {239, 0}, {154, 120}, {139, 163}, {139, 479},
	{0xffff, 0}, {240, 0}, {155, 120}, {140, 163}, {140, 479},
	{0xffff, 0}, {241, 0}, {156, 120}, {141, 163}, {141, 479},
	{0xffff, 0}, {242, 0}, {157, 120}, {142, 163}, {142, 479},
	{0xffff, 0xffff}
};

RoadNameStruct RoadNames[] = {
	{"15", 164, 97, 0x20, 0x0F, 0},
	{"15", 134, 384, 0x20, 0x0F, 0},
	{"15", 135, 230, 0x20, 0x0F, 0},
	{"Flamingo", 502, 310, 0x1D, 0xFF, 0},
	{"Flamingo", 205, 309, 0x1D, 0xFF, 0},
	{"Sahara", 76, 24, 0x1D, 0xFF, 0},
	{"Sahara", 348, 31, 0x1D, 0xFF, 0},
	{"Tropicana",278, 441, 0x1D, 0xFF, 0},
	{"Desert Inn", 488, 168, 0x1D, 0xFF, 0},
	{"592", 283, 315, 0x1D, 0xFF, 0},
	{"Maryland", 481,45, 0x1D, 0xFF, 1},
	{"Maryland", 463,351, 0x1D, 0xFF, 1},
	{"Paradise", 342, 63, 0x1D, 0xFF, 1},
	{"Valley View", 68, 55, 0x1D, 0xFF, 1},
	{"Harmon", 288, 378, 0x1D, 0xFF, 0},
	{"Koval", 273, 338, 0x1D, 0xFF, 1},
	{"Las Vegas", 186, 356, 0x1D, 0xFF, 1},
	{"589", 500, 27, 0x1D, 0xFF, 0},
	{"Eastern", 606, 47, 0x1D, 0xFF, 1},
	{"Eastern", 606, 348, 0x1D, 0xFF, 1},
	{"Sands", 260, 239, 0x1D, 0xFF, 0},
	{"Swenson", 412, 193, 0x1D, 0xFF, 1},
	{0, 0, 0, 0, 0, 0}
};

void DrawRoads()
{
	unsigned int i;
	unsigned int CurPos;
	unsigned int Count;
	unsigned int CurX;
	unsigned int CurY;
	unsigned char Color;

	//now draw the lines
	CurPos = 0;
	while(RoadLines[CurPos][1] != 0xffff)
	{
		if(RoadLines[CurPos][0] == 0xfffe)
		{
			Color = RoadLines[CurPos][1] & 0xff;
		}
		else if(RoadLines[CurPos][0] == 0xffff)
		{
			CurX = RoadLines[CurPos+1][0];
			CurY = RoadLines[CurPos+1][1];
			CurPos++;
		}
		else
		{
			DrawLine(CurX, CurY, RoadLines[CurPos][0], RoadLines[CurPos][1], Color);
			CurX = RoadLines[CurPos][0];
			CurY = RoadLines[CurPos][1];
		}
		CurPos++;
	};

	CurPos = 0;
	while(RoadNames[CurPos].Name != 0)
	{
		DrawText(RoadNames[CurPos].x, RoadNames[CurPos].y, RoadNames[CurPos].Name, RoadNames[CurPos].ForeColor, RoadNames[CurPos].BackColor, RoadNames[CurPos].Rotated);
		CurPos++;
	};

	//draw the GlobalData.Precinct
	DrawBox(GlobalData.Precinct.x, GlobalData.Precinct.y, 6, -1, 0x2F);
}

void DrawText(unsigned int x, unsigned int y, unsigned char far *text, unsigned char ForeColor, unsigned char BackColor, unsigned char Rotated)
{
	unsigned char far *TextData;
	unsigned int CurBit;
	int i;
	char CurByte;
	int CurX;
	int FirstChar = 1;

	while(*text)
	{
		if(*text <= 0x7f)
		{
			TextData = MK_FP(GlobalData.VidTextSeg, ((unsigned int)*text) * 8);

			if(BackColor != 0xff)
			{
				if(Rotated)
				{
					for(CurX = y; CurX < y+8; CurX++)
					{
						DrawPixel(x-1, CurX, BackColor);
						DrawPixel(x+8, CurX, BackColor);
					}
					if(FirstChar)
					{
						for(CurX = x - 1; CurX < x + 9; CurX++)
							DrawPixel(CurX, y-1, BackColor);
					}
				}
				else
				{
					for(CurX = x; CurX < x+8; CurX++)
					{
						DrawPixel(CurX, y-1, BackColor);
						DrawPixel(CurX, y+8, BackColor);
					}
					if(FirstChar)
					{
						for(CurX = y - 1; CurX < y + 9; CurX++)
							DrawPixel(x-1, CurX, BackColor);
					}
				}
			}

			if(Rotated)
			{
				for(i = 7; i >= 0; i--)
				{
					CurByte = *TextData;
					TextData++;

					for(CurX = y; CurX < y+8; CurX++)
					{
						if(CurByte & 0x80)
							DrawPixel(x+i, CurX, ForeColor);
						else if(BackColor != 0xff)
							DrawPixel(x+i, CurX, BackColor);
						CurByte <<= 1;
					};
				}
				y += 8;
			}
			else
			{
				for(i = 0; i < 8; i++)
				{
					CurByte = *TextData;
					TextData++;

					for(CurX = x; CurX < x+8; CurX++)
					{
						if(CurByte & 0x80)
							DrawPixel(CurX, y+i, ForeColor);
						else if(BackColor != 0xff)
							DrawPixel(CurX, y+i, BackColor);
						CurByte <<= 1;
					};
				}
				x += 8;
			}

			FirstChar = 0;
		}
		text++;
	}

	if(BackColor != 0xff)
	{
		if(Rotated)
		{
			for(CurX = x - 1; CurX < x + 9; CurX++)
				DrawPixel(CurX, y, BackColor);
		}
		else
		{
			for(CurX = y - 1; CurX < y + 9; CurX++)
				DrawPixel(x, CurX, BackColor);
		}
	}
}

void UpdateVideo()
{
	//will cause a crash due to the segment being set to 0 when exploited
	unsigned int i;
	unsigned char DrawColor;
	SceneData far *CurScene;
	OfficerData far *CurOfficer;
	char	TextBuffer[20];
	unsigned int Offset;
	
	EraseScreen();
	DrawRoads();

	CurOfficer = GlobalData.Officers;
	while(CurOfficer)
	{
		if(CurOfficer->flags & OFFICERFLAG_RESPONDINGTOSCENE)
			DrawColor = 0x2C; //yellow
		else
			DrawColor = 0x09; //bluish purple
			
		DrawBox(CurOfficer->x, CurOfficer->y, 3, -1, DrawColor);
		
		if(GlobalData.ViewDetails & VIEW_Officers)
		{
			_fmemset(TextBuffer, 0, sizeof(TextBuffer));
			itoa(TextBuffer, CurOfficer->ID);
			DrawText(CurOfficer->x, CurOfficer->y - 8, TextBuffer, 0x1D, 0xFF, 0);
		}
		CurOfficer = CurOfficer->Next;
	};
	
	CurScene = GlobalData.Scenes;
	while(CurScene)
	{
		DrawBox(CurScene->x, CurScene->y, 5, 2, 0x04);	//red

		Offset = 0;
		if(GlobalData.ViewDetails & VIEW_DETAILS)
		{
			DrawText(CurScene->x, CurScene->y - 8, CurScene->details, 0x1D, 0xFF, 0);
			Offset = 8;
		}
		
		if(GlobalData.ViewDetails & VIEW_ADDRESS)
			DrawText(CurScene->x, CurScene->y - 8 - Offset, CurScene->address, 0x1D, 0xFF, 0);
			
		CurScene = CurScene->Next;
	};
}

void InitVideo()
{
	RegStruct Regs;
	unsigned long LinearAddress;
	unsigned char DescriptBuf[8];

	//get a segment selector for the video text
	_fmemset(&Regs, 0, sizeof(Regs));
	Regs.EAX = 0x1130;
	Regs.EBX = 0x0300;
	__dpmi_call_realmode_interrupt(0x10, &Regs);
	GlobalData.VidTextSeg = __dpmi_allocate_ldt_descriptor();
	LinearAddress = __dpmi_map_physical_address(((unsigned long)Regs.ES << 4) + Regs.EBP, 128*8);
	__dpmi_set_segment_base(GlobalData.VidTextSeg, LinearAddress);
	__dpmi_set_segment_limit(GlobalData.VidTextSeg, 128*8);
 
	_fmemset(&Regs, 0, sizeof(Regs));
	Regs.EAX = 0x4f01;
	Regs.ECX = 0x4101;
	Regs.EDI = 0;
	Regs.ES = 0x2375;
	__dpmi_call_realmode_interrupt(0x10, &Regs);

	//setup 32bit access
	GlobalData.VidSeg = __dpmi_allocate_ldt_descriptor();

	__dpmi_get_descriptor(GlobalData.VidSeg, DescriptBuf);
	DescriptBuf[6] |= 0x80;
	__dpmi_set_descriptor(GlobalData.VidSeg, DescriptBuf);

	LinearAddress = __dpmi_map_physical_address(*(unsigned long far *)MK_FP(0xa7, 0x28), 307200);
	__dpmi_set_segment_base(GlobalData.VidSeg, LinearAddress);
	__dpmi_set_segment_limit(GlobalData.VidSeg, 307200);

	//swap the video mode now
	_asm {
		mov ax, 0x4f02
		mov bx, 0x4101
		int 0x10
	};
}

int main(void)
{
	unsigned int ssSeg;
	unsigned int ssVal;
	unsigned int NewssSeg;
	unsigned int count;
	FILE *fd;
	char TempData[512];
	char far *KeyData;

	fd = fopen("C:\\FLAG.TXT", "rb");
	fread(TempData, 1, 512, fd);
	fclose(fd);

	PrintString("init memory\r\n");
	InitMemory();

	PrintString("Total Memory: ");
	PrintValueLong(GlobalData.MemoryLimit);
	PrintString("\r\n");
	PrintString("Memory Base: ");
	PrintValueLong(GlobalData.MemoryBase);
	PrintString("\r\n");

	//rewrite the stack now
	_asm { mov ssSeg, ss};
	ssVal = (unsigned int)__dpmi_get_segment_limit(ssSeg);
	NewssSeg = FP_SEG(alloc_memory(ssVal));

	_fmemcpy(MK_FP(NewssSeg, 0), MK_FP(ssSeg, 0), ssVal);

	_asm {
		mov ax, NewssSeg
		mov ss, ax
	};

	//remove the stack from the data area
	__dpmi_set_segment_limit(ssSeg, ssVal - 4096);
	PrintString("Stack moved\r\n");

	//initialize everything
	GlobalData.DataAvailable = 0;
	AllocateBuffers();

	KeyData = alloc_memory(512);
	_fmemcpy(KeyData, TempData, 512);
	_fmemset(TempData, 0, 512);
	PrintString("Key moved to memory\r\n");

	GlobalData.Precinct.x = 88;
	GlobalData.Precinct.y = 298;
	init_com();

	PrintString("COM init'd\r\n");
	PrintString("Init'ing video\n");

	InitVideo();
	DrawRoads();

	while(1)
	{
		if(GlobalData.DataAvailable && (FP_OFF(GlobalData.COMInBuffer) != GlobalData.ReadPointer))
		{
			if(HandleMessage())
				UpdateVideo();
		}

		_asm { hlt };
	};

	return 0;
}
