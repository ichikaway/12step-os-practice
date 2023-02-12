

Step8の大まかな流れがわかるシーケンス図がある。  
https://komamenamame.hatenablog.com/entry/2020/05/21/185800
https://komamenamame.hatenablog.com/entry/2020/06/06/224126
https://komamenamame.hatenablog.com/entry/2020/06/07/224225

PDF資料
23ページ目からコードの流れが書いてある  
https://www-tech.eng.kagoshima-u.ac.jp/?plugin=attach&refer=H25_01%E7%A0%94%E4%BF%AE&openfile=Kozos12step_08_20131218.pdf  
他のステップの資料  
https://www-tech.eng.kagoshima-u.ac.jp/?H25_01%E7%A0%94%E4%BF%AE


thread_run()では、TCBにスレッドを登録するのみ。スレッドで実行される関数ポインタや引数をセット。(thp->init.func)  
thread_stackにstacksize分の領域を確保。これをspとする。
spにレジスタに復帰させる値をセットしていき、thread_initの関数ポインタもセット。
これで別のタイミングで呼ばれるdispatch()の中でレジスタにスタックから復帰され、最後にthread_initからrteで実行される。  
dispatch(&current->context)でそのスレッドのスタックポインタを渡してdispatchするため、
dispatchの中でレジスタに復帰されるのはそのスレッドのcontext.spのスタックポインタのスタックの値から。


thread_init()では、実際にスレッドのメイン関数を実行。
それが終わるとthread_end()が呼ばれkz_exit()のsyscallが実行される。


thread_intr()が重要で、systemcallが発行された時のレジスタをスタックに退避、イベントハンドラ呼んでthread_intr()を呼び出し、退避したスタックのspをいま動いているスレッドに保存、別のスレッドを起動してディスパッチ（ここで別のスレッドのspを渡す）

kz_syscall()でsystemcallを呼び出す準備。動いているスレッドのsyscall.type, syscall.paramに値をセットして、
syscall_proc()からcall_function()で

## システムコール
システムコールはkz_runとkz_exitの2つのみ。スレッドの切り替えはこの２つのどちらかが呼ばれるタイミングのみとなる。
呼ばれると割り込みを発生させてタスクを切り返す。kz_runの場合はタスク切り替え前に新しいTCBを作ってreadyque.tailに入れる。

## 流れ

### mainからの大きな流れ


- main()
- kz_start() [kozo.c]  
  - 割り込みハンドラーセット  
  - thread_run() [kozo.c]  
     - start_threadsをTCBに登録  
     - readyqueはstartのみ  
  - dispatch() [start.s]  
     - start_threads()実行してtest08_1_mainを新しいTCBに登録
     - start_threadsのスレッドが終了してkz_exitシステムコールにより次のタスクtest08_1_mainが実行される


### start_threads()で最初のスレッドでこの関数を実行

- start_threads()
  - kz_run()システムコール発行 [syscall.c]  
      - test08_1_mainの関数ポインタを unionのsystem call用のパラメータ構造体に入れる  
      - kz_syscall()[kozo.c]  
        - 実行中のタスクのTCBのスレッドにsyscall.type, syscall.paramをセットしてシステムコールさせる（システムコールへの引数渡しでCPU依存をなくすための回避策）
        - systemcall trap発行して割り込み発生  

### 割り込み

- vector.cにあるsyscall用の割り込み intr_syscallが実行される [intr.S]
- interrupt() [interrupt.c] typeによって配列から必要なハンドラーを抜いてハンドラー実行
- thread_intr()[kozo.c]がソフト割り込みハンドラとして登録されているためthread_intr()が実行される。


```c
  static void thread_intr(softvec_type_t type, unsigned long sp)
{
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

```

- thread_intr()の中のhandler実行でsyscall_intr()[kozo.c] が実行される。詳細は次のパラグラフで。 
  - ハンドラによってシステムコールが発行されて新しいスレッドが登録もしくはexitされる。 
    - 今のreadyqueは [start, command]の順。 
  - schedule()[kozo.c]でreadyqueの最初にあるTCBを取り出して、dispatch()でspから停止前のレジスタを戻して関数実行 
    - thread_init[kozo.c]の関数ポインタがスタックの最後にあるのでレジスタ復帰後にthread_initが呼ばれる

### thread_init
```c
static void thread_init(kz_thread *thp)
{
    thp->init.func(thp->init.argc, thp->init.argv);
    thread_end();
}
```
thread_initによりstart_threads()[main.c]が途中で止まっていたのでそこから実行。  
それが終わるとthread_endが呼ばれてそこでkz_exit()システムコールが呼ばれる。  
次のシステムコールでcommandのTCBがreadyque.headに来るためcommandのスレッドが実行される。  


### syscall_intr()が実行される

- syscall_intr()[kozo.c] が実行される
  - syscall_proc(current->syscall.type, current->syscall.param);
- call_function()でthread_run()が実行されてsyscallでスレッドに渡したsyscall.paramのunionの中にある実行したい関数ポインタなどを渡す
   - thread_run()ではスタックに新しい領域を確保。TCBを作成。thread_init()が呼ばれるようにスタックにthread_initの関数ポインタをセット。
     - context.spに今の設定があるspのアドレスをセット。 
     - kz_runシステムコールを呼んだスレッドをreadyqueに戻す。 
     - currentに今作ったTCBをセットしてreadyqueに追加
- 割り込みはここで終了。


　
