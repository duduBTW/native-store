#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")
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
  IWICImagingFactory *WICFactory;
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

  gD2D.RenderTarget->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0), &gD2D.Brush); // só uma vez

  hr = CoCreateInstance(
      CLSID_WICImagingFactory, nullptr,
      CLSCTX_INPROC_SERVER,
      IID_PPV_ARGS(&gD2D.WICFactory));
  if (FAILED(hr))
    return false;

  return true; // só um
}

void Win32D2DResize(uint32 width, uint32 height)
{
  if (gD2D.RenderTarget)
    gD2D.RenderTarget->Resize(D2D1::SizeU(width, height));
}

void Win32D2DDestroy()
{
  if (gD2D.WICFactory)
  {
    gD2D.WICFactory->Release();
    gD2D.WICFactory = nullptr;
  }

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

text_metrics MeasureText(platform_font *font, const wchar_t *text, float preferredWidth)
{
  float layoutWidth = (preferredWidth == 0.0f) ? FLT_MAX : preferredWidth;

  IDWriteTextLayout *layout = nullptr;
  gD2D.DWriteFactory->CreateTextLayout(
      text,
      (UINT32)wcslen(text),
      font->Format,
      layoutWidth,
      FLT_MAX,
      &layout);

  if (preferredWidth == 0.0f)
    layout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

  DWRITE_TEXT_METRICS metrics = {};
  layout->GetMetrics(&metrics);

  // Compute min width: re-layout with wrapping at every opportunity
  float minWidth = 0.0f;
  IDWriteTextLayout *minLayout = nullptr;
  gD2D.DWriteFactory->CreateTextLayout(
      text,
      (UINT32)wcslen(text),
      font->Format,
      0.0f, // zero width forces wrapping at every word
      FLT_MAX,
      &minLayout);

  DWRITE_TEXT_METRICS minMetrics = {};
  minLayout->GetMetrics(&minMetrics);
  minWidth = minMetrics.width;
  minLayout->Release();

  layout->Release();

  return {metrics.width, metrics.height, minWidth};
}

void DrawText(platform_font *font, const wchar_t *text,
              float x, float y, float preferredWidth,
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

  IDWriteTextLayout *layout = nullptr;
  float layoutWidth = (preferredWidth == 0.0f) ? FLT_MAX : preferredWidth;
  gD2D.DWriteFactory->CreateTextLayout(
      text,
      (UINT32)wcslen(text),
      font->Format,
      layoutWidth,
      FLT_MAX,
      &layout);

  layout->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);

  SetBrush(color);
  gD2D.RenderTarget->DrawTextLayout(
      D2D1::Point2F(x, y),
      layout,
      gD2D.Brush);

  layout->Release();
}

struct platform_image
{
  ID2D1Bitmap *Bitmap;
};

image_dimensions ImageDimensions(platform_image *image)
{
  Assert(image);
  image_dimensions dimensions = {};

  if (!image || !image->Bitmap)
    return dimensions;

  D2D1_SIZE_U size = image->Bitmap->GetPixelSize();

  dimensions.width = size.width;
  dimensions.height = size.height;

  dimensions.aspectRatio =
      size.height != 0
          ? (float)size.width / (float)size.height
          : 0.0f;

  return dimensions;
}

platform_image *DrawLoadImage(const wchar_t *path)
{
  IWICBitmapDecoder *decoder = nullptr;
  IWICBitmapFrameDecode *frame = nullptr;
  IWICFormatConverter *converter = nullptr;

  platform_image *image = new platform_image{};

  HRESULT hr = gD2D.WICFactory->CreateDecoderFromFilename(
      path, nullptr, GENERIC_READ,
      WICDecodeMetadataCacheOnLoad, &decoder);
  if (FAILED(hr))
    goto cleanup;

  hr = decoder->GetFrame(0, &frame);
  if (FAILED(hr))
    goto cleanup;

  hr = gD2D.WICFactory->CreateFormatConverter(&converter);
  if (FAILED(hr))
    goto cleanup;

  hr = converter->Initialize(
      frame,
      GUID_WICPixelFormat32bppPBGRA,
      WICBitmapDitherTypeNone, nullptr,
      0.0f, WICBitmapPaletteTypeMedianCut);
  if (FAILED(hr))
    goto cleanup;

  gD2D.RenderTarget->CreateBitmapFromWicBitmap(converter, nullptr, &image->Bitmap);

cleanup:
  if (converter)
    converter->Release();
  if (frame)
    frame->Release();
  if (decoder)
    decoder->Release();

  return image;
}

void DrawDestroyImage(platform_image *image)
{
  if (image)
  {
    if (image->Bitmap)
      image->Bitmap->Release();
    delete image;
  }
}

void DrawImage(platform_image *image, float x, float y, float w, float h)
{
  if (!image || !image->Bitmap)
    return;
  gD2D.RenderTarget->DrawBitmap(
      image->Bitmap,
      D2D1::RectF(x, y, x + w, y + h),
      1.0f,
      D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
}