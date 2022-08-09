# PiDASPlus

[nrck](https://github.com/nrck)氏作成の [PiDAS](https://github.com/nrck/PiDAS) をC++に移植したものです。
キットや処理の詳細については元のリポジトリをご確認ください。

## オリジナルのコードからの変更点

- 1サイクル1.5ms程度で処理できるようになったことにより、100Hzで処理が行えるようになりました。
  - オリジナルのコードでは1サイクル15~18msかかっており、100Hzで処理できていませんでした。(おま環かも)
- まだ仮ですが、観測情報のNMEAフォーマットでの出力に対応しています。

## つかいかた

1. `$ git clone https://github.com/ingen084/PiDASPlus.git --recursive`
2. PiDASPlus フォルダを PlatformIO で開く
3. ビルドする

## 注意事項

`include/filter.hpp` に含まれている処理は[特許5946067](https://plidb.inpit.go.jp/pldb/html/HTML.L/2016/001/L2016001200.html)を使用しています。  
詳細は[元のリポジトリの記載](https://github.com/nrck/PiDAS/blob/main/NOTICE)もご確認ください。

このあたりの処理については[同人誌](https://booth.pm/ja/items/3022619)で詳しく解説されていますので、興味のある方は購入しましょう。

特許･ライセンス周りがよくわかってないので間違えてたら指摘いただけると嬉しいです…。
