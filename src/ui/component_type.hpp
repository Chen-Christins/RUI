/**
 * @file component_type.hpp
 * @brief Defines the component type property for RUI components.
 * @author Christins (chen.christins@qq.com)
 * @date 2026-07-22
 * @copyright Apache 2.0
 */
#pragma once

namespace rui {

inline constexpr const char* kComponentTypeProperty = "rui_component_type";
inline constexpr const char* kWeightProperty = "rui_weight";
inline constexpr const char* kClickableProperty = "rui_clickable";
inline constexpr const char* kClipProperty = "rui_clip";
inline constexpr const char* kCornerRadiusProperty = "rui_corner_radius";
inline constexpr const char* kFillMaxWidthProperty = "rui_fill_max_width";
inline constexpr const char* kFillMaxHeightProperty = "rui_fill_max_height";
inline constexpr const char* kAlignmentProperty = "rui_alignment";
inline constexpr const char* kOffsetXProperty = "rui_offset_x";
inline constexpr const char* kOffsetYProperty = "rui_offset_y";
inline constexpr const char* kZIndexProperty = "rui_z_index";

inline constexpr const char* kUIWindowProperty = "Window";
inline constexpr const char* kUINavHostProperty = "NavHost";
inline constexpr const char* kUIButtonProperty = "Button";
inline constexpr const char* kUILabelProperty = "Label";
inline constexpr const char* kUIImageProperty = "Image";
inline constexpr const char* kUIColumnProperty = "Column";
inline constexpr const char* kUIRowProperty = "Row";
inline constexpr const char* kUISpacerProperty = "Spacer";
inline constexpr const char* kUITextProperty = "Text";
inline constexpr const char* kUIBoxProperty = "Box";
inline constexpr const char* kUITextFieldProperty = "TextField";
inline constexpr const char* kUITopAppBarProperty = "TopAppBar";
inline constexpr const char* kUIFloatingActionButtonProperty = "FAB";
inline constexpr const char* kUIScaffoldProperty = "Scaffold";
inline constexpr const char* kUILazyColumnProperty = "LazyColumn";
inline constexpr const char* kUILazyRowProperty = "LazyRow";
inline constexpr const char* kUISnackbarHostProperty = "SnackbarHost";
inline constexpr const char* kUISwitchProperty = "Switch";
inline constexpr const char* kUICheckboxProperty = "Checkbox";
inline constexpr const char* kUIRadioButtonProperty = "RadioButton";
inline constexpr const char* kUICardProperty = "Card";
inline constexpr const char* kUIDividerProperty = "Divider";
inline constexpr const char* kUIDialogProperty = "Dialog";
inline constexpr const char* kUIAnimatedVisibilityProperty = "AnimatedVisibility";

} // namespace rui
