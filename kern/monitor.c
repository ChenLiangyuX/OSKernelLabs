// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
        { "backtrace", "Display backtrace of stack", mon_backtrace },
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < NCOMMANDS; i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

/*
 * output format:
 *Stack backtrace:
 *  ebp f0109e58  eip f0100a62  args 00000001 f0109e80 f0109e98 f0100ed2 00000031
 *  ebp f0109ed8  eip f01000d6  args 00000000 00000000 f0100058 f0109f28 00000061
 *...
 Stack backtrace:
   ebp f010ff78  eip f01008ae  args 00000001 f010ff8c 00000000 f0110580 00000000
            kern/monitor.c:143: monitor+106
   ebp f010ffd8  eip f0100193  args 00000000 00001aac 00000660 00000000 00000000
            kern/init.c:49: i386_init+59
   ebp f010fff8  eip f010003d  args 00000000 00000000 0000ffff 10cf9a00 0000ffff
            kern/entry.S:70: <unknown>+0
 */
int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// Your code here.
        cprintf("Stack backtrace:\n");
        uint32_t *ebp = (uint32_t *)read_ebp();
        while (ebp) {
           cprintf("  ebp %08x  eip %08x  args", (uint32_t) ebp, (uint32_t) ebp[1]);
           int offset;
           for (offset = 2; offset < 2 + 5; offset++) {
              cprintf(" %08x", (uint32_t)ebp[offset]);
           }
           cprintf("\n");

           struct Eipdebuginfo info;
           debuginfo_eip((uintptr_t)ebp[1], &info);
           int delta = (uint32_t) ebp[1] - info.eip_fn_addr;
           cprintf("            %s:%d: %.*s+%d\n",
                 info.eip_file,
                 info.eip_line,
                 info.eip_fn_namelen,
                 info.eip_fn_name,
                 delta
           );
           /*
           cprintf("eip_file = %s, eip_line = %d, eip_fn_name = %s, eip_fn_namelen = %d, eip_fn_addr = %08x, eip_fn_narg = %d\n",
                 info.eip_file,
                 info.eip_line,
                 info.eip_fn_name,
                 info.eip_fn_namelen,
                 info.eip_fn_addr,
                 info.eip_fn_narg
                 );
                 */


           ebp = (uint32_t *)ebp[0];
        }
        return 0;

        /*

         //a little ugly

        cprintf("Stack backtrace:\n");
        uint32_t ebp = read_ebp();
        while (ebp) {
           uint32_t eip =  *(uint32_t *) (ebp + 4);

           cprintf("  ebp %08x  eip %08x  args", ebp, eip);
           uint32_t arg_st = ebp + 8, arg_ed = ebp + 8 + 4 * 5;
           uint32_t arg_p;
           for (arg_p = arg_st; arg_p < arg_ed; arg_p += 4) {
              cprintf(" %08x", *(uint32_t *)arg_p);
           }
           cprintf("\n");
           ebp = *(uint32_t *)ebp;
        }
        */

        return 0;
}



/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
