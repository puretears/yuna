#include "printk.h"
#include "memory.h"

void memory_init() {
  unsigned int entries = *(unsigned int*)0xFFFF800000007E00;
  struct e820_entry *p_entry = (struct e820_entry *)0xFFFF800000007E40;

  printk(BLUE, BLACK, 
    "Physical Address Map, Type(\
    1:RAM, 2:ROM, 3:ACPI Reclaim Memory, 4:ACPI NVS Memory, Other:Undefined)\n");
  
  int i = 0;
  while (i < entries) {
    printk(ORANGE, BLACK, "Address:%#18lx\tSize:%#18lx\tType:%10x\n", 
      p_entry->addr, p_entry->size, p_entry->type);
    ++p_entry;
    ++i;
  }
}
