#include <iostream>
#include <memory>
#include <array>
#include <bitset>
#include <ios>
#include <limits>

typedef char16_t word;

word bus = 0;
word regA = 0;
word regB = 0;

// 0, 0, 0, 0, 0, 0, Zero, Carry
word flags;
word p;

#define ZERO_FLAG 0b10
#define CARRY_FLAG 0b01

class ALU {
public:
	void invoke() {
		word in = bus;
		regA = add(in, regA);
	}
private:
	word add(word a, word b) {
		flags &= ~0b0000000000000011;
		if (a > 0 && b > 0xFFFF - a) {
			flags |= CARRY_FLAG;
		}
		word temp = a + b;
		if (temp == 0) {
			flags |= ZERO_FLAG;
		}
		return temp;
	}
} Alu;

class PROGRAMCOUNTER {
public:
	void invoke() {
		count += 1;
	}
	void jump() {
		count = bus - 1;
	}
	void reset() {
		count = 0;
	}
	word count = 0;
} ProgramCounter;

class R0M {
public:
	const std::array<word, 128> storage = {
		0x1001, 0x0201, 0x1000, 0x0500, 
		
		0x0301, 0x0202, 0x0101, 0x0200,

		0x0102, 0x0201, 0x0100, 0x0700, 
		
		0x0603, 0x0000, 0x0000, 0x0000
	};

} Rom;

class RAM {
public:
	std::array<word, 128> storage;
} Ram;

// instructions
#define NOP 0x00

#define	SUB	0x04

#define	ADD	0x03 // Load value from ram and add to regA
#define LDA 0x01 // Load Value from RAM into regA
#define	STA	0x02 // Store Value from regA into RAM

#define	OUT	0x05
#define	JMP	0x06
#define	JC	0x07
#define	JZ	0x08

#define HLT 0x0a

#define	LDI	0x10 // Load instantly
#define	ADI	0x11 // Add  instantly

#define	LDR	0x20 // Load Value from ROM into regA
#define	ADR	0x21 // Add value from ROM into regA

int main() {
	while (true) {
		auto currentInstructionFull = Rom.storage[ProgramCounter.count];
		auto instruction = (word)(currentInstructionFull >> 0x8);
		auto location = (word)(currentInstructionFull & 0x00FF);
	
		switch (instruction) {
		case NOP:
			break;

		// Loads 
		case LDA: 
			regA = Ram.storage[location];
			break;
		case LDR:
			regA = Rom.storage[location];
			break;
		case LDI:
			regA = location;
			break;

		// Stores
		case STA:
			Ram.storage[location] = regA;
			break;

		// Arth
		case ADD:
			bus = Ram.storage[location];
			Alu.invoke();
			break;
		case ADI:
			bus = location;
			Alu.invoke();
			break;

		case OUT:
			std::cout << std::dec << (int)regA << std::endl;
			break;

		case JMP:
			bus = location;
			ProgramCounter.jump();
			break;
		case JC:
			if ((flags & CARRY_FLAG) == CARRY_FLAG) {
				bus = location;
				ProgramCounter.jump();
			}
			break;
		case JZ:
			if ((flags & ZERO_FLAG) == ZERO_FLAG) {
				bus = location;
				ProgramCounter.jump();
			}
			break;
			
		case HLT:
			while (1);
			break;
		}
		ProgramCounter.invoke();
	}
	
	while (1);
}