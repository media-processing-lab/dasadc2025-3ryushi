# DASADC 2025 三留師チーム プログラム
[DAシンポジウム ADC 2025 プログラミング競技](https://dasadc.github.io/adc2025/programming.html)の提出コード

## 実行用ファイルの生成
### ソースコードのminify

Paiza.IOはプロジェクトの合計テキストサイズに制限がある。ソースコードのスペースや改行を削除することで、入力盤面データのサイズを最大化する。

```bash
./tools/cpp_minifier.py main.cpp -o minified.cpp
```

### バイナリのコンパイル
Paiza.IO の Python 環境 (Ubuntu 20.04, aarch64) で実行可能なバイナリを生成する

Docker が必要

```bash
./tools/g++-aarch64.sh main.cpp -O3 --std=c++17
```

## 実行結果
|盤面データ|実行時間|スコア|Paiza.IO|
|:-:|:-:|:-:|:-:|
|1210_both_dangerous|0.51 s|168,732|[Link↗](https://paiza.io/projects/4KNdhAuXG0btu4KtwJpgIQ)|
|8516_both_middle|2.00 s|837,881<sup>*1</sup>|[Link↗](https://paiza.io/projects/8tkIim70mUrI-UIyjmtlzQ)|

*1: 盤面データパックが Paiza.IO の入力ファイルサイズ制限に引っかかるので、外部サーバーに配置。Python スクリプトでダウンロードしてバイナリを実行
