#include "store.h"
#include "layout.cpp"

bool hasInit = false;
platform_font *gFont;

void AppUpdateHandler(app_memory *Memory)
{
  if (!hasInit)
  {
    hasInit = true;
    gFont = DrawCreateFont(L"Segoe UI", 18.0f);
    // StartWebView(L"http://example.com/", 0, 0, 40, 0);
  }

  DrawBegin(ColorRGBA(30, 30, 30));
  UiElement el = {
      .position = {.x = 100, .y = 20},
      // .size = {.width = 960, .height = 540},
      .padding = {.top = 10, .left = 20},
      .backgroundColor = ColorRGBA(3, 115, 252),
      .gap = 12,
  };
  UiElement el2 = {
      .size = {.width = 100, .height = 100},
      .backgroundColor = ColorRGBA(255, 0, 0),
  };
  ELEMENT(el)
  {
    ELEMENT(el2) {}
    ELEMENT(el2) {}
  }
  Render();
  // UiElement layout = {
  // .position = {.x = 100, .y = 20},
  // .size = {.width = 960, .height = 540},
  // .padding = {.top = 10, .left = 20},
  // .backgroundColor = ColorRGBA(3, 115, 252),
  // .gap = 12,
  //     .children = {
  //         UiElement{
  //             .size = {.width = 100, .height = 100},
  //             .backgroundColor = ColorRGBA(255, 0, 0)},
  //         UiElement{
  //             .size = {.width = 200, .height = 50},
  //             .backgroundColor = ColorRGBA(0, 255, 0)}}};
  // DrawLayout(layout);
  DrawEnd();
}