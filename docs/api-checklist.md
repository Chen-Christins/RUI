# RUI API 对齐清单

## 图例
- `[ ]` 待实现
- `[x]` 已完成

---

## 1. 布局 (Layout)

| Component | Status | Notes |
|-----------|--------|-------|
| `Column` | [x] | `Arrangement`, `Alignment` |
| `Row` | [x] | `Arrangement`, `Alignment` |
| `Box` | [x] | 层叠布局, `Modifier.align()` |
| `Spacer` | [x] | `Modifier` |
| `LazyColumn` | [x] | `itemCount`, `factory(int index)` |
| `LazyRow` | [x] | `itemCount`, `factory(int index)` |
| `FlowRow` | [x] | 自动换行布局 |
| `FlowColumn` | [x] | 自动换列布局 |
| `Scaffold` | [x] | `topBar`, `content`, `bottomBar`, `floatingActionButton` |
| `TopAppBar` / `CenterAlignedTopAppBar` | [x] | `title`, `navigationIcon`, `actions` |
| `FloatingActionButton` | [x] | `onClick`, `content`, `modifier` |
| `ConstraintLayout` | [ ] | `ref()`, `constrainAs()` |

---

## 2. Modifier

| Modifier | Status | Notes |
|----------|--------|-------|
| `size(w, h)` / `size(s)` | [x] | |
| `width(w)` | [x] | |
| `height(h)` | [x] | |
| `fillMaxWidth(fraction)` | [x] | |
| `fillMaxHeight(fraction)` | [x] | |
| `fillMaxSize(fraction)` | [x] | |
| `weight(f)` | [x] | |
| `padding(all)` / `padding(h, v)` | [x] | |
| `align(alignment)` | [x] | 在 `Box` / `Column` / `Row` 内 |
| `background(color, shape)` | [x] | |
| `foreground(color)` | [x] | |
| `border(stroke, shape)` | [x] | `BorderStroke`, `RoundedCornerShape` |
| `clip(shape)` | [x] | |
| `clickable(onClick)` | [x] | |
| `alpha(value)` | [x] | |
| `fontSize(px)` | [x] | 仅 `Text` |
| `offset(x, y)` | [x] | 偏移不影响布局（在 `Box` 内生效） |
| `shadow(elevation, shape)` | [x] | 阴影/高程 |
| `zIndex(z)` | [x] | 叠放次序（在 `Box` 内生效） |
| `graphicsLayer { }` | [x] | 缩放/旋转/位移/透明度（与 shadow 合并为统一 effect 栈） |
| `requiredWidth(w)` | [x] | 强制宽度（忽略 max） |
| `requiredHeight(h)` | [x] | 强制高度 |
| `requiredSize(s)` | [x] | 强制尺寸 |
| `defaultMinSize(minW, minH)` | [x] | 最小尺寸但不强制 |
| `aspectRatio(ratio)` | [x] | 宽高比 |
| `intrinsicSize(IntrinsicSize.Min/Max)` | [ ] | 内部尺寸策略 |
| `onGloballyPositioned { }` | [x] | 位置回调 |
| `onSizeChanged { }` | [x] | 尺寸回调 |
| `focusable()` / `onFocusChanged { }` | [x] | 焦点 |
| `hoverable(onHoverChanged)` | [x] | 悬停 Enter/Leave 回调 |
| `pointerInput(key) { }` | [ ] | 自定义手势 |
| `draggable { }` | [ ] | |
| `scrollable { }` | [ ] | |
| `combinedClickable(onLongClick, onClick)` | [x] | |

---

## 3. Material 组件

