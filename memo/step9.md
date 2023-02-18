# 9th step 優先度スケジューリング

デバッグ付きの実行結果.  
ReadyQue: のtest09_1:1の:1はpriorityの数字.  


```
kozos boot succeed! step9
syscall_proc-1; Current: idle; ReadyQue: idle:0, 
syscall_proc-2; Current: idle; ReadyQue: 
syscall_proc-1; Current: idle; ReadyQue: idle:0, test09_1:1, 
syscall_proc-2; Current: idle; ReadyQue: test09_1:1, 
syscall_proc-1; Current: idle; ReadyQue: idle:0, test09_1:1, test09_2:2, 
syscall_proc-2; Current: idle; ReadyQue: test09_1:1, test09_2:2, 
syscall_proc-1; Current: idle; ReadyQue: idle:0, test09_1:1, test09_2:2, test09_3:3, 
syscall_proc-2; Current: idle; ReadyQue: test09_1:1, test09_2:2, test09_3:3, 
test09_1 started.
test09_1 sleep in.
syscall_proc-1; Current: test09_1; ReadyQue: test09_1:1, test09_2:2, test09_3:3, idle:f, 
syscall_proc-2; Current: test09_1; ReadyQue: test09_2:2, test09_3:3, idle:f, 
test09_2 started.
test09_2 sleep in.
syscall_proc-1; Current: test09_2; ReadyQue: test09_2:2, test09_3:3, idle:f, 
syscall_proc-2; Current: test09_2; ReadyQue: test09_3:3, idle:f, 
test09_3 started.
test09_3 wakeup in test09_1 in.
syscall_proc-1; Current: test09_3; ReadyQue: test09_3:3, idle:f, 
syscall_proc-2; Current: test09_3; ReadyQue: idle:f, 
test09_1 sleep out.
test09_1 chpri in.
syscall_proc-1; Current: test09_1; ReadyQue: test09_1:1, test09_3:3, idle:f, 
syscall_proc-2; Current: test09_1; ReadyQue: test09_3:3, idle:f, 
test09_3 wakeup out.
test09_3 wakeup in test09_2 in.
syscall_proc-1; Current: test09_3; ReadyQue: test09_3:3, test09_1:3, idle:f, 
syscall_proc-2; Current: test09_3; ReadyQue: test09_1:3, idle:f, 
test09_2 sleep out.
test09_2 chpri in.
syscall_proc-1; Current: test09_2; ReadyQue: test09_2:2, test09_1:3, test09_3:3, idle:f, 
syscall_proc-2; Current: test09_2; ReadyQue: test09_1:3, test09_3:3, idle:f, 
test09_1 chpri out.
test09_1 wait int.
syscall_proc-1; Current: test09_1; ReadyQue: test09_1:3, test09_3:3, test09_2:3, idle:f, 
syscall_proc-2; Current: test09_1; ReadyQue: test09_3:3, test09_2:3, idle:f, 
test09_3 wakeup out.
test09_3 wait int.
syscall_proc-1; Current: test09_3; ReadyQue: test09_3:3, test09_2:3, test09_1:3, idle:f, 
syscall_proc-2; Current: test09_3; ReadyQue: test09_2:3, test09_1:3, idle:f, 
test09_2 chpri out.
test09_2 wait int.
syscall_proc-1; Current: test09_2; ReadyQue: test09_2:3, test09_1:3, test09_3:3, idle:f, 
syscall_proc-2; Current: test09_2; ReadyQue: test09_1:3, test09_3:3, idle:f, 
test09_1 wait out.
test09_1 trap int.
test09_1 Down.
test09_1 EXIT. 
test09_3 wait out.
test09_3 exit in.
syscall_proc-1; Current: test09_3; ReadyQue: test09_3:3, test09_2:3, idle:f, 
syscall_proc-2; Current: test09_3; ReadyQue: test09_2:3, idle:f, 
test09_3 EXIT. 
test09_2 wait out.
test09_2 exit.
syscall_proc-1; Current: test09_2; ReadyQue: test09_2:3, idle:f, 
syscall_proc-2; Current: test09_2; ReadyQue: idle:f, 
test09_2 EXIT. 

```

