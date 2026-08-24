# RUI 开发路线图

> 本文档描述 RUI 框架后续功能开发规划。已完成能力的详细清单见 `docs/api-checklist.md`。

## 阶段 A — 基础能力补全（收尾阶段）

补齐响应式状态、组合上下文、布局修饰与动画规格等基础设施，为后续所有特性打地基。

| # | 任务 | 状态 | 说明 |
|---|------|------|------|
| A1 | `derivedStateOf` | 已完成 | 读取追踪 + `DerivedState<T>`，仅依赖变化时重算 |
| A2 | `produceState` | 已完成 | 异步数据转为 State |
| A2b | `snapshotFlow` / Flow 抽象 | 已完成 | 将 State 转为 Flow（`src/core/flow.hpp`） |
| A3 | `CompositionLocal` + `LocalDensity`/`LocalConfiguration` | 已完成 | 组合上下文、dp/sp 换算、屏幕配置 |
| A4 | Modifier 补全（常规项） | 已完成 | `aspectRatio`/`onSizeChanged`/`onGloballyPositioned`/`requiredSize`/`defaultMinSize`/`focusable`/`combinedClickable` |
| A4b | `graphicsLayer` | 已完成 | 缩放/旋转/位移/透明度，已与 `shadow` 合并到统一 `RuiGraphicsEffect` 栈 |
| A4c | `intrinsicSize` / 手势修饰符 | 待实现 | `intrinsicSize`、`pointerInput`/`draggable`/`scrollable`/`swipeable`/`transformable` |
| A5 | `keyframes` 动画规格 | 已完成 | 任意 fraction 关键帧插值 |

## 阶段 B — 桌面端常用组件

| # | 任务 | 状态 |
|---|------|------|
| B1 | `FlowRow` / `FlowColumn` | 已完成 |
| B2 | `Surface` / `TriStateCheckbox` / `BottomAppBar` | 部分完成（缺 `BottomAppBar`） |
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
| D3 | `ScrollState` / `LazyListState` | LazyListState 已实现（`firstVisibleIndex` / `scrollToItem` / `animateScrollToItem`），ScrollState 待实现 |
| D4 | `ModalDatePicker` / `TimePicker` | 待实现 |
| D5 | `LazyColumn`/`LazyRow` Compose 对齐 | 待实现 | 见下方「LazyList Compose 对齐」说明 |

## 阶段 E — 平台 / 无障碍 / 工程化

| # | 任务 | 状态 |
|---|------|------|
| E1 | `WindowInsets` / `imePadding` / `systemBarsPadding` / `semantics` | 待实现 |
| E2 | 引入单元测试（当前仅 `tests/test.cc` 演示） | 待实现 |
| E3 | DevTools 增强（Canvas 节点、状态变更时间线） | 待实现 |
| E4 | 修正 `api-checklist.md` 文档不一致 | 待实现 |

## 技术债

- `ComposableContext::remember` 已新增 keyed `remember(key, init)` 重载（`src/core/composable.hpp`），条件分支内调用点改用 keyed 版本；位置式 `remember` 仍保留，供无条件稳定调用点使用。
- `graphicsLayer` 与 `shadow` 已合并到统一 `RuiGraphicsEffect` 栈（`src/ui/graphics_effect.hpp`），不再互相覆盖。

## LazyList Compose 对齐

当前 `LazyColumn`/`LazyRow`（`src/ui/lazy_list.cc`）已对齐的核心机制：可变 item 真实测量、按 offset 精确定位、key 化 item 复用、`LazyListState`（`firstVisibleIndex`/`scrollToItem`/`animateScrollToItem`）。

仍与 Jetpack Compose 存在差距，待实现：

| 维度 | Compose | RUI 现状 |
|------|---------|---------|
| API 形态 | `LazyColumn { items(list, key=...) }` 声明式 DSL | `LazyColumn(count, factory, key)` 命令式回调 |
| 数据源 | `LazyListScope`，随状态重组 | 一次性 `itemCount`，无动态增删通知 |
| 高级特性 | `stickyHeader` / `span` / `contentPadding` / `itemType` | 均未实现 |
| key 复用语义 | key 复用 + recomposition 按 key 重算内容 | key 只复用 widget 实例，内容不重算 |

其中声明式 DSL、动态数据、sticky 等需在 composable 层支撑，非 LazyList 单点可完成。
