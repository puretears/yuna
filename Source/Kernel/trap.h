#ifndef __TRAP_H__
#define __TRAP_H__

void divide_error(); // Defined in entry.S

void tss_init();
void vector_init();
#endif
