# AGENTS.md

## Scope

These instructions apply to the entire RUI repository.

## Project

RUI is a C++20 / CMake / Qt6 UI framework. Prefer existing `src/ui`, `src/core`, and `tools/devtools` patterns over new abstractions.

## Code Style

- Always use braces for `if`, `else`, `for`, and `while`, even for single-line guards.
- Do not write `if (...) return;`, `if (...) continue;`, or `if (...) statement;`.
- Put non-template function implementations in `.cc` files. Headers should contain declarations, templates, and truly necessary `constexpr` or trivial inline definitions only.
- Do not hardcode RUI component type strings at call sites. New UI components must add a `kUI...Property` constant in `src/ui/component_type.hpp` and use that constant with `utils::setComponentType`.
- Keep edits scoped. Avoid unrelated refactors and do not rewrite generated/runtime config unless requested.
- Use ASCII comments for new or edited comments unless the file is already valid UTF-8 Chinese.
- Preserve file encoding. Avoid PowerShell `Set-Content` defaults for source rewrites.
- Prefer `apply_patch` for manual edits.

## Qt/UI Notes

- `LazyList` uses an overlay scrollbar pattern: native scrollbars are hidden, a viewport child scrollbar is synchronized to the native scroll value, and it hides after inactivity.
- Every UI factory/widget must set its component type through `utils::setComponentType(widget, kUI...Property)` or `setProperty(kComponentTypeProperty, kUI...Property)` when direct property access is required.
- `ImageWidget` should render pixmaps in a high-DPI aware way: scale to target physical pixels, set device pixel ratio, then draw in logical coordinates.
- Keep SVG rendering through `QSvgRenderer`.

## Verification

Run after C++ edits:

```powershell
cmake --build --preset mingw-debug
git diff --check
```

If changing code style broadly, inspect control flow for missing braces before finishing.
