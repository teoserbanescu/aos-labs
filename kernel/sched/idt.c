#include <assert.h>
#include <stdio.h>

#include <x86-64/asm.h>
#include <x86-64/gdt.h>
#include <x86-64/idt.h>

#include <kernel/acpi.h>
#include <kernel/sched/idt.h>
#include <kernel/monitor.h>
#include <kernel/sched/syscall.h>

#include <kernel/sched/sched.h>
#include <kernel/sched/task.h>

#include <kernel/vma/pfault.h>

#include <kernel/acpi/lapic.h>

/* Defined in stubs.S */
extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();
extern void isr32();
extern void isr33();
extern void isr34();
extern void isr35();
extern void isr36();
extern void isr37();
extern void isr38();
extern void isr39();
extern void isr40();
extern void isr41();
extern void isr42();
extern void isr43();
extern void isr44();
extern void isr45();
extern void isr46();
extern void isr47();
extern void isr48();
extern void isr49();
extern void isr50();
extern void isr51();
extern void isr52();
extern void isr53();
extern void isr54();
extern void isr55();
extern void isr56();
extern void isr57();
extern void isr58();
extern void isr59();
extern void isr60();
extern void isr61();
extern void isr62();
extern void isr63();
extern void isr64();
extern void isr65();
extern void isr66();
extern void isr67();
extern void isr68();
extern void isr69();
extern void isr70();
extern void isr71();
extern void isr72();
extern void isr73();
extern void isr74();
extern void isr75();
extern void isr76();
extern void isr77();
extern void isr78();
extern void isr79();
extern void isr80();
extern void isr81();
extern void isr82();
extern void isr83();
extern void isr84();
extern void isr85();
extern void isr86();
extern void isr87();
extern void isr88();
extern void isr89();
extern void isr90();
extern void isr91();
extern void isr92();
extern void isr93();
extern void isr94();
extern void isr95();
extern void isr96();
extern void isr97();
extern void isr98();
extern void isr99();
extern void isr100();
extern void isr101();
extern void isr102();
extern void isr103();
extern void isr104();
extern void isr105();
extern void isr106();
extern void isr107();
extern void isr108();
extern void isr109();
extern void isr110();
extern void isr111();
extern void isr112();
extern void isr113();
extern void isr114();
extern void isr115();
extern void isr116();
extern void isr117();
extern void isr118();
extern void isr119();
extern void isr120();
extern void isr121();
extern void isr122();
extern void isr123();
extern void isr124();
extern void isr125();
extern void isr126();
extern void isr127();
extern void isr128();
extern void isr129();
extern void isr130();
extern void isr131();
extern void isr132();
extern void isr133();
extern void isr134();
extern void isr135();
extern void isr136();
extern void isr137();
extern void isr138();
extern void isr139();
extern void isr140();
extern void isr141();
extern void isr142();
extern void isr143();
extern void isr144();
extern void isr145();
extern void isr146();
extern void isr147();
extern void isr148();
extern void isr149();
extern void isr150();
extern void isr151();
extern void isr152();
extern void isr153();
extern void isr154();
extern void isr155();
extern void isr156();
extern void isr157();
extern void isr158();
extern void isr159();
extern void isr160();
extern void isr161();
extern void isr162();
extern void isr163();
extern void isr164();
extern void isr165();
extern void isr166();
extern void isr167();
extern void isr168();
extern void isr169();
extern void isr170();
extern void isr171();
extern void isr172();
extern void isr173();
extern void isr174();
extern void isr175();
extern void isr176();
extern void isr177();
extern void isr178();
extern void isr179();
extern void isr180();
extern void isr181();
extern void isr182();
extern void isr183();
extern void isr184();
extern void isr185();
extern void isr186();
extern void isr187();
extern void isr188();
extern void isr189();
extern void isr190();
extern void isr191();
extern void isr192();
extern void isr193();
extern void isr194();
extern void isr195();
extern void isr196();
extern void isr197();
extern void isr198();
extern void isr199();
extern void isr200();
extern void isr201();
extern void isr202();
extern void isr203();
extern void isr204();
extern void isr205();
extern void isr206();
extern void isr207();
extern void isr208();
extern void isr209();
extern void isr210();
extern void isr211();
extern void isr212();
extern void isr213();
extern void isr214();
extern void isr215();
extern void isr216();
extern void isr217();
extern void isr218();
extern void isr219();
extern void isr220();
extern void isr221();
extern void isr222();
extern void isr223();
extern void isr224();
extern void isr225();
extern void isr226();
extern void isr227();
extern void isr228();
extern void isr229();
extern void isr230();
extern void isr231();
extern void isr232();
extern void isr233();
extern void isr234();
extern void isr235();
extern void isr236();
extern void isr237();
extern void isr238();
extern void isr239();
extern void isr240();
extern void isr241();
extern void isr242();
extern void isr243();
extern void isr244();
extern void isr245();
extern void isr246();
extern void isr247();
extern void isr248();
extern void isr249();
extern void isr250();
extern void isr251();
extern void isr252();
extern void isr253();
extern void isr254();
extern void isr255();


