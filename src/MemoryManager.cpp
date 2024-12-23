#include "MemoryManager.h"
#include "Debug.h"

#include <cstdio>
#include <string>

MemoryManager::MemoryManager() {

}

MemoryManager::~MemoryManager() {

}

bool MemoryManager::copyFrom(const void *src, uint32_t dest, uint32_t len) {
  for (uint32_t i = 0; i < len; ++i) {
    if (!this->isAddrExist(dest + i)) {
      dbgprintf("Data copy to invalid addr 0x%x!\n", dest + i);
      return false;
    }
    this->setByte(dest + i, ((uint8_t *)src)[i]);
  }
  return true;
}

bool MemoryManager::setByte(uint32_t addr, uint8_t val) {
  if (!this->isAddrExist(addr)) {
    dbgprintf("Byte write to invalid addr 0x%x!\n", addr);
    return false;
  }
  this->memory[addr] = val;
  return true;
}

uint8_t MemoryManager::getByte(uint32_t addr) {
  if (!this->isAddrExist(addr)) {
    dbgprintf("Byte read to invalid addr 0x%x!\n", addr);
    return false;
  }
  return this->memory[addr];
}

bool MemoryManager::setShort(uint32_t addr, uint16_t val) {
  if (!this->isAddrExist(addr)) {
    dbgprintf("Short write to invalid addr 0x%x!\n", addr);
    return false;
  }
  this->setByte(addr, val & 0xFF);
  this->setByte(addr + 1, (val >> 8) & 0xFF);
  return true;
}

uint16_t MemoryManager::getShort(uint32_t addr) {
  uint32_t b1 = this->getByte(addr);
  uint32_t b2 = this->getByte(addr + 1);
  return b1 + (b2 << 8);
}

bool MemoryManager::setInt(uint32_t addr, uint32_t val) {
  if (!this->isAddrExist(addr)) {
    dbgprintf("Int write to invalid addr 0x%x!\n", addr);
    return false;
  }
  this->setByte(addr, val & 0xFF);
  this->setByte(addr + 1, (val >> 8) & 0xFF);
  this->setByte(addr + 2, (val >> 16) & 0xFF);
  this->setByte(addr + 3, (val >> 24) & 0xFF);
  return true;
}

uint32_t MemoryManager::getInt(uint32_t addr) {
  uint32_t b1 = this->getByte(addr);
  uint32_t b2 = this->getByte(addr + 1);
  uint32_t b3 = this->getByte(addr + 2);
  uint32_t b4 = this->getByte(addr + 3);
  return b1 + (b2 << 8) + (b3 << 16) + (b4 << 24);
}

bool MemoryManager::setLong(uint32_t addr, uint64_t val) {
  if (!this->isAddrExist(addr)) {
    dbgprintf("Long write to invalid addr 0x%x!\n", addr);
    return false;
  }
  this->setByte(addr, val & 0xFF);
  this->setByte(addr + 1, (val >> 8) & 0xFF);
  this->setByte(addr + 2, (val >> 16) & 0xFF);
  this->setByte(addr + 3, (val >> 24) & 0xFF);
  this->setByte(addr + 4, (val >> 32) & 0xFF);
  this->setByte(addr + 5, (val >> 40) & 0xFF);
  this->setByte(addr + 6, (val >> 48) & 0xFF);
  this->setByte(addr + 7, (val >> 56) & 0xFF);
  return true;
}

uint64_t MemoryManager::getLong(uint32_t addr) {
  uint64_t b1 = this->getByte(addr);
  uint64_t b2 = this->getByte(addr + 1);
  uint64_t b3 = this->getByte(addr + 2);
  uint64_t b4 = this->getByte(addr + 3);
  uint64_t b5 = this->getByte(addr + 4);
  uint64_t b6 = this->getByte(addr + 5);
  uint64_t b7 = this->getByte(addr + 6);
  uint64_t b8 = this->getByte(addr + 7);
  return b1 + (b2 << 8) + (b3 << 16) + (b4 << 24) + (b5 << 32) + (b6 << 40) +
         (b7 << 48) + (b8 << 56);
}

bool MemoryManager::isAddrExist(uint32_t addr) {
  if (addr >= MEMORYSIZE) return false;
  return true;
}
