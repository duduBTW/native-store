#include <d2d1.h>
#include <dwrite.h>
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

struct platform_font
{
  IDWriteTextFormat *Format;
};

struct win32_d2d_state
{
  ID2D1HwndRenderTarget *RenderTarget;
  IDWriteFactory *DWriteFactory;
  ID2D1SolidColorBrush *Brush;
};

global_variable win32_d2d_state gD2D = {};

// Called from WinMain after window creation
bool Win32D2DInit(HWND window)
{
  ID2D1Factory *factory = nullptr;
  if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory)))
    return false;

  RECT rc;
  GetClientRect(window, &rc);

  HRESULT hr = factory->CreateHwndRenderTarget(
      D2D1::RenderTargetProperties(),
      D2D1::HwndRenderTargetProperties(window, D2D1::SizeU(rc.right, rc.bottom)),
      &gD2D.RenderTarget);

  factory->Release();
  if (FAILED(hr))
    return false;

  hr = DWriteCreateFactory(
      DWRITE_FACTORY_TYPE_SHARED,
      __uuidof(IDWriteFactory),
      reinterpret_cast<IUnknown **>(&gD2D.DWriteFactory));
  if (FAILED(hr))
    return false;

  gD2D.RenderTarget->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0), &gD2D.Brush);
  return true;
}

void Win32D2DResize(uint32 width, uint32 height)
{
  if (gD2D.RenderTarget)
    gD2D.RenderTarget->Resize(D2D1::SizeU(width, height));
}

void Win32D2DDestroy()
{
  if (gD2D.Brush)
  {
    gD2D.Brush->Release();
    gD2D.Brush = nullptr;
  }
  if (gD2D.DWriteFactory)
  {
    gD2D.DWriteFactory->Release();
    gD2D.DWriteFactory = nullptr;
  }
  if (gD2D.RenderTarget)
  {
    gD2D.RenderTarget->Release();
    gD2D.RenderTarget = nullptr;
  }
}

// --- Helpers ---

internal inline D2D1_COLOR_F ToD2DColor(render_color c)
{
  return D2D1::ColorF(c.R, c.G, c.B, c.A);
}

internal inline void SetBrush(render_color color)
{
  gD2D.Brush->SetColor(ToD2DColor(color));
}

// --- Platform-agnostic API implementation ---

void DrawBegin(render_color clearColor)
{
  gD2D.RenderTarget->BeginDraw();
  gD2D.RenderTarget->Clear(ToD2DColor(clearColor));
}

void DrawEnd()
{
  gD2D.RenderTarget->EndDraw();
}

void DrawFillRect(float x, float y, float w, float h, render_color color)
{
  SetBrush(color);
  gD2D.RenderTarget->FillRectangle(D2D1::RectF(x, y, x + w, y + h), gD2D.Brush);
}

void DrawOutlineRect(float x, float y, float w, float h, render_color color, float strokeWidth)
{
  SetBrush(color);
  gD2D.RenderTarget->DrawRectangle(D2D1::RectF(x, y, x + w, y + h), gD2D.Brush, strokeWidth);
}

void DrawFillRoundRect(float x, float y, float w, float h, float rx, float ry, render_color color)
{
  SetBrush(color);
  D2D1_ROUNDED_RECT rr = {D2D1::RectF(x, y, x + w, y + h), rx, ry};
  gD2D.RenderTarget->FillRoundedRectangle(rr, gD2D.Brush);
}

platform_font *DrawCreateFont(const wchar_t *family, float size, bool bold, bool italic)
{
  platform_font *font = new platform_font{};
  DWRITE_FONT_WEIGHT weight = bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
  DWRITE_FONT_STYLE style = italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;

  gD2D.DWriteFactory->CreateTextFormat(
      family, nullptr, weight, style,
      DWRITE_FONT_STRETCH_NORMAL, size, L"", &font->Format);

  return font;
}

void DrawDestroyFont(platform_font *font)
{
  if (font)
  {
    if (font->Format)
      font->Format->Release();
    delete font;
  }
}

void DrawText(platform_font *font, const wchar_t *text,
              float x, float y, float w, float h,
              render_color color, text_align hAlign, text_valign vAlign)
{
  DWRITE_TEXT_ALIGNMENT hA;
  switch (hAlign)
  {
  case TextAlign_Center:
    hA = DWRITE_TEXT_ALIGNMENT_CENTER;
    break;
  case TextAlign_Right:
    hA = DWRITE_TEXT_ALIGNMENT_TRAILING;
    break;
  default:
    hA = DWRITE_TEXT_ALIGNMENT_LEADING;
    break;
  }

  DWRITE_PARAGRAPH_ALIGNMENT vA;
  switch (vAlign)
  {
  case TextVAlign_Center:
    vA = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
    break;
  case TextVAlign_Bottom:
    vA = DWRITE_PARAGRAPH_ALIGNMENT_FAR;
    break;
  default:
    vA = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
    break;
  }

  font->Format->SetTextAlignment(hA);
  font->Format->SetParagraphAlignment(vA);
  SetBrush(color);

  gD2D.RenderTarget->DrawText(
      text, (UINT32)wcslen(text), font->Format,
      D2D1::RectF(x, y, x + w, y + h),
      gD2D.Brush);
}