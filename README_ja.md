# Antialias

Antialias は、Adobe Photoshop SDK を使って構築された Windows 向け Adobe Photoshop フィルタープラグインです。

## 機能

- 選択した画像のピクセルにアンチエイリアス処理を適用します。
- グレースケール画像と RGB 画像をサポートします。
- アルファ値は保持します。

## 対応範囲

このプラグインは現在、以下をサポートしています。

- RGB 画像
- アルファ付き RGB 画像
- グレースケール画像
- アルファ付きグレースケール画像
- Windows x64 ビルドターゲット

その他の Photoshop 画像モードは対象外です。
32 ビット浮動小数点画素形式には対応していません。

## プロジェクト構成

- [myfilterplugin/common/MyFilter.cpp](myfilterplugin/common/MyFilter.cpp): Photoshop フィルターの実装
- [myfilterplugin/common/PiPLs.json](myfilterplugin/common/PiPLs.json): プラグイン登録メタデータ
- [myfilterplugin/win/MyFilter.vcxproj](myfilterplugin/win/MyFilter.vcxproj): `.8bf` プラグイン用の Visual Studio/MSBuild プロジェクト
- [Makefile](Makefile): ビルド用の補助コマンド

## ビルド要件

- Windows
- Visual Studio 2026 と MSVC ビルドツール
- Adobe Photoshop SDK がこのワークスペースの `pluginsdk` 配下に展開されていること

このワークスペースには、MSVC のコマンドライン環境を初期化するための [vcvars.bat](vcvars.bat) が含まれています。

## 想定ディレクトリ構成

このリポジトリは次の構成を前提としています。

```text
.
├── Makefile
├── README.md
├── README_ja.md
├── vcvars.bat
├── myfilterplugin/
│   ├── common/
│   └── win/
└── pluginsdk/
    ├── photoshopapi/
    │   ├── photoshop/
    │   └── pica_sp/
    └── samplecode/
        └── common/
            └── includes/
```

重要な点:

- `myfilterplugin/win/MyFilter.vcxproj` は相対パスの include 設定を使っています。
- Adobe Photoshop SDK は [pluginsdk](pluginsdk) の直下に配置されている必要があります。
- プロジェクトが期待する Photoshop SDK ヘッダーの場所:
  - [pluginsdk/photoshopapi/photoshop](pluginsdk/photoshopapi/photoshop)
  - [pluginsdk/photoshopapi/pica_sp](pluginsdk/photoshopapi/pica_sp)
- さらに、共有される SDK サンプルヘッダーも使用します:
  - [pluginsdk/samplecode/common/includes](pluginsdk/samplecode/common/includes)

Photoshop SDK を別のフォルダー名や深さに展開した場合は、[myfilterplugin/win/MyFilter.vcxproj](myfilterplugin/win/MyFilter.vcxproj) の include パスを更新する必要があります。

## ビルド方法

ワークスペースのルートで、まず MSVC 環境を初期化します。

```bat
call vcvars.bat
```

その後、次のいずれかを実行します。

Debug ビルド:

```bat
msbuild myfilterplugin\win\MyFilter.vcxproj /p:Configuration=Debug /p:Platform=x64 /m
```

Release ビルド:

```bat
msbuild myfilterplugin\win\MyFilter.vcxproj /p:Configuration=Release /p:Platform=x64 /m
```

または、Makefile ターゲットを使えます。

```bat
nmake build-debug
```

```bat
nmake build-release
```

```bat
nmake clean
```

[Makefile](Makefile) には `install` ターゲットもありますが、`C:\Program Files\...` に書き込むため管理者権限が必要です。

## ビルド出力

生成される Photoshop プラグインは次のとおりです。

- [myfilterplugin/build/Debug/Antialias.8bf](myfilterplugin/build/Debug/Antialias.8bf)

## Photoshop での使い方

1. `Antialias.8bf` を Photoshop のプラグインフォルダーへコピーします。
2. Photoshop を起動、または再起動します。
3. RGB またはグレースケールの画像を開きます。
4. 選択範囲を作成します。
5. `Antialias` カテゴリからフィルターを実行します。

期待される動作:

- RGB ドキュメント: 選択された色値にアンチエイリアスが適用されます。
- グレースケールドキュメント: 選択されたグレースケール値にアンチエイリアスが適用されます。
- アルファチャンネルは保持されます。

