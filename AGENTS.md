# AGENTS.md

## Scope

These instructions apply to the entire RUI repository.

## Project

RUI is a C++20 / CMake / Qt6 UI framework. Prefer existing `src/ui`, `src/core`, and `tools/devtools` patterns over new abstractions.

## Code Style

- Always use braces for `if`, `else`, `for`, and `while`, even for single-line guards.
- Do not write `if (...) return;`, `if (...) continue;`, or `if (...) statement;`.
- Put non-template function implementations in `.cc` files. Headers should contain declarations, templates, and truly necessary `constexpr` or trivial inline definitions only.
- Concrete singleton implementation classes must be named `*Manager`, with a `using *Mgr = Singleton<*Manager>;` alias for access. Call sites should use `*Mgr::Instance()`.
- Keep singleton manager constructors public; do not add `friend class Singleton<*Manager>`.
- Do not hardcode RUI component type strings at call sites. New UI components must add a `kUI...Property` constant in `src/ui/component_type.hpp` and use that constant with `utils::setComponentType`.
- Keep edits scoped. Avoid unrelated refactors and do not rewrite generated/runtime config unless requested.
- Use ASCII comments for new or edited comments unless the file is already valid UTF-8 Chinese.
- Preserve file encoding. Avoid PowerShell `Set-Content` defaults for source rewrites.
- Prefer `apply_patch` for manual edits.

## Config

- Prefer usage-site config registration with `static auto gName = ConfigVar<T>::Lookup("key", defaultValue, "description")`.
- Do not add dedicated getters to `ConfigManager` for each new setting.
- Keep file discovery, file opening, and fatal startup policy outside the Config module. Config should parse caller-provided input and maintain registered values.
- Config must not know semantic keys such as `workpath`, `logs`, or `devtools.frontend`.
- Use dotted XML paths for nested config, for example `devtools.frontend` maps to `<devtools><frontend>...</frontend></devtools>`.
- Parse XML attributes generically as dotted child keys, for example `<logs enabled="true">logs/app.log</logs>` maps to `logs.enabled` and `logs`.
- Resolve relative paths in startup or usage-side code, not in Config.

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
