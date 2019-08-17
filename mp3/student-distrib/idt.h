#ifndef IDT_H_
#define IDT_H_

#include "x86_desc.h"

#define NUM_EXCEPTIONS  20
#define IDT_EXC_19      0x13
#define NUM_INTERRUPTS  16
#define IDT_INT_0       0x20
#define IDT_INT_F       0x2F
#define IRQ_KB          1
#define IRQ_RTC         8
#define IRQ_PIT         0
#define IDT_SYS_CALL    0x80

/* Exception stub labels */
extern void exc00(void);
extern void exc01(void);
extern void exc02(void);
extern void exc03(void);
extern void exc04(void);
extern void exc05(void);
extern void exc06(void);
extern void exc07(void);
extern void exc08(void);
extern void exc09(void);
extern void exc0A(void);
extern void exc0B(void);
extern void exc0C(void);
extern void exc0D(void);
extern void exc0E(void);
extern void exc0F(void);
extern void exc10(void);
extern void exc11(void);
extern void exc12(void);
extern void exc13(void);

/* Interrupt stub labels */
extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irqA(void);
extern void irqB(void);
extern void irqC(void);
extern void irqD(void);
extern void irqE(void);
extern void irqF(void);

void do_exc(int exc_number);
void do_irq(int irq_number, uint32_t proc_push_top, uint32_t pushed_cs);
void set_all_idt(idt_desc_t* idt);

extern void return_from_intr(void);
extern void iret_and_save_tss_esp(void);

#endif
