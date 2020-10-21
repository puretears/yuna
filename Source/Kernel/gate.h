#ifndef __GATE_H__
#define __GATE_H__

typedef struct {
  unsigned int low;
  unsigned int high;
} descriptor;

typedef struct {
  unsigned long low;
  unsigned long high;
} tss_descriptor;

typedef struct {
  unsigned long low;
  unsigned long high;
} gate_descriptor;

// These extern symbols are defined in head.S
extern descriptor gdt[];
extern gate_descriptor idt[];
extern unsigned int tss[26];

void set_intr_gate64(unsigned int n, unsigned char ist, void *handler);
void set_trap_gate64(unsigned int n, unsigned char ist, void *handler);
// inline void set_user_intr_gate64(unsigned int n, unsigned char ist, void *handler);
// inline void set_user_trap_gate64(unsigned int n, unsigned char ist, void *handler);

void load_tr(unsigned char sel);
void set_tss64(
  unsigned long rsp0, unsigned long rsp1, unsigned long rsp2,
  unsigned long ist1, unsigned long ist2, unsigned long ist3,
  unsigned long ist4, unsigned long ist5, unsigned long ist6,
  unsigned long ist7);
void set_tss64_desc(descriptor *desc);
 
#endif
