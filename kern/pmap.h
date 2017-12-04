/* See COPYRIGHT for copyright information. */

#ifndef JOS_KERN_PMAP_H
#define JOS_KERN_PMAP_H
#ifndef JOS_KERNEL
# error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/memlayout.h>
#include <inc/assert.h>
struct Env;

extern char bootstacktop[], bootstack[];

extern struct PageInfo *pages;
extern struct PageInfo *super_pages;
extern size_t npages;
extern size_t nsuper_pages;

extern pde_t *kern_pgdir;


/* This macro takes a kernel virtual address -- an address that points above
 * KERNBASE, where the machine's maximum 256MB of physical memory is mapped --
 * and returns the corresponding physical address.  It panics if you pass it a
 * non-kernel virtual address.
 */
#define PADDR(kva) _paddr(__FILE__, __LINE__, kva)

static inline physaddr_t
_paddr(const char *file, int line, void *kva)
{
	if ((uint32_t)kva < KERNBASE)
		_panic(file, line, "PADDR called with invalid kva %08lx", kva);
	return (physaddr_t)kva - KERNBASE;
}

/* This macro takes a physical address and returns the corresponding kernel
 * virtual address.  It panics if you pass an invalid physical address. */
#define KADDR(pa) _kaddr(__FILE__, __LINE__, pa)

static inline void*
_kaddr(const char *file, int line, physaddr_t pa)
{
	if (PGNUM(pa) >= (npages + nsuper_pages * (PTSIZE / PGSIZE)))
		_panic(file, line, "KADDR called with invalid pa %08lx", pa);
	return (void *)(pa + KERNBASE);
}


enum {
	// For page_alloc, zero the returned physical page.
	ALLOC_ZERO = 1<<0,
};

void	mem_init(void);

void	page_init(void);
struct PageInfo *page_alloc(int alloc_flags);
void	page_free(struct PageInfo *pp);
int	page_insert(pde_t *pgdir, struct PageInfo *pp, void *va, int perm);
void	page_remove(pde_t *pgdir, void *va);
struct PageInfo *page_lookup(pde_t *pgdir, void *va, pte_t **pte_store);
void	page_decref(struct PageInfo *pp);

struct PageInfo *super_page_alloc(int alloc_flags);
void	super_page_free(struct PageInfo *pp);
int	super_page_insert(pde_t *pgdir, struct PageInfo *pp, void *va, int perm);
void	super_page_remove(pde_t *pgdir, void *va);
struct PageInfo *super_page_lookup(pde_t *pgdir, void *va, pde_t **pde_store);
void	super_page_decref(struct PageInfo *pp);

void	tlb_invalidate(pde_t *pgdir, void *va);

void *	mmio_map_region(physaddr_t pa, size_t size);

int	user_mem_check(struct Env *env, const void *va, size_t len, int perm);
void	user_mem_assert(struct Env *env, const void *va, size_t len, int perm);

static inline physaddr_t
page2pa(struct PageInfo *pp)
{
	return (pp - pages) << PGSHIFT;
}

static inline physaddr_t
super_page2pa(struct PageInfo *pp)
{
	return ((pp - super_pages) << PTSHIFT) + (npages << PGSHIFT);
}

static inline struct PageInfo*
pa2page(physaddr_t pa)
{
	if (PGNUM(pa) >= npages)
		panic("pa2page called with invalid pa");
	return &pages[PGNUM(pa)];
}

static inline struct PageInfo*
pa2super_page(physaddr_t pa)
{
	if (PGNUM(pa) < npages) {
		panic("pa2super_page called with invalid pa (low)");
	}
	pa -= npages * PGSIZE;
	if (SPGNUM(pa) >= nsuper_pages) {
		panic("pa2super_page called with invalid pa (high)");
	}
	return &super_pages[SPGNUM(pa)];
}

static inline void*
page2kva(struct PageInfo *pp)
{
	return KADDR(page2pa(pp));
}

static inline void*
super_page2kva(struct PageInfo *pp)
{
	return KADDR(super_page2pa(pp));
}

pte_t *pgdir_walk(pde_t *pgdir, const void *va, int create);
pde_t *super_pgdir_walk(pde_t *pgdir, const void *va, int create);

#endif /* !JOS_KERN_PMAP_H */
