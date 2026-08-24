// main.cpp
#include <QDialog>

#include "core/platform_adapter.hpp"
#include "core/nav_controller.hpp"
#include "core/composable.hpp"
#include "core/compose_widget.hpp"
#include "core/effects.hpp"
#include "core/derived_state.hpp"
#include "core/flow.hpp"
#include "core/composition_local.hpp"
#include "config/config.hpp"
#include "ui/text.hpp"
#include "ui/button.hpp"
#include "ui/column.hpp"
#include "ui/color.hpp"
#include "ui/row.hpp"
#include "ui/spacer.hpp"
#include "ui/box.hpp"
#include "ui/image.hpp"
#include "ui/text_field.hpp"
#include "ui/lazy_list.hpp"
#include "ui/scaffold.hpp"
#include "ui/animation.hpp"
#include "ui/snackbar.hpp"
#include "ui/switch.hpp"
#include "ui/checkbox.hpp"
#include "ui/tristate_checkbox.hpp"
#include "ui/radio_button.hpp"
#include "ui/card.hpp"
#include "ui/divider.hpp"
#include "ui/dialog.hpp"
#include "ui/animated_visibility.hpp"
#include "ui/slider.hpp"
#include "ui/progress_indicator.hpp"
#include "ui/chip.hpp"
#include "ui/badge.hpp"
#include "ui/dropdown_menu.hpp"
#include "ui/animated_content.hpp"
#include "ui/surface.hpp"
#include "ui/flow.hpp"

#include "utils/logger.hpp"

using namespace rui;

static Logger::ptr logger = LOG_NAME("App");

static ConfigVar<std::string>::ptr gWorkpath =
    ConfigVar<std::string>::Lookup("workpath", {}, "Application work path");

// 独立设置窗口
Component SettingsWindow() {
    auto titleState = mutableStateOf(std::string("Settings"));
    auto statusState = mutableStateOf(std::string(""));

    return Column(
        Arrangement::Center,
        Text(titleState,
             Modifier().foreground(Color::blue())
                       .fontSize(18_dp)
                       .align(Alignment::Center)),
        Spacer(Modifier().size(0_dp, 12_dp).background(Color::orange())),
        Button(
            [statusState]() {
                statusState->set("Saved!");
            },
            mutableStateOf(std::string("Save Settings")),
            Modifier().background(Color::green(), RoundedCornerShape(8_dp))
                      .foreground(Color::white())
                      .padding(14_dp, 6_dp)
        ),
        Text(statusState,
             Modifier().foreground(Color::grey600())
                       .fontSize(12_dp)
                       .align(Alignment::Center))
    );
}

// 详情界面
Component DetailScreen(std::shared_ptr<NavController> nav) {
    auto titleState = mutableStateOf(std::string("Detail Screen"));
    auto canPopState = mutableStateOf(std::string(
        nav->canPop() ? "← Go Back (pop)" : "Cannot go back"
    ));

    return Column(
        Arrangement::Center,
        Text(titleState,
             Modifier().foreground(Color::blue())
                       .fontSize(20_dp)
                       .align(Alignment::Center)),
        Button(
            [nav]() {
                nav->pop();
            },
            mutableStateOf(std::string("Go Back")),
            Modifier().background(Color::blue(), RoundedCornerShape(8_dp))
                      .foreground(Color::white())
                      .padding(16_dp, 8_dp)
        ),
        Text(canPopState,
             Modifier().foreground(Color::grey600())
                       .fontSize(12_dp)
                       .align(Alignment::Center))
    );
}

