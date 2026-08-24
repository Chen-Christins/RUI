# RUI 开发路线图

> 本文档描述 RUI 框架后续功能开发规划。已完成能力的详细清单见 `docs/api-checklist.md`。

## 阶段 A — 基础能力补全（当前阶段）

补齐响应式状态、组合上下文、布局修饰与动画规格等基础设施，为后续所有特性打地基。

| # | 任务 | 状态 | 说明 |
|---|------|------|------|
| A1 | `derivedStateOf` | 待实现 | 读取追踪 + `DerivedState<T>`，仅依赖变化时重算 |
| A2 | `produceState` / `snapshotFlow` | 待实现 | 异步数据转为 State；Flow 抽象（拉伸） |
| A3 | `CompositionLocal` + `LocalDensity`/`LocalConfiguration` | 待实现 | 组合上下文、dp/sp 换算、屏幕配置 |
| A4 | Modifier 补全 | 待实现 | `aspectRatio`/`graphicsLayer`/`onSizeChanged`/`onGloballyPositioned`/`requiredSize`/`defaultMinSize`/`focusable`/`combinedClickable` |
| A5 | `keyframes` 动画规格 | 待实现 | 任意 fraction 关键帧插值 |

## 阶段 B — 桌面端常用组件

| # | 任务 | 状态 |
|---|------|------|
| B1 | `FlowRow` / `FlowColumn` | 待实现 |
| B2 | `Surface` / `TriStateCheckbox` / `BottomAppBar` | 待实现 |
| B3 | `Tab` / `TabRow` / `SecondaryTabRow` | 待实现 |
| B4 | `NavigationBar` / `NavigationRail` | 待实现 |
| B5 | `ModalBottomSheet` / `ModalNavigationDrawer` | 待实现 |

## 阶段 C — 绘图与手势

| # | 任务 | 状态 |
|---|------|------|
| C1 | `Canvas` + `DrawScope` + `Path`/`PathBuilder` | 待实现 |
| C2 | `drawBehind` / `drawWithContent` | 待实现 |
| C3 | 手势检测（tap/drag/transform） | 待实现 |

## 阶段 D — 列表与高级组件

| # | 任务 | 状态 |
|---|------|------|
| D1 | `LazyVerticalGrid` / `LazyHorizontalGrid` / `StaggeredGrid` | 待实现 |
| D2 | `HorizontalPager` / `VerticalPager` | 待实现 |
| D3 | `ScrollState` / `LazyListState` | 待实现 |
| D4 | `ModalDatePicker` / `TimePicker` | 待实现 |

## 阶段 E — 平台 / 无障碍 / 工程化

| # | 任务 | 状态 |
|---|------|------|
| E1 | `WindowInsets` / `imePadding` / `systemBarsPadding` / `semantics` | 待实现 |
| E2 | 引入单元测试（当前仅 `tests/test.cc` 演示） | 待实现 |
| E3 | DevTools 增强（Canvas 节点、状态变更时间线） | 待实现 |
| E4 | 修正 `api-checklist.md` 文档不一致 | 待实现 |

## 技术债

- `ComposableContext::remember` 为位置索引而非 key 匹配（`src/core/composable.hpp`），条件分支下槽位可能错位。建议在阶段 A 末尾评估是否改为 key-based memoization。
- `graphicsLayer` 与 `shadow` 共用 `QGraphicsEffect`（一个 widget 只能挂一个），需合并到统一 effect 栈。
