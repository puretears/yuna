#include "lib.h"
#include "task.h"
#include "printk.h"
#include "memory.h"

void e820_entries_init();
void print_physical_memory_info();
unsigned long get_total_physical_memory();
void bit_map_init();
void pages_init();
void zones_init();
void page_table_init();
unsigned long page_init(struct page *pp, unsigned long flags);
unsigned long *get_cr3();

void memory_init() {
  gmd.code_start = (unsigned long)(&_text);
  gmd.code_end   = (unsigned long)(&_etext);
  gmd.data_start = (unsigned long)(&_data);
  gmd.data_end   = (unsigned long)(&_edata);
  gmd.brk_end    = (unsigned long)(&_end);

  e820_entries_init();
  bit_map_init();
  pages_init();
  zones_init();
  page_table_init();

  print_physical_memory_info();
}

void e820_entries_init() {
  unsigned int entries = *(unsigned int*)0xFFFF800000007E00;
  gmd.e820_entries = entries;

  struct e820_entry *p_entry = (struct e820_entry *)0xFFFF800000007E40;
  
  int i = 0;
  while (i < entries) {
    gmd.e820_table[i].addr = p_entry->addr;
    gmd.e820_table[i].size = p_entry->size;
    gmd.e820_table[i].type = p_entry->type;

    ++p_entry;
    ++i;
  }
}

void bit_map_init() {
  unsigned long krn_end = gmd.brk_end;
  gmd.bit_map = (unsigned long *)((krn_end + PAGE_2M_SIZE - 1) & PAGE_2M_MASK);

  /// `total_mem_pages` includes all types of physical memories.
  unsigned long total_mem_pages = get_total_physical_memory() >> PAGE_2M_SHIFT;
  gmd.bit_count = total_mem_pages;
  gmd.bit_count_length = ((total_mem_pages + sizeof(long) * 8 - 1) / 8) & (~(sizeof(long) - 1));

  memset(gmd.bit_map, 0xFF, gmd.bit_count_length);
}

void pages_init() {
  unsigned long bit_map_end = (unsigned long)gmd.bit_map + gmd.bit_count_length;

  gmd.pages = (struct page *)((bit_map_end + PAGE_2M_SIZE - 1) & PAGE_2M_MASK);
  gmd.page_struct_count = get_total_physical_memory() >> PAGE_2M_SHIFT;
  unsigned long total_size = gmd.page_struct_count * sizeof(struct page);
  gmd.total_page_struct_length = (total_size + sizeof(long) - 1) & (~(sizeof(long) - 1));

  memset(gmd.pages, 0x0, gmd.total_page_struct_length);
}

void zones_init() {
  unsigned long pages_end = 
    (unsigned long)gmd.pages + gmd.total_page_struct_length;
  gmd.zones = (struct zone *)((pages_end + PAGE_2M_SIZE - 1) & PAGE_2M_MASK);
  gmd.zone_struct_count = 0;

  int i = 0;
  unsigned long start, end;
  struct zone *pz = gmd.zones;
  struct page *pp;
  for(; i < gmd.e820_entries; ++i) {
    if (gmd.e820_table[i].type != 1) {
      continue;
    }

    start = PAGE_2M_ALIGN(gmd.e820_table[i].addr);
    end = ((gmd.e820_table[i].addr + gmd.e820_table[i].size) >> PAGE_2M_SHIFT) << PAGE_2M_SHIFT;

    if (end <= start) {
      continue;
    }

    pz = gmd.zones + gmd.zone_struct_count;
    ++gmd.zone_struct_count;
    
    pz->start_addr = start;
    pz->end_addr = end;
    pz->length = end - start;
    pz->busy_page_count = 0;
    pz->free_page_count = pz->length >> PAGE_2M_SHIFT;
    pz->page_count = pz->length >> PAGE_2M_SHIFT;
    pz->total_page_ref_count = 0;
    pz->attr = 0;
    pz->pages = (struct page *)(gmd.pages + (start >> PAGE_2M_SHIFT));

    pz->gmd = &gmd;

    pp = pz->pages;
    int j = 0;
    for(; j < pz->page_count; ++j) {
      pp->zone_area = pz;
      pp->phy_addr = start + PAGE_2M_SIZE * j;
      pp->attr = 0;
      pp->ref_count = 0;
      pp->age = 0;

      *(gmd.bit_map + (((pp->phy_addr) >> PAGE_2M_SHIFT) >> 6)) ^= 
        1UL << ((pp->phy_addr >> PAGE_2M_SHIFT) % 64);
    }
  }

  unsigned long total_size = gmd.zone_struct_count * sizeof(struct zone);
  gmd.total_zone_struct_length = (total_size + sizeof(long) - 1) & (~(sizeof(long) - 1));
  gmd.struct_end = 
    ((unsigned long)gmd.zones + gmd.total_zone_struct_length + sizeof(long) - 1) & (~ (sizeof(long) - 1));
}

