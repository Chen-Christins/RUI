/**
 * @file component_type.h
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
inline constexpr const char* kUILazyColumnProperty = "LazyColumn";
inline constexpr const char* kUILazyRowProperty = "LazyRow";

} // namespace rui