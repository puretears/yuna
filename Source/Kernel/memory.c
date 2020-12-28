#include "printk.h"
#include "memory.h"

void e820_entries_init();
void print_physical_memory_info();
unsigned long get_total_physical_memory();

void memory_init() {
  e820_entries_init();
  print_physical_memory_info();

}

void e820_entries_init() {
  unsigned int entries = *(unsigned int*)0xFFFF800000007E00;
  global_memory_descriptor.e820_entries = entries;

  struct e820_entry *p_entry = (struct e820_entry *)0xFFFF800000007E40;
  
  int i = 0;
  while (i < entries) {
    global_memory_descriptor.e820_table[i].addr = p_entry->addr;
    global_memory_descriptor.e820_table[i].size = p_entry->size;
    global_memory_descriptor.e820_table[i].type = p_entry->type;

    ++p_entry;
    ++i;
  }
}

unsigned long get_total_available_memory() {
  int i = 0;
  unsigned long total_mem = 0;

  for (; i < global_memory_descriptor.e820_entries; ++i) {
    if (global_memory_descriptor.e820_table[i].type == 1) {
      total_mem += global_memory_descriptor.e820_table[i].size;
    }
  }

  return total_mem;
}

void print_physical_memory_info() {
  printk(BLUE, BLACK, 
    "Physical Address Map, Type(1:RAM, 2:ROM, 3:ACPI Reclaim Memory, 4:ACPI NVS Memory, Other:Undefined)\n");
  
  unsigned int i = 0;
  while (i < global_memory_descriptor.e820_entries) {
    printk(ORANGE, BLACK, "Address:%#18lx\tSize:%#18lx\tType:%10x\n", 
      global_memory_descriptor.e820_table[i].addr, 
      global_memory_descriptor.e820_table[i].size,
      global_memory_descriptor.e820_table[i].type);
    ++i;
  }

  printk(ORANGE, BLACK, "Total available memory: %#018lx.", get_total_available_memory());
}
