#include "defines.h"
#include "kozos.h"
#include "lib.h"
#include "memory.h"


typedef struct _kzmem_block {
    struct _kzmem_block *next;
    int size;
} kzmem_block;

typedef struct _kzmem_pool {
    int size;
    int num;
    kzmem_block *free;
} kzmem_pool;

static kzmem_pool pool[] = {
        {16, 8, NULL},
        {32, 8, NULL},
        {64, 4, NULL},
};

#define MEMORY_AREA_NUM (sizeof(pool) / sizeof(*pool))

static int kzmem_init_pool(kzmem_pool *poolp)
{
    int i;
    kzmem_block *mp;
    kzmem_block **mpp;
    extern char freearea; //リンカで定義したfreearea
    static char *area = &freearea;

    mp = (kzmem_block *)area;

    //個々の領域をすべて解放済みリンクリストにつなぐ
    mpp = &poolp->free; //poolpはkzmem_poolの配列の一つの構造体. まずはfreeの最初の要素kzmem_blockを先頭にいれるために先頭のfreeのアドレスをmppにセット

    for (i = 0; i < poolp->num; i++) {
        *mpp = mp; //kzmem_pool.freeもしくはkzmem_block.nextのアドレスにmp(kzmem_block)を入れてリンクをつなぐ
        memset(mp, 0 , sizeof(*mp));
        mp->size = poolp->size;
        mpp = &(mp->next); //mppに次のmpを入れる場所のアドレスを渡す
        mp = (kzmem_block *)((char *)mp + poolp->size); //mpのアドレスにメモリ領域のサイズを足して次のkzmem_blockのアドレスを割り出す
        area += poolp->size; //グローバル変数areaのアドレスをサイズ分進める。これで別のサイズのブロックを作る際には、areaで進んだところからのアドレスが利用される
    }
    return 0;
}

int kzmem_init(void)
{
    int i;
    for (i = 0; i < MEMORY_AREA_NUM; i++) {
        kzmem_init_pool(&pool[i]);
    }
    return 0;
}

void *kzmem_alloc(int size)
{
    int i;
    kzmem_block *mp;
    kzmem_pool *poolp;

    for (i = 0; i < MEMORY_AREA_NUM; i++) {
        poolp = &pool[i];
        if (size <= (poolp->size - sizeof(kzmem_block))) {
            if (poolp->free == NULL) {
                kz_sysdown();
                return NULL;
            }

            mp = poolp->free;
            poolp->free = poolp->free->next;
            mp->next = NULL;

            //実際にメモリとして利用可能な領域はメモリブロック構造体の直後のアドレスから。
            //そのため +1して構造体分のサイズを足したアドレスを返す
            return mp+1;
        }
    }

    kz_sysdown();
    return NULL;
}

void kzmem_free(void *mem)
{
    int i;
    kzmem_block *mp;
    kzmem_pool *poolp;

    // メモリ利用可能領域の直前にメモリブロック構造体がある
    mp = ((kzmem_block *)mem - 1);

    for (i = 0; i < MEMORY_AREA_NUM; i++) {
        poolp = &pool[i];
        if (mp->size == poolp->size) {
            mp->next = poolp->free;
            poolp->free = mp;
            return;
        }
    }

    kz_sysdown();
}