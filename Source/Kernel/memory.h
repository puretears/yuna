#ifndef __MEMORY_H__
#define __MEMORY_H__

#define E820_MAX_ENTRIES 32

enum e820_type {
  E820_TYPE_RAM = 1,
  E820_TYPE_RESERVED = 2,
  E820_TYPE_ACPI = 3,
  E820_TYPE_NVS = 4
};

struct e820_entry {
  unsigned long addr;
  unsigned long size;
  unsigned int type;
} __attribute__((packed));

struct memory_descriptor {
  struct e820_entry e820_table[E820_MAX_ENTRIES];
  unsigned long e820_entries;
};

extern struct memory_descriptor global_memory_descriptor;

void memory_init();
#endif