// 图片演示界面
Component ImageDemo() {
    std::string path = gWorkpath->get();
    std::string img_path = path + "/tests/avatar.jpg";

    std::string svg_path = path + "/tests/avatar.svg";

    return Column(
        Arrangement::Center,
        Text(mutableStateOf(std::string("Image Demo")),
             Modifier().fontSize(20_dp).foreground(Color::blue()).align(Alignment::Center)),
        Spacer(Modifier().height(20_dp)),
        // 用户卡片：头像 + 名称/角色
        Row(
            Arrangement::Center,
            Image("https://avatars.githubusercontent.com/u/121867895?v=4",
                  Modifier().size(45_dp, 45_dp)
                            .clip(RoundedCornerShape(23_dp))),
            Spacer(Modifier().width(5_dp)),
            Column(
                Arrangement::SpaceBetween,
                Text(mutableStateOf(std::string("Christins Chen")),
                     Modifier().fontSize(18_dp).foreground(Color::grey800())),
                Text(mutableStateOf(std::string("Software Engineer")),
                     Modifier().fontSize(12_dp).foreground(Color::grey500()))
            )
        ),
        Spacer(Modifier().height(16_dp)),
        // ContentScale 演示
        Row(
            Arrangement::SpaceEvenly,
            Column(
                Image(img_path,
                      Modifier().size(60_dp, 60_dp).background(Color::grey200()),
                      Alignment::Center, ContentScale::Fit),
                Text(mutableStateOf(std::string("Fit")),
                     Modifier().fontSize(10_dp).align(Alignment::Center))
            ),
            Column(
                Image(img_path,
                      Modifier().size(60_dp, 60_dp).background(Color::grey200()),
                      Alignment::Center, ContentScale::Crop),
                Text(mutableStateOf(std::string("Crop")),
                     Modifier().fontSize(10_dp).align(Alignment::Center))
            ),
            Column(
                Image(img_path,
                      Modifier().size(60_dp, 60_dp).background(Color::grey200()),
                      Alignment::Center, ContentScale::FillBounds),
                Text(mutableStateOf(std::string("FillBounds")),
                     Modifier().fontSize(10_dp).align(Alignment::Center))
            )
        ),
        Spacer(Modifier().height(12_dp)),
        // alpha + colorFilter 演示
        Row(
            Arrangement::SpaceEvenly,
            Column(
                Image(img_path,
                      Modifier().size(60_dp, 60_dp).alpha(0.5f)),
                Text(mutableStateOf(std::string("alpha 0.5")),
                     Modifier().fontSize(10_dp).align(Alignment::Center))
            ),
            Column(
                Image(img_path,
                      Modifier().size(60_dp, 60_dp),
                      Alignment::Center, ContentScale::Crop, "",
                      ColorFilter{Color::blue()}),
                Text(mutableStateOf(std::string("Tint Blue")),
                     Modifier().fontSize(10_dp).align(Alignment::Center))
            )
        ),
        Spacer(Modifier().height(12_dp)),
        // SVG 图片
        Text(mutableStateOf(std::string("SVG Image")),
             Modifier().fontSize(14_dp).foreground(Color::blue()).align(Alignment::Center)),
        Spacer(Modifier().height(4_dp)),
        Image(svg_path,
              Modifier().size(80_dp, 80_dp).background(Color::grey200()))
    );
}

// TextField 演示界面
Component TextFieldDemo() {
    auto nameState = mutableStateOf(std::string("Alice"));
    auto outputState = mutableStateOf(std::string(""));

    return Column(
        Arrangement::Center,
        Text(mutableStateOf(std::string("TextField Demo")),
             Modifier().fontSize(20_dp).foreground(Color::blue()).align(Alignment::Center)),
        Spacer(Modifier().height(16_dp)),
        Text(mutableStateOf(std::string("Filled TextField:")),
             Modifier().fontSize(12_dp).foreground(Color::grey600())),
        TextField(
            nameState,
            Modifier().fillMaxWidth().padding(16_dp, 4_dp),
            "Enter your name",
            "Name"
        ),
        Spacer(Modifier().height(12_dp)),
        Text(mutableStateOf(std::string("Outlined TextField:")),
             Modifier().fontSize(12_dp).foreground(Color::grey600())),
        OutlinedTextField(
            outputState,
            Modifier().fillMaxWidth().padding(16_dp, 4_dp),
            "Type something...",
            "Output"
        ),
        Spacer(Modifier().height(12_dp)),
        Text(mutableStateOf(std::string("Live preview:")),
             Modifier().fontSize(12_dp).foreground(Color::grey600())),
        Text(nameState, Modifier().fontSize(16_dp).foreground(Color::darkBlue()))
    );
}

// LazyColumn 演示界面
Component LazyListDemo() {
    auto count = 100;
    return LazyColumn(
        count,
        [](int i) -> Component {
            return Text(
                mutableStateOf(std::string("Item #" + std::to_string(i))),
                Modifier().fillMaxWidth()
                          .padding(12_dp, 8_dp)
                          .foreground(i % 2 == 0 ? Color::grey800() : Color::grey600())
                          .background(i % 2 == 0 ? Color::grey100() : Color::white())
            );
        },
        Modifier().fillMaxWidth().fillMaxHeight()
    );
}

// Scaffold 演示界面
Component ScaffoldDemo() {
    auto titleState = mutableStateOf(std::string("Scaffold Demo"));
    auto counter = mutableStateOf("scaffold_counter", 0);
    auto counterText = mutableStateOf("scaffold_text", std::string("FAB counter: 0"));

    return Scaffold(
        Modifier{},
        TopAppBar(titleState),
        Column(
            Arrangement::Center,
            Spacer(Modifier().height(24_dp)),
            Text(mutableStateOf(std::string("Content Area")),
                 Modifier().fontSize(16_dp).foreground(Color::grey600())
                           .align(Alignment::Center)),
            Text(counterText,
                 Modifier().fontSize(14_dp).foreground(Color::grey500())
                           .align(Alignment::Center)),
            Spacer(Modifier().weight(1))
        ),
        nullptr,
        FloatingActionButton(
            [counter, counterText]() {
                counter->set(counter->get() + 1);
                counterText->set("FAB counter: " + std::to_string(counter->get()));
            },
            Text(mutableStateOf(std::string("+")),
                 Modifier().fontSize(24_dp).foreground(Color::white())
                           .align(Alignment::Center))
        )
    );
}

