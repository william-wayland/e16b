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

// This is the assembly which will be loaded into rom
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

using word = char16_t;

word regA = 0; // used for general instructions
word regB = 0; // used for ALU and JUMP instruction

// 0, 0, 0, 0, 0, Jump, Zero, Carry
word flags;

constexpr word CARRY_FLAG = 0b001; // Last ALU invocation resulted in overflow
constexpr word ZERO_FLAG  = 0b010; // Last ALU invocation resulted in zero
constexpr word JUMP_FLAG  = 0b100; // Last instruction was a JUMP type

// instructions
constexpr uint8_t NOP = 0x00; // Null Operation
constexpr uint8_t LDA = 0x01; // Load value from RAM into regA
constexpr uint8_t STA = 0x02; // Store value from regA into RAM
constexpr uint8_t ADD = 0x03; // Load value from ram and add to regA
constexpr uint8_t SUB = 0x04; // Not implemented
constexpr uint8_t OUT = 0x05; // Output regA to console
constexpr uint8_t JMP = 0x06; // Set PC to immediate value
constexpr uint8_t JC  = 0x07; // Set PC to immediate value if carry flag set
constexpr uint8_t JZ  = 0x08; // Set PC to immediate value if zero flag set
constexpr uint8_t HLT = 0x0a; // Halt program
constexpr uint8_t LDI = 0x10; // Load immediate value into regA
constexpr uint8_t ADI = 0x11; // Add immediate value into regA
constexpr uint8_t LDR = 0x20; // Load Value from ROM into regA
constexpr uint8_t ADR = 0x21; // Add value from ROM into regA

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
    regA = add(regA, regB);
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
      count = regB;
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
} rom;

class RAM {
public:
  std::array<word, 128> storage;
} ram;

int main() {
  while (true) {
    uint8_t instruction = (uint8_t)(rom.storage[program_counter.count] >> 0x8);
    uint8_t location    = (uint8_t)(rom.storage[program_counter.count] & 0x00FF);
  
    switch (instruction) {
    case NOP:
      break;
    case LDA: 
      regA = ram.storage[location];
      break;
    case LDR:
      regA = rom.storage[location];
      break;
    case LDI:
      regA = location;
      break;
    case STA:
      ram.storage[location] = regA;
      break;
    case ADD:
      regB = ram.storage[location];
      alu.invoke();
      break;
    case ADI:
      regB = location;
      alu.invoke();
      break;
    case OUT:
      std::cout << std::dec << (int)regA << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      break;
    case JMP:
      regB = location;
      flags |= JUMP_FLAG;
      break;
    case JC:
      if (flags & CARRY_FLAG) {
        regB = location;
        flags |= JUMP_FLAG;
      }
      break;
    case JZ:
      if (flags & ZERO_FLAG) {
        regB = location;
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