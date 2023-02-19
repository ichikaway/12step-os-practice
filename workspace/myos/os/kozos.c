#include "defines.h"
#include "kozos.h"
#include "intr.h"
#include "interrupt.h"
#include "syscall.h"
#include "memory.h"
#include "lib.h"

#define THREAD_NUM 6
#define THREAD_NAME_SIZE 15
#define PRIORITY_NUM 16


typedef struct _kz_context {
    uint32 sp;
} kz_context;

typedef struct _kz_thread {
    struct _kz_thread *next;
    char name[THREAD_NAME_SIZE + 1];
    int priority;
    char *stack;
    uint32 flags;
#define KZ_THREAD_FLAG_READY (1<<0)

    /* スレッドのスタートアップ thread_init() に渡すパラメータ */
    struct {
        kz_func_t func; /* スレッドのメイン関数 */
        int argc;
        char **argv;
    } init;

    struct {
        kz_syscall_type_t type;
        kz_syscall_param_t *param;
    } syscall;

    kz_context context;
} kz_thread;

/* スレッドのレディーキュー */
static struct {
    kz_thread *head;
    kz_thread *tail;
} readyque[PRIORITY_NUM];

//グローバル変数
static kz_thread *current;
static kz_thread threads[THREAD_NUM]; /* TCB */
static kz_handler_t handlers[SOFTVEC_TYPE_NUM]; /* 割り込みハンドラー */

void dispatch(kz_context *context); /* startup.sにあるディスパッチ関数 */


void printDebug(char *text)
{
    return;
    puts(text);
    puts("; ");
    puts("Current: ");
    puts(current->name);
    puts("; ");

    puts("ReadyQue: ");
    int i;
    for (i = 0; i < PRIORITY_NUM; i++) {
        if (readyque[i].head == NULL) {
            continue;
        }
        kz_thread *now = readyque[i].head;
        while(now != NULL) {
            puts(now->name);
            puts(":");
            putxval(i, 1);
            puts(", ");
            now = now->next;
        }
    }

    puts("\n");
    return;
}

static int getcurrent(void)
{
    if (current == NULL) {
        return -1;
    }

    //ready flagが立ってなければ何もしない
    if ( !(current->flags & KZ_THREAD_FLAG_READY)) {
        return 1;
    }

    readyque[current->priority].head = current->next;
    if (readyque[current->priority].head == NULL) {
        readyque[current->priority].tail = NULL;
    }

    //ready flagを落とす
    current->flags &= ~KZ_THREAD_FLAG_READY;

    current->next = NULL;
    return 0;
}

static int putcurrent(void)
{
    if (current == NULL) {
        return -1;
    }

    // ready flagが立っている状態なら何もしない
    if ( current->flags & KZ_THREAD_FLAG_READY) {
        return 1;
    }

    if (readyque[current->priority].tail) {
        readyque[current->priority].tail->next = current;
    } else {
        readyque[current->priority].head = current;
    }
    readyque[current->priority].tail = current;

    //ready flagを立てる
    current->flags |= KZ_THREAD_FLAG_READY;

    return 0;
}

static void thread_end(void)
{
    kz_exit();
}

static void thread_init(kz_thread *thp)
{
    thp->init.func(thp->init.argc, thp->init.argv);
    thread_end();
}

static kz_thread_id_t thread_run(kz_func_t func, char *name, int priority, int stacksize, int argc, char *argv[])
{
    int i;
    kz_thread *thp;
    uint32 *sp;

    // リンカで定義したuserstackのスタック領域
    extern char userstack;
    // global変数 thread_stackでuserstackのメモリ領域を管理。グローバル変数のためspのメモリが進む場合は常にその状態が保持される
    static char *thread_stack = &userstack;

    for (i = 0; i < THREAD_NUM; i++) {
        thp = &threads[i]; //threadsはグローバル変数の配列(kz_thread型の配列)
        // init.funcが空の場合。空であればここにTCBが登録可能
        if (!thp->init.func) {
            break;
        }
    }
    //空きがない場合
    if (i == THREAD_NUM) {
        return -1;
    }

    memset(thp, 0, sizeof(*thp));

    //タスクコントロールブロックの設定
    strcpy(thp->name, name);
    thp->next = NULL;
    thp->priority = priority;
    thp->flags = 0;
    thp->init.func = func;
    thp->init.argc = argc;
    thp->init.argv = argv;

    //スタック領域を獲得
    memset(thread_stack, 0, stacksize);
    thread_stack += stacksize; //グローバル変数 thead_stackでスタックサイズ分増やすためSPはstacksizeだけ進む
    thp->stack = thread_stack;

    //スタックの初期化
    sp = (uint32 *)thp->stack;
    *(--sp) = (uint32)thread_end; //スタックにthread_init()からの戻り先としてthread_end()を設定する

    /*
     * プログラムカウンタを設定する
     * スレッドの優先度が0の場合には割り込み禁止スレッドとする
     */
    *(--sp) = (uint32)thread_init | ((uint32)(priority == 0 ? 0xc0 : 0) << 24);//優先度が0の場合には割り込み禁止で起動する
    *(--sp) = 0; //ER6
    *(--sp) = 0; //ER5
    *(--sp) = 0; //ER4
    *(--sp) = 0; //ER3
    *(--sp) = 0; //ER2
    *(--sp) = 0; //ER1

    *(--sp) = (uint32)thp; // ER0 第一引数

    thp->context.sp = (uint32)sp;

    //システムコールを呼び出したスレッドをreadyqueに戻す
    putcurrent();

    //新規作成したスレッドをreadyqueに接続
    current = thp;
    putcurrent();

    return (kz_thread_id_t)current;
}

