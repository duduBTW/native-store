#include <stdint.h>
#include <vector>

// TODO(Carlos): Remove on prod builds.
#define Assert(Expression) \
  if (!(Expression))       \
  {                        \
    *(int *)0 = 0;         \
  }

#define Kilobytes(Value) ((Value) * 1024)
#define Megabytes(Value) (Kilobytes(Value) * 1024)
#define Gigabytes(Value) (Megabytes(Value) * 1024)
#define Terabytes(Value) (Gigabytes(Value) * 1024)

#define internal static
#define local_persist static
#define global_variable static
#define Pi32 3.14159265359

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef float float32;
typedef double float64;

typedef int32 bool32;

struct AppMemory
{
  bool32 IsInitialized;

  uint64 PermanentStorageSize;
  void *PermanentStorage;

  uint64 TransientStorageSize;
  void *TransientStorage;
};

struct PlatformState
{
  bool32 isWebviewOpen;
};

void AppUpdateHandler(PlatformState *platformState, AppMemory *Memory);

// Webview
struct PlatformWebView
{
};

void StartWebView(const wchar_t *url, long bottom, long left, long top, long right);
void DestroyWebView();

// --- Platform-agnostic drawing API ---

struct render_color
{
  float R, G, B, A;
};

inline render_color Color(float r, float g, float b, float a = 1.0f)
{
  return {r, g, b, a};
}

inline render_color ColorRGBA(uint8 r, uint8 g, uint8 b, uint8 a = 255)
{
  return {r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f};
}

enum text_align
{
  TextAlign_Left,
  TextAlign_Center,
  TextAlign_Right,
};

enum text_valign
{
  TextVAlign_Top,
  TextVAlign_Center,
  TextVAlign_Bottom,
};

float32 WindowWidth();

// Lifecycle
void DrawBegin(render_color clearColor);
void DrawEnd();

// Rectangles
void DrawFillRect(float x, float y, float w, float h, render_color color);
void DrawOutlineRect(float x, float y, float w, float h, render_color color, float strokeWidth = 1.0f);
void DrawFillRoundRect(float x, float y, float w, float h, float rx, float ry, render_color color);

// Text
struct platform_font; // opaque — implemented per platform
struct text_metrics
{
  float width;
  float height;
  float minWidth;
};

platform_font *DrawCreateFont(const wchar_t *family, float size, bool bold = false, bool italic = false);
void DrawDestroyFont(platform_font *font);
void DrawText(platform_font *font, const wchar_t *text,
              float x, float y, float preferredWidth,
              render_color color, text_align hAlign, text_valign vAlign);
text_metrics MeasureText(platform_font *font, const wchar_t *text, float preferredWidth);

struct Position
{
  float32 x, y;
};
enum Direction
{
  ROW = 0,
  COLUMN = 1
};
enum SizingType
{
  SIZING_FIT = 0,
  SIZING_FIXED = 1,
  SIZING_GROW = 2,
};

struct Sizing
{
  SizingType type;
  float value;
};

struct TextConfig
{
  float32 fontSize;
  render_color textColor;
  platform_font *textFont;
};

enum Alignment
{
  ALIGNMENT_START = 0,
  ALIGNMENT_CENTER,
  ALIGNMENT_END,
};

struct UiElement
{
  Position position;

  struct
  {
    Sizing width, height;
    float32 minWidth, minHeight;
  } size;

  struct
  {
    float32 top, right, bottom, left;
  } padding;

  render_color backgroundColor;

  // layout
  float32 gap;
  Direction direction;
  Alignment mainAxisAlignment;
  Alignment crossaxisAlignment;

  // text
  const wchar_t *text;
  float32 fontSize;
  render_color textColor;
  platform_font *textFont;

  std::vector<UiElement> children;
  UiElement *parent;
  int parentIndex = -1;
  bool open = true;
};

#define FIXED(size) \
  Sizing { .type = SIZING_FIXED, .value = (float)size }
#define GROW() Sizing{.type = SIZING_GROW, .value = 0}
#define FIT() Sizing{.type = SIZING_FIT, .value = 0}

#define CONCAT_INNER(a, b) a##b
#define CONCAT(a, b) CONCAT_INNER(a, b)

#define DIV(config)                                                     \
  for (UiElement CONCAT(_el, __LINE__) = (OpenElement(config), config); \
       CONCAT(_el, __LINE__).open;                                      \
       (CONCAT(_el, __LINE__).open = false, CloseElement()))

#define TYPOGRAPHY(str, config)                                                        \
  {                                                                                    \
    UiElement _textEl = {};                                                            \
    platform_font *font = (config).textFont ? (config).textFont : appState.globalFont; \
    Assert(font);                                                                      \
    text_metrics textSize = MeasureText(font, str, 0);                                 \
    Sizing width = {.type = SIZING_FIT, .value = textSize.width};                      \
    _textEl.size = {width, FIT(), textSize.minWidth};                                  \
    _textEl.text = (str);                                                              \
    _textEl.fontSize = (config).fontSize;                                              \
    _textEl.textColor = (config).textColor;                                            \
    _textEl.textFont = font;                                                           \
    OpenElement(_textEl);                                                              \
    CloseElement();                                                                    \
  }

// App specific logic
enum ActivePage
{
  PAGE_STORE,
  PAGE_LIBRARY
};

struct StoreState
{
  bool32 hasInit;
  ActivePage activePage;
  platform_font *globalFont;
};