static const char *int_names[256] = {
	[INT_DIVIDE] = "Divide-by-Zero Error Exception (#DE)",
	[INT_DEBUG] = "Debug (#DB)",
	[INT_NMI] = "Non-Maskable Interrupt",
	[INT_BREAK] = "Breakpoint (#BP)",
	[INT_OVERFLOW] = "Overflow (#OF)",
	[INT_BOUND] = "Bound Range (#BR)",
	[INT_INVALID_OP] = "Invalid Opcode (#UD)",
	[INT_DEVICE] = "Device Not Available (#NM)",
	[INT_DOUBLE_FAULT] = "Double Fault (#DF)",
	[INT_TSS] = "Invalid TSS (#TS)",
	[INT_NO_SEG_PRESENT] = "Segment Not Present (#NP)",
	[INT_SS] = "Stack (#SS)",
	[INT_GPF] = "General Protection (#GP)",
	[INT_PAGE_FAULT] = "Page Fault (#PF)",
	[INT_FPU] = "x86 FPU Floating-Point (#MF)",
	[INT_ALIGNMENT] = "Alignment Check (#AC)",
	[INT_MCE] = "Machine Check (#MC)",
	[INT_SIMD] = "SIMD Floating-Point (#XF)",
	[INT_SECURITY] = "Security (#SX)",
};

static struct idt_entry entries[256];
static struct idtr idtr = {
	.limit = sizeof(entries) - 1,
	.entries = entries,
};

static const char *get_int_name(unsigned int_no)
{
	if (!int_names[int_no])
		return "Unknown Interrupt";

	return int_names[int_no];
}

void print_int_frame(struct int_frame *frame)
{
	cprintf("INT frame at %p\n", frame);

	/* Print the interrupt number and the name. */
	cprintf(" INT %u: %s\n",
		frame->int_no,
		get_int_name(frame->int_no));

	/* Print the error code. */
	switch (frame->int_no) {
	case INT_PAGE_FAULT:
		cprintf(" CR2 %p\n", read_cr2());
		cprintf(" ERR 0x%016llx (%s, %s, %s)\n",
			frame->err_code,
			frame->err_code & 4 ? "user" : "kernel",
			frame->err_code & 2 ? "write" : "read",
			frame->err_code & 1 ? "protection" : "not present");
		break;
	default:
		cprintf(" ERR 0x%016llx\n", frame->err_code);
	}

	/* Print the general-purpose registers. */
	cprintf(" RAX 0x%016llx"
		" RCX 0x%016llx"
		" RDX 0x%016llx"
		" RBX 0x%016llx\n"
		" RSP 0x%016llx"
		" RBP 0x%016llx"
		" RSI 0x%016llx"
		" RDI 0x%016llx\n"
		" R8  0x%016llx"
		" R9  0x%016llx"
		" R10 0x%016llx"
		" R11 0x%016llx\n"
		" R12 0x%016llx"
		" R13 0x%016llx"
		" R14 0x%016llx"
		" R15 0x%016llx\n",
		frame->rax, frame->rcx, frame->rdx, frame->rbx,
		frame->rsp, frame->rbp, frame->rsi, frame->rdi,
		frame->r8,  frame->r9,  frame->r10, frame->r11,
		frame->r12, frame->r13, frame->r14, frame->r15);

	/* Print the IP, segment selectors and the RFLAGS register. */
	cprintf(" RIP 0x%016llx"
		" RFL 0x%016llx\n"
		" CS  0x%04x"
		"            "
		" DS  0x%04x"
		"            "
		" SS  0x%04x\n",
		frame->rip, frame->rflags,
		frame->cs, frame->ds, frame->ss);
}