// 新组件演示界面
Component NewComponentsDemo() {
    auto switchState = mutableStateOf(false);
    auto checkState = mutableStateOf(true);
    auto radioState = mutableStateOf(false);
    auto triState = mutableStateOf(TriState::Indeterminate);
    auto visibleState = mutableStateOf(true);
    auto targetState = mutableStateOf(0.0f);

    auto snackbarHost = std::make_shared<SnackbarHostState>();

    return Scaffold(
        Modifier{},
        nullptr,
        Column(
            Arrangement::Start,
            Text(mutableStateOf(std::string("New Components")),
                 Modifier().fontSize(20_dp).foreground(Color::blue()).align(Alignment::Center)),
            Spacer(Modifier().height(16_dp)),

            Row(
                Text(mutableStateOf(std::string("Switch")), Modifier().padding(4_dp)),
                Spacer(Modifier().width(8_dp)),
                Switch(switchState)
            ),
            Row(
                Text(mutableStateOf(std::string("Checkbox")), Modifier().padding(4_dp)),
                Spacer(Modifier().width(8_dp)),
                Checkbox(checkState)
            ),
            Row(
                Text(mutableStateOf(std::string("RadioButton")), Modifier().padding(4_dp)),
                Spacer(Modifier().width(8_dp)),
                RadioButton(radioState)
            ),
            Row(
                Text(mutableStateOf(std::string("TriStateCheckbox")), Modifier().padding(4_dp)),
                Spacer(Modifier().width(8_dp)),
                TriStateCheckbox(triState)
            ),
            Spacer(Modifier().height(8_dp)),
            Divider(),
            Spacer(Modifier().height(8_dp)),

            Card(
                Column(
                    Text(mutableStateOf(std::string("Card")),
                         Modifier().fontSize(16_dp).foreground(Color::grey800())),
                    Spacer(Modifier().height(4_dp)),
                    Text(mutableStateOf(std::string("A simple card body.")),
                         Modifier().fontSize(12_dp).foreground(Color::grey600()))
                ),
                Modifier().fillMaxWidth().padding(12_dp)
            ),
            Spacer(Modifier().height(8_dp)),

            Row(
                Button(
                    [visibleState]() { visibleState->set(!visibleState->get()); },
                    mutableStateOf(std::string("Toggle Visibility")),
                    Modifier().background(Color::blue(), RoundedCornerShape(8_dp))
                              .foreground(Color::white())
                              .padding(12_dp, 6_dp)
                ),
                Spacer(Modifier().width(8_dp)),
                AnimatedVisibility(
                    visibleState,
                    Text(mutableStateOf(std::string("Fading")),
                         Modifier().foreground(Color::green()))
                )
            ),
            Spacer(Modifier().height(8_dp)),

            compose([targetState]() -> Component {
                observe(targetState);
                auto animated = animateFloatAsState(targetState->get(), AnimationSpec::tween(600));
                observe(animated);
                int pct = static_cast<int>(animated->get() * 100.0f);

                return Column(
                    Button(
                        [targetState]() { targetState->set(targetState->get() > 0.5f ? 0.0f : 1.0f); },
                        mutableStateOf(std::string("Animate Progress")),
                        Modifier().background(Color::green(), RoundedCornerShape(8_dp))
                                  .foreground(Color::white())
                                  .padding(12_dp, 6_dp)
                    ),
                    Spacer(Modifier().height(4_dp)),
                    Text(mutableStateOf(std::string("progress: " + std::to_string(pct) + "%")),
                         Modifier().fontSize(12_dp).foreground(Color::grey600()))
                );
            }),
            Spacer(Modifier().height(8_dp)),

            Row(
                Button(
                    [snackbarHost]() { snackbarHost->showSnackbar("Hello Snackbar!"); },
                    mutableStateOf(std::string("Show Snackbar")),
                    Modifier().background(Color::darkBlue(), RoundedCornerShape(8_dp))
                              .foreground(Color::white())
                              .padding(12_dp, 6_dp)
                ),
                Spacer(Modifier().width(8_dp)),
                Button(
                    []() {
                        auto dlg = AlertDialog(
                            nullptr,
                            nullptr,
                            nullptr,
                            "AlertDialog",
                            "This is a modal dialog."
                        );
                        if (auto* d = qobject_cast<QDialog*>(dlg.get())) {
                            d->exec();
                        }
                    },
                    mutableStateOf(std::string("Show Dialog")),
                    Modifier().background(Color::orange(), RoundedCornerShape(8_dp))
                              .foreground(Color::white())
                              .padding(12_dp, 6_dp)
                )
            )
        ),
        nullptr,
        nullptr,
        SnackbarHost(snackbarHost)
    );
}

