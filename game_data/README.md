# Victoria 2 游戏原始数据目录 (Game Data Directory)

本目录用于存放原版《维多利亚2》（Victoria 2）及其资料片（A House Divided 与 Heart of Darkness）的原始 PC 游戏资源文件，供本地开发、调试与自动化资源转换使用。

> [!IMPORTANT]
> - 本目录已在 `.gitignore` 中被全局忽略，**不会被提交至 Git 仓库**，避免侵犯游戏版权及导致仓库体积臃肿。
> - 在移动端，APK 会优先通过 SAF (Storage Access Framework) 读取用户选择的外部存储目录；当通过 ADB 推送数据或在宿主模式调试时，引擎检测到 `./game_data` 存在且包含游戏数据，将自动将其挂载为根资产路径。

---

## 预期目录结构 (Expected Directory Layout)

请将完整原版 Victoria 2 根目录下的内容复制至本目录，标准结构如下：

```text
game_data/
├── common/                  # 国家定义、法律定义、建筑、科技、商品与 POP 需求
│   ├── bookmarks.txt
│   ├── countries/
│   ├── defines.lua
│   └── ...
├── gfx/                     # 地图贴图、省份颜色、兵模、国旗与 UI 图片
│   ├── flags/
│   ├── map/
│   └── interface/
├── history/                 # 1836/1861 剧本开局数据、各国历史事件与省份初始状态
│   ├── countries/
│   ├── diplomacy/
│   ├── pops/1836.1.1/
│   └── provinces/
├── map/                     # 地图拓扑、省份定义、地形高度图与邻接表
│   ├── default.map
│   ├── definition.csv
│   ├── positions.txt
│   ├── provinces.bmp
│   └── rivers.bmp
├── interface/               # Paradox .gui 界面模板定义
├── localisation/            # 文本本地化与多语言 .csv 字典
├── sound/                   # 音效文件
├── music/                   # 背景音乐
└── mod/                     # 可选：MOD 模组目录 (.mod 文件与模组子文件夹)
```

---

## 开发与调试说明

1. **场景快照生成**:
   - 引擎首次加载时，会自动遍历 `game_data/history`、`map` 与 `common`，生成优化的 1836.1.1 单剧本流式二进制场景缓存 (`.bin`)。
2. **快速热更**:
   - 后续在阶段二（UI）与阶段三（经济）调优时，修改 `game_data/common/` 中的参数或 `game_data/map/` 中的数据可即时生效，无需重新打包内置资源。
