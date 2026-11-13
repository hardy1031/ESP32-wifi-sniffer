# learning/ の note を書く rule

このフォルダの note を書く・直すときは必ずこの rule に従う。

## 形式
- 1 file = 1 topic。file 名は英語 kebab-case（例: `freertos-tasks-and-scheduler.md`）。
- 構成は必ずこの 4 つだけ:
  - `# <title>`
  - `## 1. 要約`
  - `## 2. 学んだ situation`
  - `## 3. 再現方法`
- 長さ: 全体で約 12–16 行。長くなる topic は複数 file に分ける（1 file に詰め込まない）。

## 各 section
- **1. 要約**: 要点だけを短い箇条書き or 数行で。具体例（log 1 行、code 1 行、数字）を 1 つ入れる。
- **2. 学んだ situation**: どの疑問・作業から学んだかを 1–2 行。
- **3. 再現方法**: 自分で確かめられる command や、log のどこを見るかを 1–3 行。

## 文体
- 日本語の文に、技術用語は英語のまま混ぜる（例: 「task = kernel object」「flash は non-volatile memory」）。
- 短く言い切る。説明の繰り返し・長い表・Sources list は入れない。
- 図は必要なときだけ、数行の code block で。

## 例（既存 note）
`app-main-and-boot-chain.md`, `freertos-tasks-and-scheduler.md`, `serial-port-and-monitor.md`
