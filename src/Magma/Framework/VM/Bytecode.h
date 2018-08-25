#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

	/*
		MVM Bytecode version 1.0.
		
		Notes:
			- MVM Bytecode is stack based (it has no registers).
			- All literals are stored in big endianness.
			- Each virtual machine may have different memory and call stack sizes.

	*/

	
#define MFV_BYTECODE_POP				0x00	// Pops N bytes from stack (N = param 1 (1 byte))
#define MFV_BYTECODE_PUSH				0x01	// Pushs N bytes into the stack (N = param 1 (1 byte)) (push value = param 2 (N bytes))
#define MFV_BYTECODE_PUSH_COPY			0x02	// Makes a copy of the previous N bytes in the stack and pushes them again (N = param 1 (1 byte))

#define MFV_BYTECODE_ADDI8				0x10	// Pushes the top byte added to the next byte (1 byte signed integers)
#define MFV_BYTECODE_SUBI8				0x11	// Pushes the top byte subtracted to the next byte (1 byte signed integers)
#define MFV_BYTECODE_MULI8				0x12	// Pushes the top byte multiplied by the next byte (1 byte signed integers)
#define MFV_BYTECODE_DIVI8				0x13	// Pushes the top byte divided by the next byte (1 byte signed integers)
#define MFV_BYTECODE_MODI8				0x14	// Pushes the modulus of the top byte and the next byte (1 byte signed integers)

#define MFV_BYTECODE_ADDI16				0x20	// Pushes the top 2 bytes added to the next 2 bytes (2 byte signed integers)
#define MFV_BYTECODE_SUBI16				0x21	// Pushes the top 2 bytes subtracted to the next 2 bytes (2 byte signed integers)
#define MFV_BYTECODE_MULI16				0x22	// Pushes the top 2 bytes multiplied by the next 2 bytes (2 byte signed integers)
#define MFV_BYTECODE_DIVI16				0x23	// Pushes the top 2 bytes divided by the next 2 bytes (2 byte signed integers)
#define MFV_BYTECODE_MODI16				0x24	// Pushes the modulus of the top 2 bytes and the next 2 bytes (2 byte signed integers)

#define MFV_BYTECODE_ADDI32				0x30	// Pushes the top 4 bytes added to the next 4 bytes (4 byte signed integers)
#define MFV_BYTECODE_SUBI32				0x31	// Pushes the top 4 bytes subtracted to the next 4 bytes (4 byte signed integers)
#define MFV_BYTECODE_MULI32				0x32	// Pushes the top 4 bytes multiplied by the next 4 bytes (4 byte signed integers)
#define MFV_BYTECODE_DIVI32				0x33	// Pushes the top 4 bytes divided by the next 4 bytes (4 byte signed integers)
#define MFV_BYTECODE_MODI32				0x34	// Pushes the modulus of the top 4 bytes and the next 4 bytes (4 byte signed integers)

#define MFV_BYTECODE_ADDU8				0x40	// Pushes the top byte added to the next byte (1 byte unsigned integers)
#define MFV_BYTECODE_SUBU8				0x41	// Pushes the top byte subtracted to the next byte (1 byte unsigned integers)
#define MFV_BYTECODE_MULU8				0x42	// Pushes the top byte multiplied by the next byte (1 byte unsigned integers)
#define MFV_BYTECODE_DIVU8				0x43	// Pushes the top byte divided by the next byte (1 byte unsigned integers)
#define MFV_BYTECODE_MODU8				0x44	// Pushes the modulus of the top byte and the next byte (1 byte unsigned integers)

#define MFV_BYTECODE_ADDU16				0x50	// Pushes the top 2 bytes added to the next 2 bytes (2 byte unsigned integers)
#define MFV_BYTECODE_SUBU16				0x51	// Pushes the top 2 bytes subtracted to the next 2 bytes (2 byte unsigned integers)
#define MFV_BYTECODE_MULU16				0x52	// Pushes the top 2 bytes multiplied by the next 2 bytes (2 byte unsigned integers)
#define MFV_BYTECODE_DIVU16				0x53	// Pushes the top 2 bytes divided by the next 2 bytes (2 byte unsigned integers)
#define MFV_BYTECODE_MODU16				0x54	// Pushes the modulus of the top 2 bytes and the next 2 bytes (2 byte unsigned integers)

#define MFV_BYTECODE_ADDU32				0x60	// Pushes the top 4 bytes added to the next 4 bytes (4 byte unsigned integers)
#define MFV_BYTECODE_SUBU32				0x61	// Pushes the top 4 bytes subtracted to the next 4 bytes (4 byte unsigned integers)
#define MFV_BYTECODE_MULU32				0x62	// Pushes the top 4 bytes multiplied by the next 4 bytes (4 byte unsigned integers)
#define MFV_BYTECODE_DIVU32				0x63	// Pushes the top 4 bytes divided by the next 4 bytes (4 byte unsigned integers)
#define MFV_BYTECODE_MODU32				0x64	// Pushes the modulus of the top 4 bytes and the next 4 bytes (4 byte unsigned integers)

#define MFV_BYTECODE_ADDF32				0x70	// Pushes the top 4 bytes added to the next 4 bytes (4 byte floating point)
#define MFV_BYTECODE_SUBF32				0x71	// Pushes the top 4 bytes subtracted to the next 4 bytes (4 byte floating point)
#define MFV_BYTECODE_MULF32				0x72	// Pushes the top 4 bytes multiplied by the next 4 bytes (4 byte floating point)
#define MFV_BYTECODE_DIVF32				0x73	// Pushes the top 4 bytes divided by the next 4 bytes (4 byte floating point)
#define MFV_BYTECODE_MODF32				0x74	// Pushes the modulus of the top 4 bytes and the next 4 bytes (4 byte floating point)
#define MFV_BYTECODE_FLOORF32			0x75	// Pushes the top 4 bytes floored (4 byte floating point) 
#define MFV_BYTECODE_CEILF32			0x76	// Pushes the top 4 bytes ceiled (4 byte floating point) 
#define MFV_BYTECODE_FRACTF32			0x77	// Pushes the fractional part of the top 4 bytes (4 byte floating point) 

#ifdef __cplusplus
}
#endif
