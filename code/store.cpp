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
        .size = {
            .width = FIXED(WindowWidth()),
            .height = FIT(),
        },
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
            .size = {.width = GROW()},
            .backgroundColor = ColorRGBA(255, 0, 0),
        }))
        {
            TYPOGRAPHY(L"STORE", (TextConfig{
                                     .textColor = ColorRGBA(255, 255, 255),
                                     .textFont = gFont,
                                 }));
        }
        DIV((UiElement{
            .size = {.width = FIXED(200)},
            .backgroundColor = ColorRGBA(0, 255, 0),
        }))
        {
            TYPOGRAPHY(L"Lorem ipsum dolor sit amet, consectetur adipiscing elit",
                       (TextConfig{
                           .textColor = ColorRGBA(255, 255, 255),
                           .textFont = gFont,
                       }));
        }
    }
    DrawEnd();
}