void page_table_init() {
  unsigned long i = VirtToPhy(gmd.struct_end) >> PAGE_2M_SHIFT;
  for (int j = 0; j < i; ++j) {
    page_init(gmd.pages + j, PG_Mapped | PG_Kernel_Init | PG_Active | PG_Kernel);
  }
}

unsigned long get_total_available_memory() {
  int i = 0;
  unsigned long total_mem = 0;

  for (; i < gmd.e820_entries; ++i) {
    if (gmd.e820_table[i].type == 1) {
      total_mem += gmd.e820_table[i].size;
    }
  }

  return total_mem;
}

unsigned long get_total_physical_memory() {
  unsigned long entries = gmd.e820_entries;
  struct e820_entry last = gmd.e820_table[entries - 1];

  return last.addr + last.size;
}

unsigned long page_init(struct page *pp, unsigned long flags) {
  if (!pp->attr) {
    *(gmd.bit_map + ((pp->phy_addr >> PAGE_2M_SHIFT) >> 6)) |=
      1UL << ((pp->phy_addr >> PAGE_2M_SHIFT) % 64);
    pp->attr = flags;
    ++pp->ref_count;
    ++pp->zone_area->busy_page_count;
    --pp->zone_area->free_page_count;
    ++pp->zone_area->total_page_ref_count;
  }
  else if ((pp->attr & PG_Referenced) || (pp->attr & PG_K_Share_To_U) || 
    (flags & PG_Referenced) || (flags & PG_K_Share_To_U)) {
    pp->attr |= flags;
    ++pp->ref_count;
    ++pp->zone_area->total_page_ref_count;
  }
  else {
    *(gmd.bit_map + ((pp->phy_addr >> PAGE_2M_SHIFT) >> 6)) |=
      1UL << ((pp->phy_addr >> PAGE_2M_SHIFT) % 64);
    pp->attr |= flags;
  }

  return 0;
}

unsigned long *get_cr3() {
  unsigned long *gdt_addr;
  __asm__ __volatile__(
    "movq %%cr3, %0\n\t"
    :"=r"(gdt_addr)
    :
    :"memory"
  );

  return gdt_addr;
}

void print_physical_memory_info() {
  printk(WHITE, BLACK, "Code begins:\t%#018lx\n", gmd.code_start);
  printk(WHITE, BLACK, "Code ends:\t%#018lx\n", gmd.code_end);
  printk(WHITE, BLACK, "Data begins:\t%#018lx\n", gmd.data_start);
  printk(WHITE, BLACK, "Data ends:\t%#018lx\n", gmd.data_end);
  printk(WHITE, BLACK, "Kernel ends:\t%#018lx\n", gmd.brk_end);
  
  printk(BLUE, BLACK, 
    "Physical Address Map, Type(1:RAM, 2:ROM, 3:ACPI Reclaim Memory, 4:ACPI NVS Memory, Other:Undefined)\n");
  
  unsigned int i = 0;
  while (i < gmd.e820_entries) {
    printk(ORANGE, BLACK, "Address:%#18lx\tSize:%#18lx\tType:%10x\n", 
      gmd.e820_table[i].addr, 
      gmd.e820_table[i].size,
      gmd.e820_table[i].type);
    ++i;
  }

  printk(ORANGE, BLACK, 
    "Total available memory: %#018lx.\n", get_total_available_memory());

  printk(ORANGE, BLACK, 
    "Memory Bit Map: %#18lx,\tPages: %#18lx,\tLength: %#18lx\n",
    gmd.bit_map, gmd.bit_count, gmd.bit_count_length);

  printk(ORANGE, BLACK,
    "Page Struct: %#18lx,\tCount: %#18lx,\tLength: %#18lx\n",
    gmd.pages, gmd.page_struct_count, gmd.total_page_struct_length);

  printk(ORANGE, BLACK,
    "Zone Struct: %#18lx,\tCount: %#18lx,\tLength: %#18lx\n",
    gmd.zones, gmd.zone_struct_count, gmd.total_zone_struct_length);

  for (i = 0; i < gmd.zone_struct_count; ++i) {
    printk(ORANGE, BLACK, "Zone start: %#18lx\tZone end: %#18lx\n", 
      gmd.zones[i].start_addr, gmd.zones[i].end_addr);
  }

  unsigned long *cr3 = get_cr3();
  printk(ORANGE, BLACK, "CR3: %#018lx\n", cr3);

  unsigned long *pml4e = (unsigned long *)((*PhyToVirt(cr3)) & (~0xFFF));
  printk(INDIGO, BLACK, "PML4E[0]: %#018lx\n", pml4e);
  unsigned long *pdpte = (unsigned long *)((*PhyToVirt(pml4e)) & (~0xFFF));
  printk(INDIGO, BLACK, "PDPTE[0]: %#018lx\n", pdpte);
}

