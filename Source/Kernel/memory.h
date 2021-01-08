#ifndef __MEMORY_H__
#define __MEMORY_H__

#define E820_MAX_ENTRIES 32

#define PAGE_OFFSET ((unsigned long)0xFFFF800000000000)

#define PAGE_2M_SHIFT 21
#define PAGE_2M_SIZE (1UL << PAGE_2M_SHIFT)
#define PAGE_2M_MASK (~(PAGE_2M_SIZE - 1))
#define PAGE_2M_ALIGN(addr) (((unsigned long)(addr) + PAGE_2M_SIZE - 1) & PAGE_2M_MASK)

#define VirtToPhy(addr) ((unsigned long)addr - PAGE_OFFSET)
#define PhyToVirt(addr) ((unsigned long *)((unsigned long)addr + PAGE_OFFSET))

/// Page Attribute
#define PG_Mapped       (1 << 0)
#define PG_Kernel_Init  (1 << 1)

#define PG_Active       (1 << 4)
#define PG_Kernel       (1 << 7)

/// Zone Type
#define ZONE_DMA (1 << 0)
#define ZONE_NORMAL (1 << 1)
#define ZONE_UNMAPED (1 << 2)

int ZONE_DMA_INDEX	= 0;
int ZONE_NORMAL_INDEX	= 0;	// [0-1)GB was mapped in pagetable
int ZONE_UNMAPED_INDEX	= 0;	// [1, +] was unmapped

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

  // A bit map that keeps track of each physical memory page.
  // One bit per page.
  // 1 - busy page; 0 - free page;
  unsigned long *bit_map;
  // How many pages are maintained by the bit map
  unsigned long bit_count;
  // The bytes of the bit map (long aligned)
  unsigned long bit_count_length;

  struct page *pages;
  // How many physical memory pages that are represented by the page
  // struct, one page struct for one physical memory page.
  unsigned long page_struct_count;
  // The bytes of the page struct area (long aligned)
  unsigned long total_page_struct_length;

  struct zone *zones;
  // How many zones is the memory divided into
  unsigned long zone_struct_count;
  // The bytes of the zone struct area (long aligned)
  unsigned long total_zone_struct_length;

  unsigned long code_start;
  unsigned long code_end;
  unsigned long data_start;
  unsigned long data_end;
  unsigned long brk_end;

  unsigned long struct_end;
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
  unsigned long total_page_ref_count;

  unsigned long phy_start_addr;
  unsigned long phy_end_addr;
  unsigned long length;
  unsigned long attr;

  struct memory_descriptor *gmd;
};

extern struct memory_descriptor gmd; // global memory descriptor

void memory_init();
struct page *alloc_pages(int zone_type, int number, unsigned long flags);

unsigned long get_cr3();
void flush_tlb();

#endif
