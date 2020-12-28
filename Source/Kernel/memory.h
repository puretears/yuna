#ifndef __MEMORY_H__
#define __MEMORY_H__

#define E820_MAX_ENTRIES 32

#define PAGE_OFFSET ((unsigned long)0xFFFF800000000000)

#define PAGE_2M_SHIFT 21
#define PAGE_2M_SIZE (1UL << PAGE_2M_SHIFT)
#define PAGE_2M_MASK (~(PAGE_2M_SIZE - 1))
#define PAGE_2M_ALIGN(addr) (((unsigned long)(addr) + PAGE_2M_SIZE - 1) & PAGE_2M_MASK)

struct e820_entry {
  unsigned long addr;
  unsigned long size;
  /**
   * E820_TYPE_RAM = 1,
   * E820_TYPE_RESERVED = 2,
   * E820_TYPE_ACPI = 3,
   * E820_TYPE_NVS = 4
  */
  unsigned int type;
} __attribute__((packed));

struct memory_descriptor {
  struct e820_entry e820_table[E820_MAX_ENTRIES];
  unsigned long e820_entries;

  unsigned long *bit_map;
  unsigned long bit_size;
  unsigned long bit_length;

  struct page *pages;
  unsigned long page_entries;
  unsigned long page_length;

  struct zone *zones;
  unsigned long zone_entries;
  unsigned long zone_length;

  unsigned long code_start;
  unsigned long code_end;
  unsigned long data_start;
  unsigned long data_end;
  unsigned long brk_end;
};

struct page {
  struct zone *zone_area;
  unsigned long phy_addr;
  unsigned long attr;
  unsigned long ref_count;
  unsigned long age;
};

struct zone {
  struct page *pages;
  unsigned long page_count;
  unsigned long busy_page_count;
  unsigned long free_page_count;
  unsigned long total_ref_count;

  unsigned long start_addr;
  unsigned long end_addr;
  unsigned long length;
  unsigned long attr;

  struct memory_descriptor *gmd;
};

extern struct memory_descriptor global_memory_descriptor;

void memory_init();
#endif
