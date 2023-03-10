# 7th step 割り込み　メモ

ROMの先頭の割り込みベクターの配列は、配列の番号によってどの割り込みから呼び出されるか決まっている。
例えば、割り込み番号57の場合は、シリアルの受信があった場合にベクター57番目の配列に入っているアドレスがコールされる。

ブートローダ側では、vector.cで呼び出される関数名を配列に指定。例えば `intr_serintr` と配列に定義。  
呼び出される関数はintr.Sにある `_intr_serintr` のセクション。
この中では割り込み時にまず今のレジスタをスタックに退避し、 `interrupt()` 関数をコールする。
関数終了後に退避したレジスタをスタックから戻す。

`interrupt()`の中では、事前にセットされたイベントハンドラがあればその関数ポインタからハンドラをコールする。  
イベントハンドラがセットされる領域は、ブートローダとOSのmain.cそれぞれが共有するsoftvecセクションになる。  
softvecセクションはリンカで定義され物理メモリ`0xffbf20`が利用される。

ブートローダとOSのリンカそれぞれに0xffbf20をsoftvecに利用すると定義され、
interrupt.hの `extern char softvec`でそのアドレスにアクセスできるようになっている。
bootloadの`interrupt.h`では次のように定義される
```c
extern char softvec;
#define SOFTVEC_ADDR (&softvec)
#define SOFTVECS ((softvec_handler_t *)SOFTVEC_ADDR)
```
softvecがリンカのsoftvecセクション、そのアドレスを`SOFTVEC_ADDR`という名前にする。
`SOFTVECS`配列はsoftvecアドレスから始まる `softvec_hander_t`というハンドラの関数ポインタの型になる
```c
typedef void (*softvec_handler_t)(softvec_type_t type, unsigned long sp);
```

OS側のmain.cではブートローダから呼び出されたあとにまずはハンドラ intr()の関数ポインタのアドレスをリンカで定義した `softvec(0xffbf20)`にセットする。  

OS側のmain.cでその後シリアルの割り込みを有効にしてスリープする。シリアルから受信されると割り込みが発生して割り込みベクターから順に処理されてintr()が呼び出される