// P2 组件演示界面
Component P2ComponentsDemo() {
    auto sliderValue = mutableStateOf(0.3f);
    auto chipSelected = mutableStateOf(false);
    auto closeChipVisible = mutableStateOf(true);
    auto badgeCount = mutableStateOf(std::string("3"));
    auto menuExpanded = mutableStateOf(false);
    auto menuResult = mutableStateOf(std::string(""));
    auto colorTarget = mutableStateOf(false);

    std::vector<DropdownMenuItem> items = {
        {"Copy", [menuResult]() { menuResult->set("Copy clicked"); }},
        {"Rename", [menuResult]() { menuResult->set("Rename clicked"); }},
        {"Delete", [menuResult]() { menuResult->set("Delete clicked"); }},
    };

    return Column(
        Arrangement::Start,
        Text(mutableStateOf(std::string("P2 Components")),
             Modifier().fontSize(20_dp).foreground(Color::blue()).align(Alignment::Center)),
        Spacer(Modifier().height(16_dp)),

        // Surface + FlowRow
        Surface(
            Column(
                Text(mutableStateOf(std::string("Surface")),
                     Modifier().fontSize(14_dp).foreground(Color::grey800())),
                Spacer(Modifier().height(4_dp)),
                FlowRow(
                    Chip(mutableStateOf(std::string("Flow")), []() {}, Modifier().padding(2_dp)),
                    Chip(mutableStateOf(std::string("Wrap")), []() {}, Modifier().padding(2_dp)),
                    Chip(mutableStateOf(std::string("Layout")), []() {}, Modifier().padding(2_dp)),
                    Chip(mutableStateOf(std::string("Items")), []() {}, Modifier().padding(2_dp))
                )
            ),
            Modifier().fillMaxWidth().padding(12_dp),
            Color::grey50(),
            RoundedCornerShape(8_dp),
            1.0f
        ),
        Spacer(Modifier().height(12_dp)),

        // Slider + Progress
        Text(mutableStateOf(std::string("Slider")), Modifier().fontSize(12_dp).foreground(Color::grey600())),
        Slider(sliderValue, nullptr, Modifier().fillMaxWidth()),
        Spacer(Modifier().height(8_dp)),
        LinearProgressIndicator(sliderValue, Modifier().fillMaxWidth().padding(16_dp, 0_dp)),
        Spacer(Modifier().height(8_dp)),
        Row(
            CircularProgressIndicator(sliderValue),
            Spacer(Modifier().width(12_dp)),
            Text(mutableStateOf(std::string("Circular progress follows slider")),
                 Modifier().fontSize(12_dp).foreground(Color::grey600()))
        ),
        Spacer(Modifier().height(12_dp)),

        // 颜色动画（animateColorAsState）
        compose([colorTarget]() -> Component {
            observe(colorTarget);
            auto animated = animateColorAsState(colorTarget->get() ? Color::blue() : Color::orange());
            observe(animated);
            return Button(
                [colorTarget]() { colorTarget->set(!colorTarget->get()); },
                mutableStateOf(std::string("Animate Color")),
                Modifier().background(animated->get(), RoundedCornerShape(8_dp))
                          .foreground(Color::white())
                          .padding(12_dp, 6_dp)
            );
        }),
        Spacer(Modifier().height(12_dp)),
        Divider(),
        Spacer(Modifier().height(12_dp)),

        // Chips
        Row(
            Chip(mutableStateOf(std::string("Assist Chip")),
                 []() {}, Modifier().padding(2_dp)),
            compose([closeChipVisible]() -> Component {
                observe(closeChipVisible);
                if (!closeChipVisible->get()) {
                    return nullptr;
                }
                return Row(
                    Spacer(Modifier().width(8_dp)),
                    Chip(mutableStateOf(std::string("Close me")),
                         nullptr, Modifier().padding(2_dp),
                         [closeChipVisible]() { closeChipVisible->set(false); })
                );
            }),
            Spacer(Modifier().width(8_dp)),
            FilterChip(chipSelected,
                       mutableStateOf(std::string("FilterChip")),
                       [chipSelected]() { chipSelected->set(!chipSelected->get()); },
                       Modifier().padding(2_dp))
        ),
        Spacer(Modifier().height(12_dp)),

        // Badge + BadgedBox
        Row(
            Badge(badgeCount),
            Spacer(Modifier().width(12_dp)),
            BadgedBox(
                Badge(badgeCount),
                Modifier().height(40_dp),
                Text(mutableStateOf(std::string("Inbox")), Modifier().padding(8_dp))
            )
        ),
        Spacer(Modifier().height(12_dp)),

        // DropdownMenu
        Row(
            Button(
                [menuExpanded]() { menuExpanded->set(true); },
                mutableStateOf(std::string("Open Menu")),
                Modifier().background(Color::blue(), RoundedCornerShape(8_dp))
                          .foreground(Color::white())
                          .padding(12_dp, 6_dp)
            ),
            DropdownMenu(menuExpanded, nullptr, items)
        ),
        Spacer(Modifier().height(4_dp)),
        Text(menuResult, Modifier().fontSize(12_dp).foreground(Color::grey600()))
    );
}

