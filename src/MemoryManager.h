#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <cstdint>
#include <cstdio>

#include <elfio/elfio.hpp>

#define MEMORYSIZE 104857600 // 100MB

class Cache;

class MemoryManager
{
public:
  MemoryManager();
  ~MemoryManager();

  bool copyFrom(const void *src, uint32_t dest, uint32_t len);

  bool setByte(uint32_t addr, uint8_t val);
  uint8_t getByte(uint32_t addr);

  bool setShort(uint32_t addr, uint16_t val);
  uint16_t getShort(uint32_t addr);

  bool setInt(uint32_t addr, uint32_t val);
  uint32_t getInt(uint32_t addr);

  bool setLong(uint32_t addr, uint64_t val);
  uint64_t getLong(uint32_t addr);

private:
  bool isAddrExist(uint32_t addr);

  uint8_t memory[MEMORYSIZE];
};

#endif