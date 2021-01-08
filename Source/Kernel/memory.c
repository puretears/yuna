#include "lib.h"
#include "task.h"
#include "printk.h"
#include "memory.h"

void e820_entries_init();
unsigned long get_total_physical_memory();
void bit_map_init();
void pages_init();
void zones_init();
void page_table_init();
unsigned long page_init(struct page *pp, unsigned long flags);

unsigned long *invalid_user_mapping();

void print_physical_memory_info();
void print_page_table(struct page *pp);

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
  // invalid_user_mapping();

  print_physical_memory_info();
  struct page *allocated = alloc_pages(ZONE_NORMAL, 16, PG_Mapped | PG_Active | PG_Kernel_Init);
  print_page_table(allocated);
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
    
    pz->phy_start_addr = start;
    pz->phy_end_addr = end;
    pz->length = end - start;
    pz->busy_page_count = 0;
    pz->free_page_count = pz->length >> PAGE_2M_SHIFT;
    pz->page_count = pz->length >> PAGE_2M_SHIFT;
    pz->total_page_ref_count = 0;
    pz->attr = 0;
    pz->pages = (struct page *)(gmd.pages + (start >> PAGE_2M_SHIFT));

    pz->gmd = &gmd;

    pp = pz->pages;
    for(int j = 0; j < pz->page_count; ++j) {
      pp->zone_area = pz;
      pp->phy_addr = start + PAGE_2M_SIZE * j;
      pp->attr = 0;
      pp->ref_count = 0;
      pp->age = 0;

      // (pp->phy_addr) >> PAGE_2M_SHIFT: The index of the page
      // ((pp->phy_addr) >> PAGE_2M_SHIFT) >> 6:
      //   The index of the page that is calculated in the size of long.
      // (pp->phy_addr >> PAGE_2M_SHIFT) % 64:
      //  The offset of the page that is relative to the above calculated index.
      // Because we have initialize all the bits to 1 previously, here the
      // XOR will set these available pages to free.
      *(gmd.bit_map + (((pp->phy_addr) >> PAGE_2M_SHIFT) >> 6)) ^= 
        1UL << ((pp->phy_addr >> PAGE_2M_SHIFT) % 64);
      ++pp;
    }
  }

  unsigned long total_size = gmd.zone_struct_count * sizeof(struct zone);
  gmd.total_zone_struct_length = (total_size + sizeof(long) - 1) & (~(sizeof(long) - 1));

  // - TODO:
  // Calculate memory zones (DMA, NORMAL, UNMAPPED) correctly.

  gmd.struct_end = 
    ((unsigned long)gmd.zones + gmd.total_zone_struct_length + sizeof(long) - 1) & (~ (sizeof(long) - 1));
}

// Mark 0 - gmd.struct_end pages busy.
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
  /**
   * - TODO:
   * Handle referenced only pages correctly.
  */
  else {
    *(gmd.bit_map + ((pp->phy_addr >> PAGE_2M_SHIFT) >> 6)) |=
      1UL << ((pp->phy_addr >> PAGE_2M_SHIFT) % 64);
    pp->attr |= flags;
  }

  return 0;
}

unsigned long *invalid_user_mapping() {
  unsigned long pml4_addr = get_cr3() & (~0xFFF);

  // Only the first PML4 record takes charge of user
  // address space mapping during kernel bootstrap.
  // and now we do not need this mapping any more.
  *PhyToVirt(pml4_addr) = 0UL;
  flush_tlb();
  return (unsigned long*)pml4_addr;
}

/// Alloc `number` pages with `flags` from `zone_type`.
struct page *alloc_pages(int zone_type, int number, unsigned long flags) {
  int zone_start_index = 0;
  int zone_end_index = 0;

  switch (zone_type) {
    case ZONE_DMA:
      zone_start_index = 0;
      zone_end_index = ZONE_DMA_INDEX;
      break;
    case ZONE_NORMAL:
      zone_start_index = ZONE_DMA_INDEX;
      zone_end_index = ZONE_NORMAL_INDEX;
      break;
    case ZONE_UNMAPED:
      zone_start_index = ZONE_UNMAPED_INDEX;
      zone_end_index = gmd.zone_struct_count - 1;
      break;
    default:
      printk(RED, BLACK, "%s: Invalid zone_type: %d.\n", __func__, zone_type);
  }

  unsigned long page_begin = 0;

