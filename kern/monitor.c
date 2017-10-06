// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>
#include <inc/mmu.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>
#include <kern/pmap.h>

#define CMDBUF_SIZE 80 // enough for one VGA text line

uint32_t xtoi(char* input);

struct Command {
  const char *name;
  const char *desc;
  // return -1 to force monitor to exit
  int (*func)(int argc, char **argv, struct Trapframe * tf);
};

static struct Command commands[] = {
  { "help",      "Display this list of commands",        mon_help       },
  { "info-kern", "Display information about the kernel", mon_infokern   },
  { "backtrace", "Backtrace and display stack information", mon_backtrace   },
  { "showmappings", "Show memory mappings", mon_showmappings   },
  { "chmperm", "Change permission of a memory mapping", mon_chmperm   },
  { "lsmem", "List memory content", mon_lsmem   },
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
mon_infokern(int argc, char **argv, struct Trapframe *tf)
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


int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
  uint32_t * ebp = (uint32_t *) read_ebp();
  cprintf("Stack backtrace:\n");

  while (ebp) {
    cprintf("ebp %08x  ", ebp);
    cprintf("eip %08x  ", ebp[1]);
    cprintf("args %08x ", *(ebp + 2));
    cprintf("%08x ", *(ebp + 3));
    cprintf("%08x ", *(ebp + 4));
    cprintf("%08x ", *(ebp + 5));
    cprintf("%08x \n", *(ebp + 6));

    struct Eipdebuginfo info;
    debuginfo_eip(ebp[1], &info);
    // Note: eip_fn_name is not null terminated
    cprintf("\t%s:%d: %.*s+%d\n",
            info.eip_file, info.eip_line,
            info.eip_fn_namelen, info.eip_fn_name, info.eip_fn_addr);

    ebp = (uint32_t *) *ebp; // restore previous ebp
  }
  return 0;
}

uint32_t xtoi(char* input) {
    uint32_t result = 0;
    input += 2;
    while (*input) { 
        // handle a-f
        if (*input >= 'a') {
            *input = '0' + *input - 'a' + 10;
        }
        result = result * 16 + *input - '0';
        input++;
    }
    return result;
}

long int cstrtol(const char *nptr, char **endptr, int base) {
    long int result = 0;
    while (*nptr) { 
        result = result * base + *nptr - '0';
        nptr++;
    }
    return result;
}

int
mon_showmappings(int argc, char **argv, struct Trapframe *tf)
{
    if (argc != 3) {
        cprintf("Usage: showmapping 0xstart_addr 0xend_address\n"); 
        return 0;
    }
    uint32_t start = xtoi(argv[1]);
    uint32_t end = xtoi(argv[2]);
    for (; start <= end; start += PGSIZE) {
        pte_t *pte = pgdir_walk(kern_pgdir, (void *) start, 0);
        if (pte && (*pte & PTE_P)) {
            cprintf("virtual address 0x%x maps to physical address 0x%x, ", start, PTE_ADDR(*pte));
            cprintf("permissions: PTE_U: %d, PTE_W: %d\n", (*pte & PTE_U) != 0, (*pte & PTE_W) != 0);
        } else {
            cprintf("virtual address 0x%x not mapped\n", start);
        }
    }
    return 0;
}

int
mon_chmperm(int argc, char **argv, struct Trapframe *tf)
{
    if (argc != 4) {
        cprintf("Usage: chmperm 0xaddr U|W 0|1\n");
        return 0;
    }
    uint32_t va = xtoi(argv[1]);
    pte_t *pte = pgdir_walk(kern_pgdir, (void *) va, 0);
    if (pte && (*pte & PTE_P)) {
        cprintf("before chmperm: virtual address 0x%x maps to physical address 0x%x, ", va, PTE_ADDR(*pte));
        cprintf("permissions: PTE_U: %d, PTE_W: %d\n", (*pte & PTE_U) != 0, (*pte & PTE_W) != 0);
    } else {
        cprintf("virtual address 0x%x not mapped\n", va);
        return 0;
    }
    uint32_t new_perm = 0;
    if (argv[2][0] == 'U') {
        new_perm = PTE_U;
    } else if (argv[2][0] == 'W') {
        new_perm = PTE_W;
    } else {
        cprintf("Usage: chmperm 0xaddr U|W 0|1\n");
        return 0;
    }

    if (argv[3][0] == '0') {
        *pte = *pte & ~new_perm;
    } else if (argv[3][0] == '1') {
        *pte = *pte | new_perm;
    } else {
        cprintf("Usage: chmperm 0xaddr U|W 0|1\n");
        return 0;
    }
    cprintf("after chmperm: virtual address 0x%x maps to physical address 0x%x, ", va, PTE_ADDR(*pte));
    cprintf("permissions: PTE_U: %d, PTE_W: %d\n", (*pte & PTE_U) != 0, (*pte & PTE_W) != 0);
    return 0;
}

int
mon_lsmem(int argc, char **argv, struct Trapframe *tf)
{
    if (argc != 4) {
        cprintf("Usage: lsmem v|p 0xaddr count\n");
        return 0;
    }
    int input_addr = xtoi(argv[2]);
    int count = cstrtol(argv[3], NULL, 10);
    char *va = NULL;
    if (argv[1][0] == 'v') {
        va = (char *) input_addr;
        int i;
        for(i = 0; i < count; i++) {
            cprintf("virtual address 0x%x ", input_addr + i);
            cprintf("has content: 0x%02x\n", 0x000000FF & va[i]);
        }
    } else if (argv[1][0] == 'p') {
        va = KADDR(input_addr);
        int i;
        for(i = 0; i < count; i++) {
            cprintf("physical address 0x%x ", input_addr + i);
            cprintf("has content: 0x%02x\n", 0x000000FF & va[i]);
        }
    } else {
        cprintf("Usage: lsmem v|p 0xaddr count\n");
        return 0;
    }
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
  for (i = 0; i < NCOMMANDS; i++)
    if (strcmp(argv[0], commands[i].name) == 0)
      return commands[i].func(argc, argv, tf);
  cprintf("Unknown command '%s'\n", argv[0]);
  return 0;
}

void
monitor(struct Trapframe *tf)
{
  char *buf;

  cprintf("Welcome to the JOS kernel monitor!\n");
  cprintf("Type 'help' for a list of commands.\n");
  cprintf("%qEnjoy %qthis %qcolorful %qsystem!\n", 0x0100, 0x0200, 0x0300, 0x0400);

  if (tf != NULL)
    print_trapframe(tf);

  while (1) {
    buf = readline("K> ");
    if (buf != NULL)
      if (runcmd(buf, tf) < 0)
        break;
  }
}
