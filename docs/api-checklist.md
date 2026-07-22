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
| `LazyColumn` | [ ] | 虚拟列表, `items(items) { }`, `item { }` |
| `LazyRow` | [ ] | 同上, 水平方向 |
| `FlowRow` | [ ] | 自动换行布局 |
| `FlowColumn` | [ ] | 自动换列布局 |
| `Scaffold` | [ ] | `topBar`, `bottomBar`, `floatingActionButton`, `snackbarHost` |
| `ConstraintLayout` | [ ] | `ref()`, `constrainAs()` |

---

## 2. Modifier

| Modifier | Status | Notes |
|----------|--------|-------|
| `size(w, h)` / `size(s)` | [x] | |
| `width(w)` | [ ] | |
| `height(h)` | [ ] | |
| `fillMaxWidth(fraction)` | [x] | |
| `fillMaxHeight(fraction)` | [x] | |
| `fillMaxSize(fraction)` | [ ] | |
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
| `offset(x, y)` | [ ] | 偏移不影响布局 |
| `shadow(elevation, shape)` | [ ] | 阴影/高程 |
| `zIndex(z)` | [ ] | 叠放次序 |
| `graphicsLayer { }` | [ ] | 缩放/旋转/位移/透明度 |
| `requiredWidth(w)` | [ ] | 强制宽度（忽略 max） |
| `requiredHeight(h)` | [ ] | 强制高度 |
| `requiredSize(s)` | [ ] | 强制尺寸 |
| `defaultMinSize(minW, minH)` | [ ] | 最小尺寸但不强制 |
| `aspectRatio(ratio)` | [ ] | 宽高比 |
| `intrinsicSize(IntrinsicSize.Min/Max)` | [ ] | 内部尺寸策略 |
| `onGloballyPositioned { }` | [ ] | 位置回调 |
| `onSizeChanged { }` | [ ] | 尺寸回调 |
| `focusable()` / `onFocusChanged { }` | [ ] | 焦点 |
| `pointerInput(key) { }` | [ ] | 自定义手势 |
| `draggable { }` | [ ] | |
| `scrollable { }` | [ ] | |
| `combinedClickable(onLongClick, onClick)` | [ ] | |

---

## 3. Material 组件

| Component | Status | Notes |
|-----------|--------|-------|
| `Text` | [x] | `fontSize`, `foreground`, `align`, `fontWeight`?, `lineHeight`? |
| `Button` | [x] | `onClick`, `Modifier`, `content` |
| `Image` | [x] | `ContentScale`, `ColorFilter`, `Alignment`, `alpha` |
| `TextField` / `OutlinedTextField` | [x] | `state`, `placeholder`, `label`, `enabled`, `readOnly`, `singleLine` |
| `Switch` | [ ] | `checked`, `onCheckedChange`, `enabled` |
| `Checkbox` | [ ] | `checked`, `onCheckedChange`, `enabled` |
| `TriStateCheckbox` | [ ] | `state` (`Checked`/`Unchecked`/`Indeterminate`) |
| `RadioButton` | [ ] | `selected`, `onClick` |
| `Card` | [ ] | `shape`, `elevation`, `colors`, `content` |
| `Surface` | [ ] | `shape`, `elevation`, `color`, `content` |
| `Divider` | [ ] | `thickness`, `color` |
| `Dialog` / `AlertDialog` | [ ] | `onDismissRequest`, `confirmButton`, `dismissButton`, `title`, `text` |
| `DropdownMenu` | [ ] | `expanded`, `onDismissRequest`, `DropdownMenuItem` |
| `TopAppBar` / `CenterAlignedTopAppBar` | [ ] | `title`, `navigationIcon`, `actions` |
| `BottomAppBar` | [ ] | |
| `FloatingActionButton` | [ ] | `onClick`, `containerColor`, `content` |
| `BottomNavigation` / `NavigationBar` (M3) | [ ] | `items { }`, `selected`, `onClick` |
| `NavigationRail` | [ ] | |
| `Slider` | [ ] | `value`, `onValueChange`, `valueRange`, `steps` |
| `CircularProgressIndicator` | [ ] | `progress`, `strokeWidth`, `color` |
| `LinearProgressIndicator` | [ ] | `progress`, `color`, `trackColor` |
| `Chip` / `InputChip` / `FilterChip` / `ChoiceChip` / `AssistChip` | [ ] | |
| `Badge` / `BadgedBox` | [ ] | |
| `Snackbar` / `SnackbarHost` | [ ] | |
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
| `animateFloatAsState(target, spec, finishedListener)` | [ ] | |
| `animateDpAsState(target, spec)` | [ ] | |
| `animateColorAsState(target, spec)` | [ ] | |
| `animateSizeAsState()` | [ ] | |
| `animateOffsetAsState()` | [ ] | |
| `animateRectAsState()` | [ ] | |
| `animateIntAsState()` | [ ] | |
| `AnimatedVisibility(visible, enter, exit, content)` | [ ] | `fadeIn`, `slideInVertically`, `expandVertically`, ... |
| `AnimatedContent(targetState, transitionSpec, content)` | [ ] | |
| `Crossfade(targetState, content)` | [ ] | |
| `rememberInfiniteTransition()` | [ ] | |
| `updateTransition(targetState, label)` | [ ] | |
| `AnimationSpec`: `tween`, `spring`, `keyframes`, `repeatable`, `snap` | [ ] | |
| `Easing`: `LinearEasing`, `FastOutSlowInEasing`, `FastOutLinearInEasing`, ... | [ ] | |
| `Animatable(initialValue, typeConverter)` | [ ] | |

---

## 5. 副作用 / 协程 (Effects / Side-effects)

| API | Status | Notes |
|-----|--------|-------|
| `LaunchedEffect(key, block)` | [ ] | 在 Composable 中启动协程, key 变化时重启 |
| `rememberCoroutineScope()` | [ ] | 获取协程作用域 |
| `DisposableEffect(key, effect { onDispose { } })` | [ ] | 生命周期绑定, 离开 composition 时 dispose |
| `SideEffect(block)` | [ ] | 每次重组时回调 |
| `derivedStateOf { computation }` | [ ] | 派生状态, 仅依赖变化时重算 |
| `snapshotFlow { }` | [ ] | 将 State 转为 Flow |
| `produceState(initialValue, key, producer)` | [ ] | 将非 Compose 异步数据转为 State |

---

## 6. 组合上下文 (Composition / Local)

| API | Status | Notes |
|-----|--------|-------|
| `CompositionLocalProvider(local to value) { content }` | [ ] | |
| `staticCompositionLocalOf { default }` | [ ] | |
| `compositionLocalOf { default }` | [ ] | |
| `LocalConfiguration` | [ ] | 屏幕配置 (密度/字体缩放/横竖屏) |
| `LocalDensity` | [ ] | dp/px/sp 转换 |

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
