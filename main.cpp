#include <iostream>
#include <memory>
#include <array>
#include <bitset>
#include <ios>
#include <limits>
#include <string>
#include <sstream>
#include <cstdlib>
#include <string_view>
#include <thread>
#include <unordered_map>

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
#define STA 0x02 // Store Value from regA into RAM
#define ADD 0x03 // Load value from ram and add to regA
#define SUB 0x04 

#define OUT 0x05

#define JMP 0x06
#define JC  0x07
#define JZ  0x08

#define HLT 0x0a

#define LDI 0x10 // Load instantly
#define ADI 0x11 // Add  instantly

#define LDR 0x20 // Load Value from ROM into regA
#define ADR 0x21 // Add value from ROM into regA

static const std::unordered_map<std::string, uint8_t> INSTRUCTION_STR {
  {"NOP", NOP},
  {"LDA", LDA},
  {"STA", STA},
  {"ADD", ADD},
  {"SUB", SUB},
  {"OUT", OUT},
  {"JMP", JMP},
  {"JC",  JC},
  {"JZ",  JZ},
  {"HLT", HLT},
  {"LDI", LDI},
  {"ADI", ADI},
  {"LDR", LDR},
  {"ADR", ADR}
} ;

static const std::unordered_map<std::string, bool> INSTRUCTION_HAS_VALUE {
  {"NOP", false},
  {"LDA", true},
  {"STA", true},
  {"ADD", true},
  {"SUB", true},
  {"OUT", false},
  {"JMP", true},
  {"JC",  true},
  {"JZ",  true},
  {"HLT", false},
  {"LDI", true},
  {"ADI", true},
  {"LDR", true},
  {"ADR", true}
} ;


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
} alu;

class PC {
public:
  void invoke() {
    if (flags & JUMP_FLAG) {
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
} program_counter;

class ROM {
public:

  ROM() 
  : storage{loadRom()}
  {}

  using ROM_TYPE = std::array<word, 128>;

  const ROM_TYPE storage;

private:
  static uint8_t getValue(std::stringstream &ss) {
    int n;
    ss >> n;
    return n;
  }

  static word getRomEntry(uint8_t inst, uint8_t value) {
    int k = (inst << 0x8) + value;
    return k;
  } 

  static ROM_TYPE loadRom() {
    std::stringstream ss(static_cast<std::string>(assem));
    int i = 0;
    std::string s;

    ROM_TYPE storage{};

    do {
      ss >> s;  

      const auto ihv  = INSTRUCTION_HAS_VALUE.find(s);
      const auto istr = INSTRUCTION_STR.find(s);
      if (istr == INSTRUCTION_STR.end() || ihv == INSTRUCTION_HAS_VALUE.end()) {
        std::cout << "Failed to parse assembly: Unrecognised instruction: \"" << s << "\"" << std::endl;
        exit(-1);
      }

      const bool has_value = ihv->second;
      const uint8_t instruction = istr->second;
      
      storage[i] = (instruction << 0x8);
      
      if (has_value) {
        storage[i] += getValue(ss);
      }

      i++;
    } while (ss.peek() != EOF);

    return storage;
  }

  static constexpr std::string_view assem = R"ASS(
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

} rom;

class RAM {
public:
  std::array<word, 128> storage;
} ram;

int main() {
  while (true) {
    auto currentInstructionFull = rom.storage[program_counter.count];
    auto instruction = (word)(currentInstructionFull >> 0x8);
    auto location = (word)(currentInstructionFull & 0x00FF);
  
    switch (instruction) {
    case NOP:
      break;
    // Loads ram location into regA
    case LDA: 
      regA = ram.storage[location];
      break;
    // Loads rom location into regA
    case LDR:
      regA = rom.storage[location];
      break;
    // Loads immediate value (max half of LDA and LDR)
    case LDI:
      regA = location;
      break;
    // Stores regA into ram 
    case STA:
      ram.storage[location] = regA;
      break;
    // Adds ram value to regA
    case ADD:
      bus = ram.storage[location];
      alu.invoke();
      break;
    // Adds immediate value to regA
    case ADI:
      bus = location;
      alu.invoke();
      break;
    case OUT:
      std::cout << std::dec << (int)regA << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      break;
    case JMP:
      bus = location;
      flags |= JUMP_FLAG;
      break;
    case JC:
      if (flags & CARRY_FLAG) {
        bus = location;
        flags |= JUMP_FLAG;
      }
      break;
    case JZ:
      if (flags & ZERO_FLAG) {
        bus = location;
        flags |= JUMP_FLAG;
      }
      break;
    case HLT:
      while (1);
      break;
    }
    program_counter.invoke();
  }
}