// 动效演示界面
Component AnimationDemo() {
    auto sizeTarget = mutableStateOf(false);
    auto transitionTarget = mutableStateOf(0);
    auto contentTarget = mutableStateOf(0);

    return Column(
        Arrangement::Start,
        Text(mutableStateOf(std::string("Animation Demo")),
             Modifier().fontSize(20_dp).foreground(Color::blue()).align(Alignment::Center)),
        Spacer(Modifier().height(16_dp)),

        // animateSizeAsState
        compose([sizeTarget]() -> Component {
            observe(sizeTarget);
            auto size = animateSizeAsState(
                sizeTarget->get() ? Size{120_dp, 40_dp} : Size{60_dp, 20_dp},
                AnimationSpec::tween(400));
            observe(size);
            return Button(
                [sizeTarget]() { sizeTarget->set(!sizeTarget->get()); },
                mutableStateOf(std::string("Animate Size")),
                Modifier().size(size->get().width, size->get().height)
                          .background(Color::blue(), RoundedCornerShape(8_dp))
                          .foreground(Color::white())
            );
        }),
        Spacer(Modifier().height(12_dp)),

        // updateTransition
        compose([transitionTarget]() -> Component {
            observe(transitionTarget);
            auto transition = updateTransition(transitionTarget);
            auto color = transition->animateColor(
                [](int t) { return t == 0 ? Color::blue() : Color::orange(); });
            observe(color);
            return Button(
                [transitionTarget]() { transitionTarget->set(transitionTarget->get() == 0 ? 1 : 0); },
                mutableStateOf(std::string("UpdateTransition")),
                Modifier().background(color->get(), RoundedCornerShape(8_dp))
                          .foreground(Color::white())
                          .padding(12_dp, 6_dp)
            );
        }),
        Spacer(Modifier().height(12_dp)),

        // rememberInfiniteTransition
        compose([]() -> Component {
            auto infinite = rememberInfiniteTransition();
            auto alpha = infinite->animateFloat(0.2f, 1.0f, AnimationSpec::tween(800));
            observe(alpha);
            return Text(
                mutableStateOf(std::string("Pulsing")),
                Modifier().fontSize(18_dp)
                          .foreground(Color::green())
                          .alpha(alpha->get())
                          .align(Alignment::Center)
            );
        }),
        Spacer(Modifier().height(12_dp)),

        // AnimatedContent
        Row(
            Button(
                [contentTarget]() { contentTarget->set(contentTarget->get() == 0 ? 1 : 0); },
                mutableStateOf(std::string("Switch")),
                Modifier().background(Color::green(), RoundedCornerShape(8_dp))
                          .foreground(Color::white())
                          .padding(12_dp, 6_dp)
            ),
            Spacer(Modifier().width(12_dp)),
            AnimatedContent(
                contentTarget,
                [](int t) -> Component {
                    return Text(
                        mutableStateOf(std::string(t == 0 ? "Content A" : "Content B")),
                        Modifier().fontSize(14_dp)
                                  .foreground(t == 0 ? Color::blue() : Color::red())
                    );
                },
                Modifier().height(30_dp)
            )
        )
    );
}