static int thread_exit(void)
{
    puts(current->name);
    puts(" EXIT. \n");
    memset(current, 0, sizeof(*current));
    return 0;
}

static int thread_wait(void)
{
    //ready queからいったん外して接続し直すことでラウンドロビンで他のスレッドを動作させる
    putcurrent();
    return 0;
}

static int thread_sleep(void)
{
    //ready queから外されたままになるためスケジューリングされなくなる
    return 0;
}

static int thread_wakeup(kz_thread_id_t id)
{
    //wakeupを呼び出したスレッドをready queに戻す
    putcurrent();

    //指定されたスレッドをready queに接続させてwake upする
    current = (kz_thread *)id;
    putcurrent(); //ready queに戻す
    return 0;
}

static kz_thread_id_t thread_getid(void)
{
    putcurrent();
    return (kz_thread_id_t)current; //TCBのアドレスがスレッドIDとなる
}

static int thread_chpri(int priority)
{
    int old = current->priority;
    if (priority >= 0) {
        current->priority = priority;
    }
    putcurrent(); //新しい優先度のready queに入れる
    return old;
}

static void *thread_kmalloc(int size)
{
    putcurrent();
    return kzmem_alloc(size);
}

static int thread_kmfree(char *mem)
{
    kzmem_free(mem);
    putcurrent();
    return 0;
}

static void thread_intr(softvec_type_t type, unsigned long sp);
static int setintr(softvec_type_t type, kz_handler_t handler)
{
    softvec_setintr(type, thread_intr);

    handlers[type] = handler;
    return 0;
}

static void call_functions(kz_syscall_type_t type, kz_syscall_param_t *p)
{
    switch (type) {
        case KZ_SYSCALL_TYPE_RUN: // kz_run()
            p->un.run.ret = thread_run(
                    p->un.run.func,
                    p->un.run.name,
                    p->un.run.priority,
                    p->un.run.stacksize,
                    p->un.run.argc,
                    p->un.run.argv
                    );
            break;
        case KZ_SYSCALL_TYPE_EXIT: //kz_exit()
            thread_exit();
            break;
        case KZ_SYSCALL_TYPE_WAIT:
            p->un.wait.ret = thread_wait();
            break;
        case KZ_SYSCALL_TYPE_SLEEP:
            p->un.sleep.ret = thread_sleep();
            break;
        case KZ_SYSCALL_TYPE_WAKEUP:
            p->un.wakeup.ret = thread_wakeup(p->un.wakeup.id);
            break;
        case KZ_SYSCALL_TYPE_GETID:
            p->un.getid.ret = thread_getid();
            break;
        case KZ_SYSCALL_TYPE_CHPRI:
            p->un.chpri.ret = thread_chpri(p->un.chpri.priority);
            break;
        case KZ_SYSCALL_TYPE_KMALLOC:
            p->un.kmalloc.ret = thread_kmalloc(p->un.kmalloc.size);
            break;
        case KZ_SYSCALL_TYPE_KMFREE:
            p->un.kmfree.ret = thread_kmfree(p->un.kmfree.mem);
            break;
        default:
            break;
    }
}

static void syscall_proc(kz_syscall_type_t type, kz_syscall_param_t *p)
{
    printDebug("syscall_proc-1");
    getcurrent();
    printDebug("syscall_proc-2");
    call_functions(type, p);
}

static void schedule(void)
{
    int i;
    for (i = 0; i < PRIORITY_NUM; i++) {
        if (readyque[i].head) {
            break;
        }
    }

    //次に実行すべきスレッドが見つからなかった
    if (i == PRIORITY_NUM) {
        kz_sysdown();
    }

    current = readyque[i].head;
}


//イベントハンドラ。システムコールの呼び出し
static void syscall_intr(void)
{
    syscall_proc(current->syscall.type, current->syscall.param);
}

//イベントハンドラ
static void softerr_intr(void)
{
    puts(current->name);
    puts(" Down.\n");
    getcurrent();
    thread_exit();
}

static void thread_intr(softvec_type_t type, unsigned long sp)
{
    //printDebug("thread_intr()");

    // 割り込み発生時に動いてるスレッドのspをそのスレッドのTCBのcontext.spに保存
    current->context.sp = sp;

    /*
     * 割込みごとの処理を実行する。
     * SOFTVEC_TYPE_SYSCALL, SOFTVEC_TYPE_SOFTERR の場合は
     * syscall_intr(), softerr_intr() がハンドラに登録されているので
     * それらが実行される。
     */
    if (handlers[type]) {
        handlers[type]();
    }
    schedule();
    dispatch(&current->context); //startup.sの_dispatchが実行される
    /* ここには返ってこない */
}

//初期スレッドを起動しOSの動作を開始する
void kz_start(kz_func_t func, char *name, int priority, int stacksize, int argc, char *argv[])
{
    kzmem_init();

    current = NULL;

    memset(readyque, 0, sizeof(readyque));
    memset(threads, 0, sizeof(threads)); //グローバル変数配列を初期化
    memset(handlers, 0, sizeof(handlers));

    setintr(SOFTVEC_TYPE_SYSCALL, syscall_intr);
    setintr(SOFTVEC_TYPE_SOFTERR, softerr_intr);

    current = (kz_thread *) thread_run(func, name, priority, stacksize, argc, argv);

    // thread_runの中でcontextのspがセットされている
    dispatch(&current->context);
    //ここには返ってこない

}

void kz_sysdown(void)
{
    puts("system error!\n");
    while(1);
}

void kz_syscall(kz_syscall_type_t type, kz_syscall_param_t *param)
{
    current->syscall.type = type; //systemcall番号
    current->syscall.param = param;
    asm volatile("trapa #0"); //trap call
}
