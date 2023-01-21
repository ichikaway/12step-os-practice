1. コンパイル

./shell.sh で端末ログイン
docker内のsrc以下にworkspaceがマウントされているのでmakeしてコンパイル
make でELFファイル生成
make image でmotファイル生成

2. 実行
ボードのrom writeモードに変更
3だけ下げる

rootユーザになって
./write-image.shを実行で書き込み

ボードのスイッチを変更して起動
1と3だけ上げる

terminal.shもしくは
screen /dev/ttyUSB0 9600

screen経由でxmodemでのファイル転送方法
terminal.shでscreen起動後に、
Ctl-a : でコマンド入力になったあとに
exec !! sx -b  (転送したいファイル)
exec !! sx -b workspace/myos/kzload.elf
を入力する

sxは-kbのように-kコマンドを指定するとelfファイルのような大きめのファイルが転送できなくなるので注意。


