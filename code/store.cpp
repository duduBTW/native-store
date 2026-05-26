#include "store.h"

global_variable StoreState appState = {
    .activePage = PAGE_STORE,
};

global_variable PlatformState *platformState;

#include "layout.cpp"

#define NavItemColor(isActive) (isActive) ? ColorRGBA(255, 255, 255) : ColorRGBA(214, 214, 214)

void NAV()
{
    DIV((UiElement{
        .size = {
            .width = GROW(),
        },
        .padding = {
            .top = 4,
            .right = 4,
            .bottom = 4,
            .left = 4,
        },
        .gap = 12,
    }))
    {
        DIV((UiElement{
            .id = L"store",
        }))
        {
            TYPOGRAPHY(L"STORE",
                       (TextConfig{
                           .textColor = NavItemColor(appState.activePage == PAGE_STORE),
                       }));
        }
        DIV((UiElement{
            .id = L"library",
        }))
        {
            TYPOGRAPHY(L"LIBRARY",
                       (TextConfig{
                           .textColor = NavItemColor(appState.activePage == PAGE_LIBRARY),
                       }));
        }
    }
}

Game games[256] = {
    Game{
        .Name = L"Deadlock",
        .Image = {
            .Icon = L"W:\\native-store\\assets\\deadlock-icon.jpg",
            .LibraryHero = L"W:\\native-store\\assets\\deadlock_library_hero.jpg",
        },
    },
    Game{
        .Name = L"Counter-Strike 2",
        .Image = {
            .Icon = L"W:\\native-store\\assets\\cs2-icon.jpg",
            .LibraryHero = L"W:\\native-store\\assets\\cs2-library_hero.jpg",
        },
    },
};

void LibraryPage()
{
    DIV((UiElement{
        // todo: check why width fixed doesn't work here.
        .size = {.width = GROW(), .height = GROW()},
        .padding = {
            .top = 8,
        },
    }))
    {
        DIV((UiElement{
            .size = {.width = FIXED(240), .height = GROW()},
            .padding = {
                .top = 16,
            },
            .backgroundColor = ColorRGBA(36, 40, 47),
            .gap = 2,
            .direction = COLUMN,
        }))
        {
            for (size_t i = 0; i < 2; i++)
            {
                Game game = games[i];
                bool isSelected = appState.selectedGame && game.Name == appState.selectedGame->Name;
                DIV((UiElement{
                    .id = game.Name,
                    .size = {.width = GROW()},
                    .padding = {
                        .top = 4,
                        .right = 12,
                        .bottom = 4,
                        .left = 12,
                    },
                    .backgroundColor = isSelected ? ColorRGBA(62, 78, 100) : ColorRGBA(36, 40, 47),
                    .gap = 4,
                    .crossaxisAlignment = ALIGNMENT_CENTER,
                }))
                {
                    IMAGE(game.Image.Icon, (ImageConfig({
                                               .width = FIXED(32),
                                               .height = FIXED(32),
                                           })));
                    TYPOGRAPHY(game.Name, (TextConfig{
                                              .textColor = ColorRGBA(255, 255, 255),
                                          }));
                }
            }
        }

        DIV((UiElement{
            .size = {.width = GROW(), .height = GROW()},
            .gap = 12,
            .direction = COLUMN,
        }))
        {
            if (appState.selectedGame && appState.selectedGame->Name)
            {
                Game game = *appState.selectedGame;
                IMAGE(game.Image.LibraryHero, (ImageConfig({
                                                  .width = GROW(),
                                              })));
                TYPOGRAPHY(game.Name, (TextConfig{
                                          .textColor = ColorRGBA(255, 255, 255),
                                      }))
            }
        }
    }
}

void AppUpdateHandler(PlatformState *_platformState, AppMemory *Memory)
{
    platformState = _platformState;
    if (!appState.hasInit)
    {
        appState.hasInit = true;
        appState.globalFont = DrawCreateFont(L"Segoe UI", 18.0f);
    }

    if (!platformState->isWebviewOpen && appState.activePage == PAGE_STORE)
    {
        StartWebView(L"https://store.steampowered.com/?l=portuguese", 0, 0, 52, 0);
    }

    DrawBegin(ColorRGBA(30, 30, 30));
    DIV((UiElement{
        .size = {
            .width = FIXED(WindowWidth()),
            .height = FIXED(WindowHeight())},
        .padding = {
            .top = 10,
            .right = 10,
            .bottom = 10,
            .left = 10,
        },
        .direction = COLUMN,
    }))
    {
        NAV();

        if (appState.activePage == PAGE_LIBRARY)
        {
            LibraryPage();
        }
    }
    DrawEnd();
}

void Events()
{
    if (appState.activePage != PAGE_LIBRARY &&
        PointerOver(L"library") && platformState->isClicked)
    {
        appState.activePage = PAGE_LIBRARY;

        if (platformState->isWebviewOpen)
        {
            DestroyWebView();
        }
    }

    if (PointerOver(L"store") && platformState->isClicked)
    {
        appState.activePage = PAGE_STORE;
    }

    for (size_t i = 0; i < 2; i++)
    {
        Game *game = &games[i];

        if (PointerOver(game->Name) && platformState->isClicked)
        {
            appState.selectedGame = game;
        }
    }
}