| Component | Status | Notes |
|-----------|--------|-------|
| `Text` | [x] | `fontSize`, `foreground`, `align`, `fontWeight`?, `lineHeight`? |
| `Button` | [x] | `onClick`, `Modifier`, `content` |
| `Image` | [x] | `ContentScale`, `ColorFilter`, `Alignment`, `alpha` |
| `TextField` / `OutlinedTextField` | [x] | `state`, `placeholder`, `label`, `enabled`, `readOnly`, `singleLine` |
| `Switch` | [x] | `checked`, `onCheckedChange`, `enabled` |
| `Checkbox` | [x] | `checked`, `onCheckedChange`, `enabled` |
| `TriStateCheckbox` | [x] | `state` (`Checked`/`Unchecked`/`Indeterminate`) |
| `RadioButton` | [x] | `selected`, `onSelectedChange` |
| `Card` | [x] | `content`, `modifier`, `cornerRadius`, `elevation` |
| `Surface` | [x] | `shape`, `elevation`, `color`, `content` |
| `Divider` | [x] | `thickness`, `color` |
| `Dialog` / `AlertDialog` | [x] | `AlertDialog` 已完成 (`onDismissRequest`, `confirmButton`, `dismissButton`, `title`, `text`) |
| `DropdownMenu` | [x] | `expanded`, `onDismissRequest`, `DropdownMenuItem` |
| `TopAppBar` / `CenterAlignedTopAppBar` | [x] | `title`, `navigationIcon`, `actions` |
| `BottomAppBar` | [ ] | |
| `FloatingActionButton` | [x] | `onClick`, `containerColor`, `content` |
| `BottomNavigation` / `NavigationBar` (M3) | [ ] | `items { }`, `selected`, `onClick` |
| `NavigationRail` | [ ] | |
| `Slider` | [x] | `value`, `onValueChange`, `valueRange`, `steps` |
| `CircularProgressIndicator` | [x] | `progress`, `strokeWidth`, `color` |
| `LinearProgressIndicator` | [x] | `progress`, `color`, `trackColor` |
| `Chip` / `InputChip` / `FilterChip` / `ChoiceChip` / `AssistChip` | [x] | `Chip` / `FilterChip` 已完成 |
| `Badge` / `BadgedBox` | [x] | |
| `Snackbar` / `SnackbarHost` | [x] | `SnackbarHostState`, `SnackbarHost` |
| `ModalNavigationDrawer` | [ ] | `drawerContent`, `content` |
| `ModalBottomSheet` | [ ] | |
| `ModalDatePicker` / `DatePicker` | [ ] | |
| `TimePicker` | [ ] | |
| `Tab` / `TabRow` | [ ] | |
| `TabRowDefaults.SecondaryTabRow` | [ ] | |

---

## 4. 动画 (Animation)

| API | Status | Notes |
|-----|--------|-------|
| `animateFloatAsState(target, spec, finishedListener)` | [x] | |
| `animateDpAsState(target, spec)` | [x] | |
| `animateColorAsState(target, spec)` | [x] | |
| `animateSizeAsState()` | [x] | `Size` 几何类型 |
| `animateOffsetAsState()` | [x] | `Offset` 几何类型 |
| `animateRectAsState()` | [x] | `Rect` 几何类型 |
| `animateIntAsState()` | [x] | |
| `AnimatedVisibility(visible, enter, exit, content)` | [x] | 淡入淡出 (`fadeIn`/`fadeOut`) |
| `AnimatedContent(targetState, transitionSpec, content)` | [x] | 内容切换淡入淡出 |
| `Crossfade(targetState, content)` | [x] | `AnimatedContent` 默认别名 |
| `rememberInfiniteTransition()` | [x] | 无限循环 (`repeatable` 反向) |
| `updateTransition(targetState, label)` | [x] | 多属性统一过渡 |
| `AnimationSpec`: `tween`, `spring`, `keyframes`, `repeatable`, `snap` | [x] | `tween`/`spring`/`snap`/`repeatable`/`keyframes` 已完成 |
| `Easing`: `LinearEasing`, `FastOutSlowInEasing`, `FastOutLinearInEasing`, ... | [x] | 常见缓动曲线 |
| `Animatable(initialValue, typeConverter)` | [x] | `Animatable<T>` 泛化实现 |

---

## 5. 副作用 / 协程 (Effects / Side-effects)

| API | Status | Notes |
|-----|--------|-------|
| `LaunchedEffect(key, block)` | [x] | 在 Composable 中启动协程, key 变化时重启 |
| `rememberCoroutineScope()` | [x] | 获取协程作用域 |
| `DisposableEffect(key, effect { onDispose { } })` | [x] | 生命周期绑定, 离开 composition 时 dispose |
| `SideEffect(block)` | [x] | 每次重组时回调 |
| `derivedStateOf { computation }` | [x] | 派生状态, 仅依赖变化时重算 |
| `snapshotFlow { }` | [x] | 将 State 转为 Flow |
| `produceState(initialValue, key, producer)` | [x] | 将非 Compose 异步数据转为 State |

---

## 6. 组合上下文 (Composition / Local)