/* Set up the interrupt handlers. */
void idt_init(void)
{
	/* LAB 3: your code here. */
	uint64_t flags0, flags3;

	flags0 = IDT_PRESENT | IDT_PRIVL(0) | IDT_GATE(0) | IDT_INT_GATE32; 	// kernel
	flags3 = IDT_PRESENT | IDT_PRIVL(3) | IDT_GATE(0) | IDT_INT_GATE32;


	set_idt_entry(&entries[0], isr0, flags0, GDT_KCODE);
	set_idt_entry(&entries[1], isr1, flags0, GDT_KCODE);
	set_idt_entry(&entries[2], isr2, flags0, GDT_KCODE);
	set_idt_entry(&entries[3], isr3, flags3, GDT_KCODE);
	set_idt_entry(&entries[4], isr4, flags0, GDT_KCODE);
	set_idt_entry(&entries[5], isr5, flags0, GDT_KCODE);
	set_idt_entry(&entries[6], isr6, flags0, GDT_KCODE);
	set_idt_entry(&entries[7], isr7, flags0, GDT_KCODE);
	set_idt_entry(&entries[8], isr8, flags0, GDT_KCODE);
	set_idt_entry(&entries[9], isr9, flags0, GDT_KCODE);
	set_idt_entry(&entries[10], isr10, flags0, GDT_KCODE);
	set_idt_entry(&entries[11], isr11, flags0, GDT_KCODE);
	set_idt_entry(&entries[12], isr12, flags0, GDT_KCODE);
	set_idt_entry(&entries[13], isr13, flags0, GDT_KCODE);
	set_idt_entry(&entries[14], isr14, flags0, GDT_KCODE);
	set_idt_entry(&entries[15], isr15, flags0, GDT_KCODE);
	set_idt_entry(&entries[16], isr16, flags0, GDT_KCODE);
	set_idt_entry(&entries[17], isr17, flags0, GDT_KCODE);
	set_idt_entry(&entries[18], isr18, flags0, GDT_KCODE);
	set_idt_entry(&entries[19], isr19, flags0, GDT_KCODE);
	set_idt_entry(&entries[20], isr20, flags0, GDT_KCODE);
//	set_idt_entry(&entries[21], isr21, flags0, GDT_KCODE);
//	set_idt_entry(&entries[22], isr22, flags0, GDT_KCODE);
//	set_idt_entry(&entries[23], isr23, flags0, GDT_KCODE);
//	set_idt_entry(&entries[24], isr24, flags0, GDT_KCODE);
//	set_idt_entry(&entries[25], isr25, flags0, GDT_KCODE);
//	set_idt_entry(&entries[26], isr26, flags0, GDT_KCODE);
//	set_idt_entry(&entries[27], isr27, flags0, GDT_KCODE);
//	set_idt_entry(&entries[28], isr28, flags0, GDT_KCODE);
//	set_idt_entry(&entries[29], isr29, flags0, GDT_KCODE);
//	set_idt_entry(&entries[30], isr30, flags0, GDT_KCODE);
//	set_idt_entry(&entries[31], isr31, flags0, GDT_KCODE);
	set_idt_entry(&entries[32], isr32, flags0, GDT_KCODE);
	set_idt_entry(&entries[33], isr33, flags0, GDT_KCODE);
	set_idt_entry(&entries[34], isr34, flags0, GDT_KCODE);
	set_idt_entry(&entries[35], isr35, flags0, GDT_KCODE);
	set_idt_entry(&entries[36], isr36, flags0, GDT_KCODE);
	set_idt_entry(&entries[37], isr37, flags0, GDT_KCODE);
	set_idt_entry(&entries[38], isr38, flags0, GDT_KCODE);
	set_idt_entry(&entries[39], isr39, flags0, GDT_KCODE);
	set_idt_entry(&entries[40], isr40, flags0, GDT_KCODE);
	set_idt_entry(&entries[41], isr41, flags0, GDT_KCODE);
	set_idt_entry(&entries[42], isr42, flags0, GDT_KCODE);
	set_idt_entry(&entries[43], isr43, flags0, GDT_KCODE);
	set_idt_entry(&entries[44], isr44, flags0, GDT_KCODE);
	set_idt_entry(&entries[45], isr45, flags0, GDT_KCODE);
	set_idt_entry(&entries[46], isr46, flags0, GDT_KCODE);
	set_idt_entry(&entries[47], isr47, flags0, GDT_KCODE);
	set_idt_entry(&entries[48], isr48, flags0, GDT_KCODE);
	set_idt_entry(&entries[49], isr49, flags0, GDT_KCODE);
	set_idt_entry(&entries[50], isr50, flags0, GDT_KCODE);
	set_idt_entry(&entries[51], isr51, flags0, GDT_KCODE);
	set_idt_entry(&entries[52], isr52, flags0, GDT_KCODE);
	set_idt_entry(&entries[53], isr53, flags0, GDT_KCODE);
	set_idt_entry(&entries[54], isr54, flags0, GDT_KCODE);
	set_idt_entry(&entries[55], isr55, flags0, GDT_KCODE);
	set_idt_entry(&entries[56], isr56, flags0, GDT_KCODE);
	set_idt_entry(&entries[57], isr57, flags0, GDT_KCODE);
	set_idt_entry(&entries[58], isr58, flags0, GDT_KCODE);
	set_idt_entry(&entries[59], isr59, flags0, GDT_KCODE);
	set_idt_entry(&entries[60], isr60, flags0, GDT_KCODE);
	set_idt_entry(&entries[61], isr61, flags0, GDT_KCODE);
	set_idt_entry(&entries[62], isr62, flags0, GDT_KCODE);
	set_idt_entry(&entries[63], isr63, flags0, GDT_KCODE);
	set_idt_entry(&entries[64], isr64, flags0, GDT_KCODE);
	set_idt_entry(&entries[65], isr65, flags0, GDT_KCODE);
	set_idt_entry(&entries[66], isr66, flags0, GDT_KCODE);
	set_idt_entry(&entries[67], isr67, flags0, GDT_KCODE);
	set_idt_entry(&entries[68], isr68, flags0, GDT_KCODE);
	set_idt_entry(&entries[69], isr69, flags0, GDT_KCODE);
	set_idt_entry(&entries[70], isr70, flags0, GDT_KCODE);
	set_idt_entry(&entries[71], isr71, flags0, GDT_KCODE);
	set_idt_entry(&entries[72], isr72, flags0, GDT_KCODE);
	set_idt_entry(&entries[73], isr73, flags0, GDT_KCODE);
	set_idt_entry(&entries[74], isr74, flags0, GDT_KCODE);
	set_idt_entry(&entries[75], isr75, flags0, GDT_KCODE);
	set_idt_entry(&entries[76], isr76, flags0, GDT_KCODE);
	set_idt_entry(&entries[77], isr77, flags0, GDT_KCODE);
	set_idt_entry(&entries[78], isr78, flags0, GDT_KCODE);
	set_idt_entry(&entries[79], isr79, flags0, GDT_KCODE);
	set_idt_entry(&entries[80], isr80, flags0, GDT_KCODE);
	set_idt_entry(&entries[81], isr81, flags0, GDT_KCODE);
	set_idt_entry(&entries[82], isr82, flags0, GDT_KCODE);
	set_idt_entry(&entries[83], isr83, flags0, GDT_KCODE);
	set_idt_entry(&entries[84], isr84, flags0, GDT_KCODE);
	set_idt_entry(&entries[85], isr85, flags0, GDT_KCODE);
	set_idt_entry(&entries[86], isr86, flags0, GDT_KCODE);
	set_idt_entry(&entries[87], isr87, flags0, GDT_KCODE);
	set_idt_entry(&entries[88], isr88, flags0, GDT_KCODE);
	set_idt_entry(&entries[89], isr89, flags0, GDT_KCODE);
	set_idt_entry(&entries[90], isr90, flags0, GDT_KCODE);
	set_idt_entry(&entries[91], isr91, flags0, GDT_KCODE);
	set_idt_entry(&entries[92], isr92, flags0, GDT_KCODE);
	set_idt_entry(&entries[93], isr93, flags0, GDT_KCODE);
	set_idt_entry(&entries[94], isr94, flags0, GDT_KCODE);
	set_idt_entry(&entries[95], isr95, flags0, GDT_KCODE);
	set_idt_entry(&entries[96], isr96, flags0, GDT_KCODE);
	set_idt_entry(&entries[97], isr97, flags0, GDT_KCODE);
	set_idt_entry(&entries[98], isr98, flags0, GDT_KCODE);
	set_idt_entry(&entries[99], isr99, flags0, GDT_KCODE);
	set_idt_entry(&entries[100], isr100, flags0, GDT_KCODE);
	set_idt_entry(&entries[101], isr101, flags0, GDT_KCODE);
	set_idt_entry(&entries[102], isr102, flags0, GDT_KCODE);
	set_idt_entry(&entries[103], isr103, flags0, GDT_KCODE);
	set_idt_entry(&entries[104], isr104, flags0, GDT_KCODE);
	set_idt_entry(&entries[105], isr105, flags0, GDT_KCODE);
	set_idt_entry(&entries[106], isr106, flags0, GDT_KCODE);
	set_idt_entry(&entries[107], isr107, flags0, GDT_KCODE);
	set_idt_entry(&entries[108], isr108, flags0, GDT_KCODE);
	set_idt_entry(&entries[109], isr109, flags0, GDT_KCODE);
	set_idt_entry(&entries[110], isr110, flags0, GDT_KCODE);
	set_idt_entry(&entries[111], isr111, flags0, GDT_KCODE);
	set_idt_entry(&entries[112], isr112, flags0, GDT_KCODE);
	set_idt_entry(&entries[113], isr113, flags0, GDT_KCODE);
	set_idt_entry(&entries[114], isr114, flags0, GDT_KCODE);
	set_idt_entry(&entries[115], isr115, flags0, GDT_KCODE);
	set_idt_entry(&entries[116], isr116, flags0, GDT_KCODE);
	set_idt_entry(&entries[117], isr117, flags0, GDT_KCODE);
	set_idt_entry(&entries[118], isr118, flags0, GDT_KCODE);
	set_idt_entry(&entries[119], isr119, flags0, GDT_KCODE);
	set_idt_entry(&entries[120], isr120, flags0, GDT_KCODE);
	set_idt_entry(&entries[121], isr121, flags0, GDT_KCODE);
	set_idt_entry(&entries[122], isr122, flags0, GDT_KCODE);
	set_idt_entry(&entries[123], isr123, flags0, GDT_KCODE);
	set_idt_entry(&entries[124], isr124, flags0, GDT_KCODE);
	set_idt_entry(&entries[125], isr125, flags0, GDT_KCODE);
	set_idt_entry(&entries[126], isr126, flags0, GDT_KCODE);
	set_idt_entry(&entries[127], isr127, flags0, GDT_KCODE);
	set_idt_entry(&entries[128], isr128, flags3, GDT_KCODE);	// syscall (0x80)
	set_idt_entry(&entries[129], isr129, flags0, GDT_KCODE);
	set_idt_entry(&entries[130], isr130, flags0, GDT_KCODE);
	set_idt_entry(&entries[131], isr131, flags0, GDT_KCODE);
	set_idt_entry(&entries[132], isr132, flags0, GDT_KCODE);
	set_idt_entry(&entries[133], isr133, flags0, GDT_KCODE);
	set_idt_entry(&entries[134], isr134, flags0, GDT_KCODE);
	set_idt_entry(&entries[135], isr135, flags0, GDT_KCODE);
	set_idt_entry(&entries[136], isr136, flags0, GDT_KCODE);
	set_idt_entry(&entries[137], isr137, flags0, GDT_KCODE);
	set_idt_entry(&entries[138], isr138, flags0, GDT_KCODE);
	set_idt_entry(&entries[139], isr139, flags0, GDT_KCODE);
	set_idt_entry(&entries[140], isr140, flags0, GDT_KCODE);
	set_idt_entry(&entries[141], isr141, flags0, GDT_KCODE);
	set_idt_entry(&entries[142], isr142, flags0, GDT_KCODE);
	set_idt_entry(&entries[143], isr143, flags0, GDT_KCODE);
	set_idt_entry(&entries[144], isr144, flags0, GDT_KCODE);
	set_idt_entry(&entries[145], isr145, flags0, GDT_KCODE);
	set_idt_entry(&entries[146], isr146, flags0, GDT_KCODE);
	set_idt_entry(&entries[147], isr147, flags0, GDT_KCODE);
	set_idt_entry(&entries[148], isr148, flags0, GDT_KCODE);
	set_idt_entry(&entries[149], isr149, flags0, GDT_KCODE);
	set_idt_entry(&entries[150], isr150, flags0, GDT_KCODE);
	set_idt_entry(&entries[151], isr151, flags0, GDT_KCODE);
	set_idt_entry(&entries[152], isr152, flags0, GDT_KCODE);
	set_idt_entry(&entries[153], isr153, flags0, GDT_KCODE);
	set_idt_entry(&entries[154], isr154, flags0, GDT_KCODE);
	set_idt_entry(&entries[155], isr155, flags0, GDT_KCODE);
	set_idt_entry(&entries[156], isr156, flags0, GDT_KCODE);
	set_idt_entry(&entries[157], isr157, flags0, GDT_KCODE);
	set_idt_entry(&entries[158], isr158, flags0, GDT_KCODE);
	set_idt_entry(&entries[159], isr159, flags0, GDT_KCODE);
	set_idt_entry(&entries[160], isr160, flags0, GDT_KCODE);
	set_idt_entry(&entries[161], isr161, flags0, GDT_KCODE);
	set_idt_entry(&entries[162], isr162, flags0, GDT_KCODE);
	set_idt_entry(&entries[163], isr163, flags0, GDT_KCODE);
	set_idt_entry(&entries[164], isr164, flags0, GDT_KCODE);
	set_idt_entry(&entries[165], isr165, flags0, GDT_KCODE);
	set_idt_entry(&entries[166], isr166, flags0, GDT_KCODE);
	set_idt_entry(&entries[167], isr167, flags0, GDT_KCODE);
	set_idt_entry(&entries[168], isr168, flags0, GDT_KCODE);
	set_idt_entry(&entries[169], isr169, flags0, GDT_KCODE);
	set_idt_entry(&entries[170], isr170, flags0, GDT_KCODE);
	set_idt_entry(&entries[171], isr171, flags0, GDT_KCODE);
	set_idt_entry(&entries[172], isr172, flags0, GDT_KCODE);
	set_idt_entry(&entries[173], isr173, flags0, GDT_KCODE);
	set_idt_entry(&entries[174], isr174, flags0, GDT_KCODE);
	set_idt_entry(&entries[175], isr175, flags0, GDT_KCODE);
	set_idt_entry(&entries[176], isr176, flags0, GDT_KCODE);
	set_idt_entry(&entries[177], isr177, flags0, GDT_KCODE);
	set_idt_entry(&entries[178], isr178, flags0, GDT_KCODE);
	set_idt_entry(&entries[179], isr179, flags0, GDT_KCODE);
	set_idt_entry(&entries[180], isr180, flags0, GDT_KCODE);
	set_idt_entry(&entries[181], isr181, flags0, GDT_KCODE);
	set_idt_entry(&entries[182], isr182, flags0, GDT_KCODE);
	set_idt_entry(&entries[183], isr183, flags0, GDT_KCODE);
	set_idt_entry(&entries[184], isr184, flags0, GDT_KCODE);
	set_idt_entry(&entries[185], isr185, flags0, GDT_KCODE);
	set_idt_entry(&entries[186], isr186, flags0, GDT_KCODE);
	set_idt_entry(&entries[187], isr187, flags0, GDT_KCODE);
	set_idt_entry(&entries[188], isr188, flags0, GDT_KCODE);
	set_idt_entry(&entries[189], isr189, flags0, GDT_KCODE);
	set_idt_entry(&entries[190], isr190, flags0, GDT_KCODE);
	set_idt_entry(&entries[191], isr191, flags0, GDT_KCODE);
	set_idt_entry(&entries[192], isr192, flags0, GDT_KCODE);
	set_idt_entry(&entries[193], isr193, flags0, GDT_KCODE);
	set_idt_entry(&entries[194], isr194, flags0, GDT_KCODE);
	set_idt_entry(&entries[195], isr195, flags0, GDT_KCODE);
	set_idt_entry(&entries[196], isr196, flags0, GDT_KCODE);
	set_idt_entry(&entries[197], isr197, flags0, GDT_KCODE);
	set_idt_entry(&entries[198], isr198, flags0, GDT_KCODE);
	set_idt_entry(&entries[199], isr199, flags0, GDT_KCODE);
	set_idt_entry(&entries[200], isr200, flags0, GDT_KCODE);
	set_idt_entry(&entries[201], isr201, flags0, GDT_KCODE);
	set_idt_entry(&entries[202], isr202, flags0, GDT_KCODE);
	set_idt_entry(&entries[203], isr203, flags0, GDT_KCODE);
	set_idt_entry(&entries[204], isr204, flags0, GDT_KCODE);
	set_idt_entry(&entries[205], isr205, flags0, GDT_KCODE);
	set_idt_entry(&entries[206], isr206, flags0, GDT_KCODE);
	set_idt_entry(&entries[207], isr207, flags0, GDT_KCODE);
	set_idt_entry(&entries[208], isr208, flags0, GDT_KCODE);
	set_idt_entry(&entries[209], isr209, flags0, GDT_KCODE);
	set_idt_entry(&entries[210], isr210, flags0, GDT_KCODE);
	set_idt_entry(&entries[211], isr211, flags0, GDT_KCODE);
	set_idt_entry(&entries[212], isr212, flags0, GDT_KCODE);
	set_idt_entry(&entries[213], isr213, flags0, GDT_KCODE);
	set_idt_entry(&entries[214], isr214, flags0, GDT_KCODE);
	set_idt_entry(&entries[215], isr215, flags0, GDT_KCODE);
	set_idt_entry(&entries[216], isr216, flags0, GDT_KCODE);
	set_idt_entry(&entries[217], isr217, flags0, GDT_KCODE);
	set_idt_entry(&entries[218], isr218, flags0, GDT_KCODE);
	set_idt_entry(&entries[219], isr219, flags0, GDT_KCODE);
	set_idt_entry(&entries[220], isr220, flags0, GDT_KCODE);
	set_idt_entry(&entries[221], isr221, flags0, GDT_KCODE);
	set_idt_entry(&entries[222], isr222, flags0, GDT_KCODE);
	set_idt_entry(&entries[223], isr223, flags0, GDT_KCODE);
	set_idt_entry(&entries[224], isr224, flags0, GDT_KCODE);
	set_idt_entry(&entries[225], isr225, flags0, GDT_KCODE);
	set_idt_entry(&entries[226], isr226, flags0, GDT_KCODE);
	set_idt_entry(&entries[227], isr227, flags0, GDT_KCODE);
	set_idt_entry(&entries[228], isr228, flags0, GDT_KCODE);
	set_idt_entry(&entries[229], isr229, flags0, GDT_KCODE);
	set_idt_entry(&entries[230], isr230, flags0, GDT_KCODE);
	set_idt_entry(&entries[231], isr231, flags0, GDT_KCODE);
	set_idt_entry(&entries[232], isr232, flags0, GDT_KCODE);
	set_idt_entry(&entries[233], isr233, flags0, GDT_KCODE);
	set_idt_entry(&entries[234], isr234, flags0, GDT_KCODE);
	set_idt_entry(&entries[235], isr235, flags0, GDT_KCODE);
	set_idt_entry(&entries[236], isr236, flags0, GDT_KCODE);
	set_idt_entry(&entries[237], isr237, flags0, GDT_KCODE);
	set_idt_entry(&entries[238], isr238, flags0, GDT_KCODE);
	set_idt_entry(&entries[239], isr239, flags0, GDT_KCODE);
	set_idt_entry(&entries[240], isr240, flags0, GDT_KCODE);
	set_idt_entry(&entries[241], isr241, flags0, GDT_KCODE);
	set_idt_entry(&entries[242], isr242, flags0, GDT_KCODE);
	set_idt_entry(&entries[243], isr243, flags0, GDT_KCODE);
	set_idt_entry(&entries[244], isr244, flags0, GDT_KCODE);
	set_idt_entry(&entries[245], isr245, flags0, GDT_KCODE);
	set_idt_entry(&entries[246], isr246, flags0, GDT_KCODE);
	set_idt_entry(&entries[247], isr247, flags0, GDT_KCODE);
	set_idt_entry(&entries[248], isr248, flags0, GDT_KCODE);
	set_idt_entry(&entries[249], isr249, flags0, GDT_KCODE);
	set_idt_entry(&entries[250], isr250, flags0, GDT_KCODE);
	set_idt_entry(&entries[251], isr251, flags0, GDT_KCODE);
	set_idt_entry(&entries[252], isr252, flags0, GDT_KCODE);
	set_idt_entry(&entries[253], isr253, flags0, GDT_KCODE);
	set_idt_entry(&entries[254], isr254, flags0, GDT_KCODE);
	set_idt_entry(&entries[255], isr255, flags0, GDT_KCODE);

	load_idt(&idtr);
}