// 基础能力演示界面
Component FoundationDemo() {
    auto name = mutableStateOf(std::string("Alice"));
    auto keyTarget = mutableStateOf(false);
    auto sizeInfo = mutableStateOf(std::string("size: -"));
    auto hoverInfo = mutableStateOf(std::string("hover: false"));
    auto clickInfo = mutableStateOf(std::string("click: -"));

    return Column(
        Arrangement::Start,
        Text(mutableStateOf(std::string("Foundation Demo")),
             Modifier().fontSize(20_dp).foreground(Color::blue()).align(Alignment::Center)),
        Spacer(Modifier().height(12_dp)),

        // derivedStateOf
        compose([name]() -> Component {
            auto greeting = derivedStateOf<std::string>([name]() {
                return "Hello, " + name->get();
            });
            observe(greeting);
            return Column(
                Text(greeting, Modifier().fontSize(16_dp).foreground(Color::darkBlue())),
                Row(
                    Button([name]() { name->set("Bob"); },
                           mutableStateOf(std::string("Change name")),
                           Modifier().background(Color::blue(), RoundedCornerShape(6_dp))
                                     .foreground(Color::white())
                                     .padding(10_dp, 4_dp)),
                    Spacer(Modifier().width(8_dp)),
                    Button([name]() { name->set("Alice"); },
                           mutableStateOf(std::string("Reset")),
                           Modifier().background(Color::grey500(), RoundedCornerShape(6_dp))
                                     .foreground(Color::white())
                                     .padding(10_dp, 4_dp))
                )
            );
        }),
        Spacer(Modifier().height(12_dp)),

        // keyed remember: value persists across recomposition
        compose([keyTarget]() -> Component {
            observe(keyTarget);
            auto& count = remember(std::string("foundation_keyed"), 0);
            count++;
            return Text(mutableStateOf(std::string("keyed remember count: " + std::to_string(count))),
                        Modifier().fontSize(12_dp).foreground(Color::grey600()));
        }),
        Spacer(Modifier().height(12_dp)),

        // graphicsLayer + shadow coexist
        Text(mutableStateOf(std::string("graphicsLayer + shadow")),
             Modifier().fontSize(12_dp).foreground(Color::grey600())),
        Box(
            Modifier().size(48_dp, 48_dp)
                      .background(Color::blue(), RoundedCornerShape(6_dp))
                      .shadow(6_dp, RoundedCornerShape(6_dp))
                      .graphicsLayer([](GraphicsLayerScope& g) {
                          g.rotationZ = 40.0f;
                          g.scaleX = 1.0f;
                          g.scaleY = 1.0f;
                          g.alpha = 1.0f;
                      })
        ),
        Spacer(Modifier().height(12_dp)),

        // produceState
        compose([]() -> Component {
            auto asyncText = produceState(std::string("loading..."), 0,
                [](std::shared_ptr<MutableState<std::string>> state) -> Job {
                    co_await delay(800);
                    state->set("produceState done (800ms)");
                });
            return Text(asyncText, Modifier().fontSize(14_dp).foreground(Color::green()));
        }),
        Spacer(Modifier().height(12_dp)),

        // snapshotFlow
        compose([name]() -> Component {
            auto& lengthState = remember(std::string("snapshot_len_state"),
                                         std::shared_ptr<MutableState<std::string>>(nullptr));
            if (!lengthState) {
                lengthState = mutableStateOf(std::string(""));
                snapshotFlow<int>([name]() { return static_cast<int>(name->get().size()); })
                    ->collect([lengthState](int len) {
                        lengthState->set("name length: " + std::to_string(len));
                    });
            }
            observe(lengthState);
            return Text(lengthState, Modifier().fontSize(14_dp).foreground(Color::blue()));
        }),
        Spacer(Modifier().height(12_dp)),

        // keyframes
        compose([keyTarget]() -> Component {
            observe(keyTarget);
            auto color = animateKeyframesAsState<Color>(
                keyTarget->get()
                    ? std::vector<std::pair<float, Color>>{{0.0f, Color::blue()}, {0.5f, Color::red()}, {1.0f, Color::orange()}}
                    : std::vector<std::pair<float, Color>>{{0.0f, Color::orange()}, {0.5f, Color::green()}, {1.0f, Color::blue()}},
                600);
            observe(color);
            return Button(
                [keyTarget]() { keyTarget->set(!keyTarget->get()); },
                mutableStateOf(std::string("Keyframes Color")),
                Modifier().background(color->get(), RoundedCornerShape(8_dp))
                          .foreground(Color::white())
                          .padding(12_dp, 6_dp)
            );
        }),
        Spacer(Modifier().height(12_dp)),

        // 新 Modifier：aspectRatio / onSizeChanged / focusable / combinedClickable
        Text(mutableStateOf(std::string("New Modifiers")),
             Modifier().fontSize(12_dp).foreground(Color::grey600())),
        Row(
            Column(
                Box(
                    Modifier().width(70_dp).aspectRatio(2.0f)
                              .background(Color::lightBlue(), RoundedCornerShape(4_dp))
                              .onSizeChanged([sizeInfo](Size s) {
                                  sizeInfo->set("size: " + std::to_string(s.width.value()) +
                                                "x" + std::to_string(s.height.value()));
                              })
                ),
                Text(sizeInfo, Modifier().fontSize(10_dp).foreground(Color::grey600()))
            ),
            Spacer(Modifier().width(10_dp)),
            Column(
                Text(mutableStateOf(std::string("Hover me")),
                     Modifier().padding(8_dp).fontSize(12_dp)
                               .background(Color::grey200(), RoundedCornerShape(4_dp))
                               .hoverable([hoverInfo](bool h) {
                                   hoverInfo->set(h ? "hover: true" : "hover: false");
                               })),
                Text(hoverInfo, Modifier().fontSize(10_dp).foreground(Color::grey600()))
            ),
            Spacer(Modifier().width(10_dp)),
            Column(
                Text(mutableStateOf(std::string("Long/click")),
                     Modifier().padding(8_dp).fontSize(12_dp)
                               .background(Color::grey200(), RoundedCornerShape(4_dp))
                               .combinedClickable(
                                   [clickInfo]() { clickInfo->set("long pressed"); },
                                   [clickInfo]() { clickInfo->set("clicked"); })),
                Text(clickInfo, Modifier().fontSize(10_dp).foreground(Color::grey600()))
            )
        ),
        Spacer(Modifier().height(12_dp)),

        // CompositionLocal / LocalDensity
        CompositionLocalProvider(LocalDensity, Density{2.0f, 1.0f},
            []() -> Component {
                auto density = LocalDensity.current();
                return Text(mutableStateOf(std::string(
                                "LocalDensity: 20dp -> " + std::to_string(density.dpToPx(20)) + "px")),
                            Modifier().fontSize(13_dp).foreground(Color(156, 39, 176)));
            })
    );
}

