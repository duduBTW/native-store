#include "layout.cpp"

global_variable StoreState appState = {
    .activePage = PAGE_STORE,
};

void AppUpdateHandler(PlatformState *platformState, AppMemory *Memory)
{
    if (!appState.hasInit)
    {
        appState.hasInit = true;
        appState.globalFont = DrawCreateFont(L"Segoe UI", 18.0f);
    }

    if (!platformState->isWebviewOpen && appState.activePage == PAGE_STORE)
    {
        StartWebView(L"https://store.steampowered.com/?l=portuguese", 0, 0, 48, 0);
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
            .backgroundColor = ColorRGBA(255, 0, 0),
        }))
        {
            TYPOGRAPHY(L"STORE", (TextConfig{
                                     .textColor = ColorRGBA(255, 255, 255),
                                 }));
        }
        DIV((UiElement{
            .backgroundColor = ColorRGBA(0, 255, 0),
        }))
        {
            TYPOGRAPHY(L"LIBRARY",
                       (TextConfig{
                           .textColor = ColorRGBA(255, 255, 255),
                       }));
        }
    }
    DrawEnd();
}