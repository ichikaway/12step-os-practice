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