/* Set up the interrupt handlers per CPU. */
void idt_init_mp(void) {
	idt_init();
}

void int_dispatch(struct int_frame *frame)
{
	/* Handle processor exceptions:
	 *  - Fall through to the kernel monitor on a breakpoint.
	 *  - Dispatch page faults to page_fault_handler().
	 *  - Dispatch system calls to syscall().
	 */
	/* LAB 3: your code here. */
	switch (frame->int_no) {
		case INT_SYSCALL:
			frame->rax = syscall(frame->rdi, frame->rsi, frame->rdx, frame->rcx, frame->r8, frame->r9, frame->rbp);
			return;
		case INT_PAGE_FAULT:
			page_fault_handler(frame);
			return;
		case INT_BREAK:
			while(1) monitor(NULL);
		case IRQ_TIMER:
//			cprintf("timer at %u\n", read_tsc());
			lapic_eoi();
			sched_yield();
	default: break;
	}

	/* Unexpected trap: The user process or the kernel has a bug. */
	print_int_frame(frame);

	if (frame->cs == GDT_KCODE) {
		panic("unhandled interrupt in kernel");
	} else {
		task_destroy(cur_task);
		return;
	}
}

/* isr$N jumps to isr_common_stub which jumps to */
void int_handler(struct int_frame *frame)
{
	/* The task may have set DF and some versions of GCC rely on DF being
	 * clear. */
	asm volatile("cld" ::: "cc");

	/* Check if interrupts are disabled.
	 * If this assertion fails, DO NOT be tempted to fix it by inserting a
	 * "cli" in the interrupt path.
	 */
	assert(!(read_rflags() & FLAGS_IF));

//	cprintf("Incoming INT frame at %p\n", frame);

	if ((frame->cs & 3) == 3) {
		/* Interrupt from user mode. */
		assert(cur_task);

		/* Copy interrupt frame (which is currently on the stack) into
		 * 'cur_task->task_frame', so that running the task will restart at
		 * the point of interrupt. */
		cur_task->task_frame = *frame;

		/* Avoid using the frame on the stack. */
		frame = &cur_task->task_frame;
	}

	/* Dispatch based on the type of interrupt that occurred. */
	int_dispatch(frame);

	/* Return to the current task, which should be running. */
	task_run(cur_task);
}

void page_fault_handler(struct int_frame *frame)
{
	void *fault_va;
	unsigned perm = 0;
	int ret;

	/* Read the CR2 register to find the faulting address. */
	fault_va = (void *)read_cr2();

	/* Handle kernel-mode page faults. */
	/* LAB 3: your code here. */
    if ((frame->cs & 3) == 0) {
        cprintf("[PID %5u] user fault va %p ip %p cpuid %u\n",
                cur_task->task_pid, fault_va, frame->rip, this_cpu->cpu_id);
        print_int_frame(frame);
        // FIXME what if a kernel page has not been mapped yet?
//        Leave it like this for now to see when it crashes and fit it then
        panic("Kernel-mode page fault");
    }


	/* We have already handled kernel-mode exceptions, so if we get here, the
	 * page fault has happened in user mode.
	 */

	ret = task_page_fault_handler(cur_task, fault_va, frame->rflags);
	if (ret == 0) {
		task_run(cur_task);
	}

	/* Destroy the task that caused the fault. */
	cprintf("[PID %5u] user fault va %p ip %p\n",
		cur_task->task_pid, fault_va, frame->rip);
	print_int_frame(frame);
	task_destroy(cur_task);
}

