#ifndef _KOZO_MEMORY_H_INCLUDED_
#define _KOZO_MEMORY_H_INCLUDED_

int kzmem_init(void);
void *kzmem_alloc(int size);
void kzmem_free(void *mem);

#endif
