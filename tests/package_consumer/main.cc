#include "ui/color.hpp"

int main()
{
    const auto color = rui::Color::fromHex("#2196F3");
    return color.r() == 33 && color.g() == 150 && color.b() == 243 ? 0 : 1;
}