// 主页界面
Component HomeScreen(std::shared_ptr<NavController> nav) {
    auto counter    = mutableStateOf("counter", 0);
    auto counterText = mutableStateOf("countText", std::string("Count: 0"));

    auto onIncrement = [counter, counterText]() {
        counter->set(counter->get() + 1);
        counterText->set("Count: " + std::to_string(counter->get()));
        INFO(logger) << "Counter: " << counter->get();
    };

    struct Item {
        enum Type { Title, Button, Row, Text, Box, Compose };
        Type type;
        int navIdx = -1;
    };

    std::vector<Item> items = {
        {Item::Title},         // Title
        {Item::Button, 0},     // Push Detail
        {Item::Button, 1},     // Replace Detail
        {Item::Button, 2},     // Image Demo
        {Item::Button, 3},     // TextField Demo
        {Item::Button, 4},     // LazyColumn Demo
        {Item::Button, 5},     // Open Settings Window
        {Item::Button, 6},     // Click me
        {Item::Button, 7},     // Scaffold Demo
        {Item::Button, 8},     // New Components Demo
        {Item::Button, 9},     // P2 Components Demo
        {Item::Button, 10},    // Animation Demo
        {Item::Button, 11},    // Foundation Demo
        {Item::Row, -1},       // Spacer Row
        {Item::Text, -1},      // Text clickable
        {Item::Text, -2},      // Counter text
        {Item::Box, -1},       // Box demo
        {Item::Compose, -1},   // Compose demo
    };

    return LazyColumn(
        static_cast<int>(items.size()),
        [nav, counter, counterText, onIncrement, items](int i) -> Component {
            switch (items[i].type) {
            case Item::Title:
                return Text(mutableStateOf(std::string("Home Screen")),
                            Modifier().foreground(Color::blue())
                                      .fontSize(20_dp)
                                      .align(Alignment::Center));
            case Item::Button: {
                switch (items[i].navIdx) {
                case 0:
                    return Button(
                        [nav]() { nav->push(DetailScreen(nav)); },
                        mutableStateOf(std::string("Push Detail (保留当前)")),
                        Modifier().fillMaxWidth()
                                  .background(Color::blue(), RoundedCornerShape(8_dp))
                                  .foreground(Color::white())
                                  .padding(16_dp, 8_dp)
                    );
                case 1:
                    return Button(
                        [nav]() { nav->replace(DetailScreen(nav)); },
                        mutableStateOf(std::string("Replace Detail (替换当前)")),
                        Modifier().fillMaxWidth()
                                  .background(Color::darkBlue(), RoundedCornerShape(8_dp))
                                  .foreground(Color::white())
                                  .padding(16_dp, 8_dp)
                    );
                case 2:
                    return Button(
                        [nav]() { nav->push(ImageDemo()); },
                        mutableStateOf(std::string("Open Image Demo")),
                        Modifier().fillMaxWidth()
                                  .background(Color::green(), RoundedCornerShape(8_dp))
                                  .foreground(Color::white())
                                  .padding(16_dp, 8_dp)
                    );
                case 3:
                    return Button(
                        [nav]() { nav->push(TextFieldDemo()); },
                        mutableStateOf(std::string("Open TextField Demo")),
                        Modifier().fillMaxWidth()
                                  .background(Color::blue(), RoundedCornerShape(8_dp))
                                  .foreground(Color::white())
                                  .padding(16_dp, 8_dp)
                    );
                case 4:
                    return Button(
                        [nav]() { nav->push(LazyListDemo()); },
                        mutableStateOf(std::string("Open LazyColumn Demo")),
                        Modifier().fillMaxWidth()
                                  .background(Color::lightBlue(), RoundedCornerShape(8_dp))
                                  .foreground(Color::white())
                                  .padding(16_dp, 8_dp)
                    );
                case 5:
                    return Button(
                        [nav]() {
                            nav->openWindow(SettingsWindow(),
                                            WindowConfig{
                                                .title = "Settings",
                                                .key = "settings",
                                                .width = 300_dp,
                                                .height = 200_dp,
                                                .minWidth = 200_dp,
                                                .minHeight = 150_dp
                                            });
                        },
                        mutableStateOf(std::string("Open Settings Window")),
                        Modifier().fillMaxWidth()
                                  .background(Color::lightBlue(), RoundedCornerShape(8_dp))
                                  .foreground(Color::white())
                                  .padding(16_dp, 8_dp)
                    );
                case 7:
                    return Button(
                        [nav]() { nav->push(ScaffoldDemo()); },
                        mutableStateOf(std::string("Open Scaffold Demo")),
                        Modifier().fillMaxWidth()
                                  .background(Color::darkBlue(), RoundedCornerShape(8_dp))
                                  .foreground(Color::white())
                                  .padding(16_dp, 8_dp)
                    );
                case 8:
                    return Button(
                        [nav]() { nav->push(NewComponentsDemo()); },
                        mutableStateOf(std::string("Open New Components Demo")),
                        Modifier().fillMaxWidth()
                                  .background(Color::orange(), RoundedCornerShape(8_dp))
                                  .foreground(Color::white())
                                  .padding(16_dp, 8_dp)
                    );
                case 9:
                    return Button(
                        [nav]() { nav->push(P2ComponentsDemo()); },
                        mutableStateOf(std::string("Open P2 Components Demo")),
                        Modifier().fillMaxWidth()
                                  .background(Color::green(), RoundedCornerShape(8_dp))
                                  .foreground(Color::white())
                                  .padding(16_dp, 8_dp)
                    );
                case 10:
                    return Button(
                        [nav]() { nav->push(AnimationDemo()); },
                        mutableStateOf(std::string("Open Animation Demo")),
                        Modifier().fillMaxWidth()
                                  .background(Color::darkOrange(), RoundedCornerShape(8_dp))
                                  .foreground(Color::white())
                                  .padding(16_dp, 8_dp)
                    );
                case 11:
                    return Button(
                        [nav]() { nav->push(FoundationDemo()); },
                        mutableStateOf(std::string("Open Foundation Demo")),
                        Modifier().fillMaxWidth()
                                  .background(Color(156, 39, 176), RoundedCornerShape(8_dp))
                                  .foreground(Color::white())
                                  .padding(16_dp, 8_dp)
                    );
                case 6:
                    return Button(
                        onIncrement,
                        Text(mutableStateOf(std::string("Click me")),
                             Modifier().foreground(Color::red())),
                        Modifier().fillMaxWidth()
                                  .background(Color::blue(), RoundedCornerShape(8_dp))
                                  .padding(12_dp, 4_dp)
                    );
                default:
                    return nullptr;
                }
            }
            case Item::Row:
                return Row(
                    Spacer(Modifier().height(8_dp)
                                      .fillMaxWidth()
                                      .weight(1)
                                      .background(Color::green(), RoundedCornerShape(4_dp))
                                      .clip(RoundedCornerShape(4_dp))),
                    Spacer(Modifier().width(8_dp)),
                    Spacer(Modifier().height(8_dp)
                                      .fillMaxWidth()
                                      .weight(2)
                                      .background(Color::orange(), RoundedCornerShape(4_dp))
                                      .clip(RoundedCornerShape(4_dp)))
                );
            case Item::Text: {
                if (items[i].navIdx == -2) {
                    return Text(counterText,
                         Modifier().foreground(Color::grey600())
                                   .align(Alignment::Center));
                }
                return Text(mutableStateOf(std::string("Text clickable")),
                     Modifier().padding(8_dp)
                               .background(Color::grey200(), RoundedCornerShape(6_dp))
                               .clickable(onIncrement));
            }
            case Item::Box:
                return Box(
                    Modifier().fillMaxWidth().height(80_dp).background(Color::grey100(), RoundedCornerShape(6_dp)),
                    Text(mutableStateOf(std::string("Behind")),
                         Modifier().fontSize(24_dp)
                                   .align(Alignment::Center)
                                   .foreground(Color::grey300())),
                    Text(mutableStateOf(std::string("Front")),
                         Modifier().fontSize(16_dp)
                                   .align(Alignment::BottomEnd)
                                   .foreground(Color::blue())
                                   .padding(8_dp))
                );
            case Item::Compose:
                return compose([counter]() -> Component {
                    observe(counter);
                    auto& recomposeCount = remember(-1);
                    recomposeCount++;

                    return Column(
                        Text(mutableStateOf(std::string(
                            "Recompose #" + std::to_string(recomposeCount))),
                             Modifier().foreground(Color::grey500())
                                       .fontSize(11_dp)
                                       .align(Alignment::Center)),
                        Text(mutableStateOf(std::string(
                            "Counter = " + std::to_string(counter->get()))),
                             Modifier().foreground(Color::grey500())
                                       .fontSize(11_dp)
                                       .align(Alignment::Center))
                    );
                });
            }
            return nullptr;
        },
        Modifier().fillMaxWidth().fillMaxHeight()
    );
}

int main(int argc, char* argv[]) {
    auto platform = PlatformAdapter::create();
    platform->initialize(argc, argv);

    platform->setWindowConfig(WindowConfig{
        .title = "RUI Navigation Demo",
        .width = 450_dp,
        .height = 500_dp,
        .minWidth = 300_dp,
        .minHeight = 300_dp
    });

    platform->run([]() {
        // 创建导航控制器并设置初始界面
        auto nav = std::make_shared<NavController>();
        nav->push(HomeScreen(nav));

        auto* layout = PlatformAdapter::currentLayout();
        if (layout) {
            layout->addWidget(nav->container());
        }
    });

    return 0;
}
