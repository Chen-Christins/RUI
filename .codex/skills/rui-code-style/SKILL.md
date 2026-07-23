---
name: rui-code-style
description: Apply RUI repository coding conventions and review rules. Use when editing C++/Qt code in this repository, touching UI components under src/ui, updating tools/devtools C++ code, changing image or scroll behavior, or preparing code for build verification and review.
---

# RUI Code Style

## Core Workflow

Read the local code first and match existing architecture. Keep changes scoped to the requested behavior and avoid unrelated refactors.

For C++ changes, run:

```powershell
cmake --build --preset mingw-debug
```

Also run `git diff --check` before finishing when source files were edited.

## C++ Style

- Always use braces for `if`, `else`, `for`, and `while` bodies, including single-line guards.
- Do not write `if (...) return;`, `if (...) continue;`, or `if (...) statement;`.
- Keep `else if` on the same line as the closing brace: `} else if (...) {`.
- Put non-template function implementations in `.cc` files. Keep headers to declarations, templates, and genuinely necessary `constexpr` or trivial inline definitions.
- Concrete singleton implementation classes must be named `*Manager`, with a `using *Mgr = Singleton<*Manager>;` alias for access. Call sites should use `*Mgr::Instance()`.
- Do not hardcode RUI component type strings at call sites. New UI components must add a `kUI...Property` constant in `src/ui/component_type.hpp` and use that constant with `utils::setComponentType`.
- Prefer early returns when they simplify control flow, but still use braces.
- Use ASCII comments for new or edited comments unless the surrounding file is clearly maintained in valid UTF-8 Chinese.
- Do not re-encode files accidentally. Preserve UTF-8 text and avoid PowerShell `Set-Content` defaults for source rewrites.
- Prefer `apply_patch` for manual edits. Use scripts only for mechanical changes, then inspect the diff carefully.

Example:

```cpp
if (!widget) {
    return;
}

for (auto* child : children) {
    if (!child) {
        continue;
    }
}
```

## Qt UI Rules

- Keep component behavior inside the owning `src/ui` component unless a shared abstraction is already present.
- Every UI factory/widget must set its component type through `utils::setComponentType(widget, kUI...Property)` or `setProperty(kComponentTypeProperty, kUI...Property)` when direct property access is required.
- For `QScrollArea`-based components that need overlay scrollbars, hide the native scrollbar policy and synchronize a viewport child scrollbar with the native scroll value/range.
- Overlay scrollbars should not consume layout space. Show them only during scroll/drag activity and hide them with a short timer.
- For pixmap image rendering on high-DPI screens, scale to target physical pixels using `devicePixelRatioF()`, set the scaled pixmap DPR, then draw in logical coordinates.
- Keep SVG rendering vector-based through `QSvgRenderer`.

## Validation

Before final response after code edits:

- Run `cmake --build --preset mingw-debug`.
- Run `git diff --check`.
- Confirm C++ control-flow scan if the task involved style cleanup:

```powershell
rg -n "\b(if|else if|else|for|while)\b" src tests tools -S
```

Use the scan as a starting point, not a proof; inspect actual control flow before broad mechanical edits.
