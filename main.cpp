#include <iostream>
#include <memory>
#include <array>
#include <bitset>
#include <ios>
#include <limits>
#include <string>
#include <sstream>
#include <cstdlib>

typedef char16_t word;

word bus = 0;
word regA = 0;
word regB = 0;

// 0, 0, 0, 0, 0, Jump, Zero, Carry
word flags;

#define CARRY_FLAG 0b001
#define ZERO_FLAG  0b010
#define JUMP_FLAG  0b100

// instructions
#define NOP 0x00
#define LDA 0x01 // Load Value from RAM into regA
#define	STA	0x02 // Store Value from regA into RAM
#define	ADD	0x03 // Load value from ram and add to regA
#define	SUB	0x04

#define	OUT	0x05

#define	JMP	0x06
#define	JC	0x07
#define	JZ	0x08

#define HLT 0x0a

#define	LDI	0x10 // Load instantly
#define	ADI	0x11 // Add  instantly

#define	LDR	0x20 // Load Value from ROM into regA
#define	ADR	0x21 // Add value from ROM into regA

class ALU {
public:
	void invoke() {
		word in = bus;
		regA = add(in, regA);
	}
private:
	word add(word a, word b) {
    flags &= ~(ZERO_FLAG | CARRY_FLAG); // clear flags

    // If the addtion will result in a overflow
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
    if ((flags & JUMP_FLAG) == JUMP_FLAG) {
      count = bus;
      flags &= ~JUMP_FLAG;
    } else {
      count += 1;
    }
		
	}
	void reset() {
		count = 0;
	}
	word count = 0;
} ProgramCounter;

class R0M {
public:
	std::array<word, 128> storage = {};

  int readNumber(std::stringstream &ss){
    int x;
    std:: string n;
    ss >> n;
    return stoi(n);
  }

  void loadRom() {
    std::stringstream ss(assem);
    int i = 0;

    do {
      std::string s;
      ss >> s;  

      if(s == "NOP") {
        addToStorage(i, NOP, 0);
      }
      else if(s == "LDI") {
        addToStorage(i, LDI, readNumber(ss));
      }
      else if(s == "LDA") {
        addToStorage(i, LDA, readNumber(ss));
      }
      else if(s == "STA") {
        addToStorage(i, STA, readNumber(ss));
      }

      else if(s == "ADD") {
        addToStorage(i, ADD, readNumber(ss));
      }
      else if(s == "JMP") {
        addToStorage(i, JMP, readNumber(ss));
      }
      else if(s == "JZ") {
        addToStorage(i, JZ, readNumber(ss));
      }
      else if(s == "JC") {
        addToStorage(i, JC, readNumber(ss));
      } 
      else if(s == "LDR") {
        addToStorage(i, LDR, readNumber(ss));
      }
      else if(s == "ADR") {
        addToStorage(i, ADR, readNumber(ss));
      }

      else if (s == "OUT") {
        addToStorage(i, OUT, 0);
      } 

      else if (s == "HLT") {
        addToStorage(i, HLT, 0);
      } else {
        break;
      }

      i++;
    } while (ss.peek() != EOF);
  }

private: 
  const std::string assem = R"ASS(
  LDI 1
  STA 1
  LDI 0

  OUT

  ADD 1
  STA 2
  LDA 1
  STA 0
  LDA 2
  STA 1
  LDA 0
  
  JC  0
  JMP 3

  HLT
  )ASS";

  void addToStorage(int i, int inst, int n) {
    int k = 0;
    k += inst << 0x8;
    k += n;
    storage[i] = k;
  } 

} Rom;

class RAM {
public:
	std::array<word, 128> storage;
} Ram;

int main() {
  Rom.loadRom();
	while (true) {
		auto currentInstructionFull = Rom.storage[ProgramCounter.count];
		auto instruction = (word)(currentInstructionFull >> 0x8);
		auto location = (word)(currentInstructionFull & 0x00FF);
	
		switch (instruction) {
		case NOP:
			break;

		// Loads ram location into regA
		case LDA: 
			regA = Ram.storage[location];
			break;
    // Loads rom location into regA
		case LDR:
			regA = Rom.storage[location];
			break;
    // Loads immediate value (max half of LDA and LDR)
		case LDI:
			regA = location;
			break;

		// Stores regA into ram 
		case STA:
			Ram.storage[location] = regA;
			break;

		// Adds ram value to regA
		case ADD:
			bus = Ram.storage[location];
			Alu.invoke();
			break;
    // Adds immediate value to regA
		case ADI:
			bus = location;
			Alu.invoke();
			break;

		case OUT:
			std::cout << std::dec << (int)regA << std::endl;
			break;

		case JMP:
			bus = location;
      flags |= JUMP_FLAG;
			break;
		case JC:
			if ((flags & CARRY_FLAG) == CARRY_FLAG) {
				bus = location;
        flags |= JUMP_FLAG;
			}
			break;
		case JZ:
			if ((flags & ZERO_FLAG) == ZERO_FLAG) {
				bus = location;
        flags |= JUMP_FLAG;
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