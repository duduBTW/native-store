#include "layout.cpp"

bool hasInit = false;
platform_font *gFont;

void AppUpdateHandler(app_memory *Memory)
{
  if (!hasInit)
  {
    hasInit = true;
    gFont = DrawCreateFont(L"Segoe UI", 18.0f);
    // StartWebView(L"https://store.steampowered.com/?l=portuguese", 0, 0, 48, 0);
  }

  DrawBegin(ColorRGBA(30, 30, 30));
  DIV((UiElement{
      .size = {.width = FIXED(WindowWidth()), .height = FIT()},
      .padding = {
          .top = 4,
          .right = 4,
          .bottom = 4,
          .left = 4,
      },
      .backgroundColor = ColorRGBA(3, 115, 252),
      .gap = 12,
  }))
  {
    DIV((UiElement{
        .size = {.width = FIXED(100), .height = FIXED(40)},
        .backgroundColor = ColorRGBA(255, 0, 0),
    }))
    {
      // TYPOGRAPHY("pog", TextConfig{});
    }
    DIV((UiElement{
        .size = {.width = FIXED(100), .height = FIXED(40)},
        .backgroundColor = ColorRGBA(0, 255, 0),
    }))
    {
    }
  }
  // text_metrics metrics = MeasureText(gFont, L"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nam quis efficitur neque, sit amet ultricies odio. Pellentesque elementum erat nulla, eget condimentum leo rutrum ac. ", 100);
  // DrawText(gFont, L"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nam quis efficitur neque, sit amet ultricies odio. Pellentesque elementum erat nulla, eget condimentum leo rutrum ac. ", 40, 40, 100, Color(1, 1, 1), TextAlign_Left, TextVAlign_Top);
  DrawEnd();
}