  for (int i = zone_start_index; i <= zone_end_index; ++i) {
    struct zone *pz = gmd.zones + i;

    if (pz->free_page_count < number) {
      continue;
    }

    unsigned long start = pz->phy_start_addr >> PAGE_2M_SHIFT;
    unsigned long end = pz->phy_end_addr >> PAGE_2M_SHIFT;
    unsigned long length = pz->length >> PAGE_2M_SHIFT;

    unsigned int tmp = 64 - start % 64;
    for (unsigned int j = start; j <= end; j += j % 64 ? tmp : 64) {
      unsigned long *pm = gmd.bit_map + (j >> 6);
      unsigned long shift = j % 64;

      for (int k = shift; k < 64 - shift; ++k) {
        unsigned long bits = (*pm >> k) | (*(pm + 1) << (64 - k));
        unsigned long mask = number == 64 ? 0xFFFFFFFFFFFFFFFFUL : ((1 << number) - 1);

        if (!(bits & mask)) {
          // We have enough pages from page index k
          page_begin = j + (k - 1);
          for (int l = 0; l < number; ++l) {
            // The kth page address is page + (k - 1)
            struct page *pp = gmd.pages + page_begin + l;
            page_init(pp, flags);
          }

          goto find_free_pages;
        }
      }
    }
  }

  return NULL;
find_free_pages:
  return gmd.pages + page_begin;
}

void print_physical_memory_info() {
  printk(GREEN, BLACK, ">>>>>>>>>>>>>>> Kernel Image <<<<<<<<<<<<<<<\n");
  printk(GREEN, BLACK, "Code begins:\t %#018lx\n", gmd.code_start);
  printk(GREEN, BLACK, "Code ends:\t %#018lx\n", gmd.code_end);
  printk(GREEN, BLACK, "Data begins:\t %#018lx\n", gmd.data_start);
  printk(GREEN, BLACK, "Data ends:\t %#018lx\n", gmd.data_end);
  printk(GREEN, BLACK, "Kernel ends:\t %#018lx\n", gmd.brk_end);
  printk(GREEN, BLACK, "\n");
  
  printk(WHITE, BLACK, ">>>>>>>>>>>>>>> Physical Memory Info <<<<<<<<<<<<<<<\n");
  printk(WHITE, BLACK, 
    "Physical Address Map, Type(1:RAM, 2:ROM, 3:ACPI Reclaim Memory, 4:ACPI NVS Memory, Other:Undefined)\n");
  
  for (unsigned int i = 0; i < gmd.e820_entries; ++i) {
    printk(WHITE, BLACK, "Address:%#18.16lx    Size:%#18.16lx    Type:%2x\n\n", 
      gmd.e820_table[i].addr, 
      gmd.e820_table[i].size,
      gmd.e820_table[i].type);
  }

  printk(WHITE, BLACK, 
    "Total available memory: %#18.16lx Bytes.\n\n", get_total_available_memory());

  printk(ORANGE, BLACK, ">>>>>>>>>>>>>>> Paging Info <<<<<<<<<<<<<<<\n");
  printk(ORANGE, BLACK, 
    "Memory Bit Map: %#18.16lx,  Pages: %#10lx,  Length: %#10lx\n",
    gmd.bit_map, gmd.bit_count, gmd.bit_count_length);

  printk(ORANGE, BLACK,
    "Page Struct:    %#18.16lx,  Count: %#10lx,  Length: %#10lx\n",
    gmd.pages, gmd.page_struct_count, gmd.total_page_struct_length);

  printk(ORANGE, BLACK,
    "Zone Struct:    %#18.16lx,  Count: %#10lx,  Length: %#10lx\n\n",
    gmd.zones, gmd.zone_struct_count, gmd.total_zone_struct_length);

  for (unsigned int i = 0; i < gmd.zone_struct_count; ++i) {
    printk(ORANGE, BLACK,
      "Zone start: %#18.16lx Zone end: %#18.16lx\n\n", 
      gmd.zones[i].phy_start_addr, gmd.zones[i].phy_end_addr);
  }

  unsigned long cr3 = get_cr3();
  printk(ORANGE, BLACK, "CR3: %#18.16lx\n\n", cr3);

  unsigned long *pml4e = (unsigned long *)((*PhyToVirt(cr3 + 256 * 8)) & (~0xFFF));
  printk(INDIGO, BLACK, "PML4E[0]: %#018lx\n", pml4e);
  unsigned long *pdpte = (unsigned long *)((*PhyToVirt(pml4e)) & (~0xFFF));
  printk(INDIGO, BLACK, "PDPTE[0]: %#018lx\n", pdpte);
}

void print_page_table(struct page *pp) {
  for (int i = 0; i < 20; ++i, ++pp) {
    printk(YELLOW, BLACK, "page%d\tAddress: %#18.16lx  Attr: %#18.16lx\t",
      i, pp->phy_addr, pp->attr);
    ++i;
    ++pp;
    printk(YELLOW, BLACK, "page%d\tAddress: %#18.16lx  Attr: %#18.16lx\n",
      i, pp->phy_addr, pp->attr);
  }
}

inline unsigned long get_cr3() {
  unsigned long cr3;
  __asm__ __volatile__(
    "movq %%cr3, %0\n\t"
    :"=r"(cr3)
    :
    :"memory"
  );

  return cr3;
}

inline void flush_tlb() {
  unsigned long reg;

  __asm__ __volatile__(
    "movq %%cr3, %0 \n\t"
    "movq %0, %%cr3 \n\t"
    :"=r"(reg)
    :
    :"memory"
  );
}
