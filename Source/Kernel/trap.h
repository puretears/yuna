#ifndef __TRAP_H__
#define __TRAP_H__

void divide_error(); // Defined in entry.S
void debug();
void nmi();

void int3();
void overflow();
void bounds();

void invalid_opcode();
void device_unavailable();
void double_fault();
void coprocessor_segment_overrun();
void invalid_tss();
void segment_not_present();
void stack_segment_fault();
void general_protection();
void page_fault();

void x87_fpu_error();
void alignment_check();
void machine_check();
void simd_fp_exception();
void virtualization_exception();
void control_protection_exception();

void tss_init();
void vector_init();
#endif
