MES_10b5s-2_7045


これは、SH7045専用のMES(H8/OS)である。


MES_SRC_10b5s+2 Copyright (C) -2009 Yukio Mitsuiwa, Microtechno

オリジナルソースMES_SRC_10b5s+2.tgzは、マイクロテクノ社のアーカイブから取得した。
https://web.archive.org/web/20090104055919/http://microtechno.dip.jp/~mt_req/cdrom/VER2/H8OS/index.htm

MES, H8/OSの原作者である三岩幸夫氏のページは、アーカイブされている
https://web.archive.org/web/20051028051248/http://mes.sourceforge.jp/mes/index.html


ソース中で三岩氏がCopyleftを宣言されているので、GPL2/LGPL2による配布と見做した。


このソースツリーMES_SRC_10b5s+2-7045は、yasunoxx▼JuliaがSH7045専用として独自に改変したものである。これはオリジナルと同一のbuild環境、すなわちsh-coff-gcc用である。本バージョンにおいてH8向けの記述は不要なので削除した(サポートも出来ないので)。


本バージョンを含めたMESおよびH8/OSのサポートは行わない。動作については何ら保証しない。


ツールの依存関係：
・sh-gcc-3.2
・sh-newlib-1.10.0
・sh-binutils-2.13
・ホスト環境のgcc, make(tool/configを生成するために必要)
※必要に応じて、上記のbuildが通るOSが必要になるだろう。


特記事項：

・本バージョンにおいて、LAN関連の動作は全く確認していない。同様に、IDEやSL811HS関連の動作も全く確認していない。おそらく、今後も動作の確認は行わないし、新規にデバイスを接続した時にツリーから削除するかもしれない。

・外付けRAMは、/CS1空間に1MiBのSRAMが接続されているものとする。内蔵RAMはMESがワークエリアとして使用している。今後、キャッシュを有効にするかもしれないので、内蔵RAMの使用は推奨しない。


TODO：

・今後はプロジェクト名を"MES_10b5-2_7045"に統一する。
・クロスコンパイラを現行バージョンのGCC(sh-unknown-elf)に対応させる。
・SPIおよびSDカードストレージのサポート。
・適当なUSBホスト、ストレージクラスUSBデバイスのサポート。
・Wi-Fi, Bluetooth, IEEE802.15.4/ZigBeeのサポート。有線LANのサポートは行わないかも。
・SH7144サポート。SH以外のアーキテクチャサポート(MC680x0, Microblaze等)も考えている。


(C)2024 yasunoxx▼Julia <yasunoxx@gmail.com>
・本件に関する問い合わせメール等、簡潔な内容であれば応答するかもしれない。保証はしない。