| API | Status | Notes |
|-----|--------|-------|
| `CompositionLocalProvider(local to value) { content }` | [x] | |
| `staticCompositionLocalOf { default }` | [x] | |
| `compositionLocalOf { default }` | [x] | |
| `LocalConfiguration` | [x] | 屏幕配置 (密度/字体缩放/横竖屏) |
| `LocalDensity` | [x] | dp/px/sp 转换 |

---

## 7. 绘制 (Canvas / Drawing)

| API | Status | Notes |
|-----|--------|-------|
| `Canvas(modifier, onDraw: DrawScope.() -> Unit)` | [ ] | |
| `DrawScope.drawRect(color, topLeft, size)` | [ ] | |
| `DrawScope.drawCircle(color, radius, center)` | [ ] | |
| `DrawScope.drawOval(color, topLeft, size)` | [ ] | |
| `DrawScope.drawRoundRect(color, topLeft, size, cornerRadius)` | [ ] | |
| `DrawScope.drawLine(color, start, end, strokeWidth)` | [ ] | |
| `DrawScope.drawArc(color, startAngle, sweepAngle, useCenter, topLeft, size)` | [ ] | |
| `DrawScope.drawPath(path, color)` | [ ] | |
| `DrawScope.drawImage(image, topLeft)` | [ ] | |
| `Path { }` / `PathBuilder` | [ ] | `lineTo`, `quadraticTo`, `cubicTo`, `arcTo`, `close` |
| `drawBehind { }` / `drawWithContent { }` 修饰符 | [ ] | |

---

## 8. 手势 (Gesture / Input)

| API | Status | Notes |
|-----|--------|-------|
| `detectTapGestures(onTap, onDoubleTap, onLongPress, onPress)` | [ ] | |
| `detectDragGestures(onDragStart, onDrag, onDragEnd, onDragCancel)` | [ ] | |
| `detectTransformGestures(onGesture)` | [ ] | 缩放/旋转/平移 |
| `detectVerticalDragGestures` / `detectHorizontalDragGestures` | [ ] | |
| `Modifier.scrollable(state, orientation)` | [ ] | |
| `Modifier.draggable(state, orientation)` | [ ] | |
| `Modifier.swipeable(state, anchors, orientation)` | [ ] | 滑动到锚点 |
| `Modifier.transformable(state)` | [ ] | 多指变换 |

---

## 9. Foundation / 其他

| API | Status | Notes |
|-----|--------|-------|
| `LazyVerticalGrid` / `LazyHorizontalGrid` | [ ] | `GridCells.Fixed(n)` / `Adaptive(minSize)` |
| `StaggeredGrid` | [ ] | 瀑布流 |
| `HorizontalPager` / `VerticalPager` | [ ] | 分页滑动 |
| `AnimatedListState` / `LazyListState` | [ ] | 滚动/列表状态 |
| `ScrollState` | [ ] | `.value`, `.isScrollInProgress` |
| `LazyLayoutState` | [ ] | |
| `@Stable` / `@Immutable` | [ ] | 标记稳定类型 |
| `keep()` / `saveable` | [ ] | 状态保存/恢复 |
| `Modifier.semantics { }` / `clearAndSetSemantics` | [ ] | 无障碍 |
| `WindowInsets` / `imePadding()` / `systemBarsPadding()` | [ ] | 窗口内边距 |

---

## 优先级建议

**P0 (核心功能, 缺了没法用):**
- `TextField` / `OutlinedTextField`
- `LazyColumn` / `LazyRow`
- `Scaffold` + `TopAppBar` + `FloatingActionButton` + `SnackbarHost`
- `LaunchedEffect` / `rememberCoroutineScope`
- `animateFloatAsState` + `AnimationSpec`

**P1 (常用组件):**
- `Switch`, `Checkbox`, `RadioButton`
- `Card`, `Divider`
- `Dialog` / `AlertDialog`
- `AnimatedVisibility`
- `derivedStateOf`
- `offset`, `shadow`, `graphicsLayer`, `zIndex` 修饰符

**P2 (补充完善):**
- `Slider`, `ProgressIndicator`, `Chip`, `Badge`, `DropdownMenu`
- `CompositionLocalProvider`
- `onGloballyPositioned`, `onSizeChanged`
- `FlowRow` / `FlowColumn`

**P3 (进阶/桌面端专用):**
- `LazyVerticalGrid`, `StaggeredGrid`, `HorizontalPager`
- `NavigationRail`, `NavigationDrawer`, `BottomSheet`
- `Canvas`, `DrawScope`
- 高级手势 (`detectDragGestures`, `detectTransformGestures`)
- `ConstraintLayout`
