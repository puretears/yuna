#include "lib.h"
#include "trap.h"
#include "gate.h"
#include "printk.h"

void Start_Kernel() {
  // int *addr = (int *)0xffff800000a00000;
	// int i;

	// for(i = 0 ;i<1440*20; i++)
	// {
	// 	*((char *)addr+0)=(char)0x00;
	// 	*((char *)addr+1)=(char)0x00;
	// 	*((char *)addr+2)=(char)0xff;
	// 	*((char *)addr+3)=(char)0x00;	
	// 	addr +=1;	
	// }
	// for(i = 0 ;i<1440*20;i++)
	// {
	// 	*((char *)addr+0)=(char)0x00;
	// 	*((char *)addr+1)=(char)0xff;
	// 	*((char *)addr+2)=(char)0x00;
	// 	*((char *)addr+3)=(char)0x00;	
	// 	addr +=1;	
	// }
	// for(i = 0 ;i<1440*20;i++)
	// {
	// 	*((char *)addr+0)=(char)0xff;
	// 	*((char *)addr+1)=(char)0x00;
	// 	*((char *)addr+2)=(char)0x00;
	// 	*((char *)addr+3)=(char)0x00;	
	// 	addr +=1;	
	// }
	// for(i = 0 ;i<1440*20;i++)
	// {
	// 	*((char *)addr+0)=(char)0xff;
	// 	*((char *)addr+1)=(char)0xff;
	// 	*((char *)addr+2)=(char)0xff;
	// 	*((char *)addr+3)=(char)0x00;	
	// 	addr +=1;	
	// }

	pos.scn_width = 1440;
	pos.scn_height = 900;
	pos.char_x = 0;
	pos.char_y = 0;
	pos.char_width = 8;
	pos.char_height = 16;
	pos.fb_addr = (unsigned int *)0xFFFF800000A00000;
	pos.fb_length = pos.scn_width * pos.scn_height * 4;

	// printk(YELLOW, BLACK, "The octal of 16 is: %o\n", 16);

	int i = -10;
	printk(YELLOW, BLACK, "Address d: %8.6d. %%", i);

	tss_init();
	load_tr(8);
	
	vector_init();

	int j = 1;
	--j;
	int b = 3 / j;
  while(1);
}