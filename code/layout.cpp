#include <unordered_map>
#include <vector>
#include <string>

float Max(float a, float b)
{
  return a > b ? a : b;
}

float Min(float a, float b)
{
  return a < b ? a : b;
}

UiElement stack[256];
UiElement *root;
int stackTop = 0;

// Images
std::unordered_map<std::wstring, platform_image *> loadedImages;
std::unordered_map<std::wstring, bool> currentImages;

void OpenElement(UiElement config)
{
  bool isFirstElement = stackTop == 0;
  config.parentIndex = isFirstElement ? -1 : stackTop - 1;
  if (config.size.minWidth == 0.0f)
  {
    config.size.minWidth = config.size.width.value;
  }
  if (config.size.minHeight == 0.0f)
  {
    config.size.minHeight = config.size.height.value;
  }

  stack[stackTop] = config;
  stackTop++;
}

// Todo(Carlos): This should not exist
void SetSizingValuePlus(Sizing *size, float32 value)
{
  if (size->type == SIZING_FIXED)
  {
    return;
  }

  // Todo(Carlos): +=?
  size->value += value;
}

void SetSizingValue(Sizing *size, float32 value)
{
  if (size->type == SIZING_FIXED)
  {
    return;
  }

  size->value = value;
}

float32 ElementChildrenGap(UiElement *element)
{
  return (element->children.size() - 1) * element->gap;
}

void FitSizeWidth(UiElement *element)
{
  SetSizingValuePlus(&element->size.width, element->padding.left + element->padding.right);
  float32 childGap = ElementChildrenGap(element);

  if (element->direction == ROW)
  {
    SetSizingValuePlus(&element->size.width, childGap);
  }

  if (!element->parent)
  {
    return;
  }

  UiElement &parent = stack[element->parentIndex];
  switch (element->direction)
  {
  case ROW:
  {
    SetSizingValuePlus(&parent.size.width, element->size.width.value);
    parent.size.minWidth += element->size.minWidth;
    // TODO(Carlos): Implement min height.
    break;
  }
  case COLUMN:
  {
    SetSizingValue(&parent.size.width, Max(parent.size.width.value, element->size.width.value));
    // TODO(Carlos): Implement min height.
    break;
  }
  default:
    break;
  }
}

void FitSizeHeight(UiElement *element)
{
  // Recursion
  for (int i = 0; i < element->children.size(); i++)
  {
    FitSizeHeight(&element->children[i]);
  }

  SetSizingValuePlus(&element->size.height, element->padding.top + element->padding.bottom);
  float32 childGap = ElementChildrenGap(element);

  if (element->imagePath && !element->size.height.value)
  {
    image_dimensions dimensions = ImageDimensions(loadedImages[element->imagePath]);
    SetSizingValue(&element->size.height, element->size.width.value / dimensions.aspectRatio);
  }

  if (element->direction == COLUMN)
  {
    SetSizingValuePlus(&element->size.height, childGap);
  }

  if (!element->parent)
  {
    return;
  }

  UiElement *parent = element->parent;

  switch (element->direction)
  {
  case ROW:
  {
    SetSizingValue(&parent->size.height, Max(parent->size.height.value, element->size.height.value));
    // TODO(Carlos): Implement min height.
    break;
  }
  case COLUMN:
  {
    SetSizingValuePlus(&parent->size.height, element->size.height.value);
    parent->size.minHeight = element->size.minHeight;
    // TODO(Carlos): Implement min height.
    break;
  }
  default:
    break;
  }
}

void GrowChildElements(UiElement *element)
{
  float32 remainderSize = element->size.width.value;
  float32 childGaps = (element->children.size() - 1) * element->gap;
  remainderSize -= element->padding.left + element->padding.right + childGaps;

  std::vector<UiElement *> growable;
  std::vector<UiElement *> shrinkable;
  for (int i = 0; i < element->children.size(); i++)
  {
    UiElement &child = element->children[i];
    switch (child.size.width.type)
    {
    case SIZING_GROW:
    {
      growable.push_back(&child);
      break;
    }
    case SIZING_FIT:
    {
      shrinkable.push_back(&child);
      break;
    }

    default:
      break;
    }
  }

  if (element->direction == ROW)
  {
    for (int i = 0; i < element->children.size(); i++)
    {
      UiElement child = element->children[i];
      remainderSize -= child.size.width.value;
    }

    while (remainderSize > 0 && growable.size() > 0)
    {
      float32 smallest = growable[0]->size.width.value;
      float32 secondSmallest = INFINITY;
      float32 sizeToAdd = remainderSize;

      // find smallest and second smallest
      for (int i = 0; i < growable.size(); i++)
      {
        float32 width = growable[i]->size.width.value;
        if (width < smallest)
        {
          secondSmallest = smallest;
          smallest = width;
        }

        if (width > smallest)
        {
          secondSmallest = Min(secondSmallest, width);
        }
      }

      if (secondSmallest != INFINITY)
      {
        sizeToAdd = secondSmallest - smallest;
      }

      sizeToAdd = Min(sizeToAdd, remainderSize / (float32)growable.size());

      for (int i = 0; i < growable.size(); i++)
      {
        if (growable[i]->size.width.value == smallest)
        {
          growable[i]->size.width.value += sizeToAdd;
          remainderSize -= sizeToAdd;
        }
      }
    }

    while (remainderSize < 0 && shrinkable.size() > 0)
    {
      float32 largest = shrinkable[0]->size.width.value;
      float32 secondLargest = 0;
      float32 sizeToRemove = remainderSize;

      for (int i = 0; i < shrinkable.size(); i++)
      {
        float32 width = shrinkable[i]->size.width.value;
        if (width > largest)
        {
          secondLargest = largest;
          largest = width;
        }
        if (width < largest)
          secondLargest = Max(secondLargest, width);
      }

      if (secondLargest != 0)
      {
        sizeToRemove = secondLargest - largest;
      }

      sizeToRemove = Max(sizeToRemove, remainderSize / (float32)shrinkable.size());

      for (int i = 0; i < shrinkable.size(); i++)
      {
        if (shrinkable[i]->size.width.value == largest)
        {
          float32 previousWidth = shrinkable[i]->size.width.value;
          shrinkable[i]->size.width.value += sizeToRemove; // sizeToRemove is negative
          remainderSize -= (shrinkable[i]->size.width.value - previousWidth);

          if (shrinkable[i]->size.width.value <= shrinkable[i]->size.minWidth)
          {
            shrinkable[i]->size.width.value = shrinkable[i]->size.minWidth;
            shrinkable.erase(shrinkable.begin() + i--);
          }
        }
      }
    }
  }
  else if (growable.size() > 0 && remainderSize > 0)
  {
    for (int i = 0; i < growable.size(); i++)
    {
      growable[i]->size.width.value = remainderSize;
    }
  }

  // Recursion
  for (int i = 0; i < element->children.size(); i++)
  {
    GrowChildElements(&element->children[i]);
  }
}

void GrowChildElementsHeight(UiElement *element)
{
  float32 remainderSize = element->size.height.value;
  float32 childGaps = (element->children.size() - 1) * element->gap;
  remainderSize -= element->padding.top + element->padding.bottom + childGaps;

  std::vector<UiElement *> growable;
  std::vector<UiElement *> shrinkable;
  for (int i = 0; i < element->children.size(); i++)
  {
    UiElement &child = element->children[i];
    switch (child.size.height.type)
    {
    case SIZING_GROW:
    {
      growable.push_back(&child);
      break;
    }
    case SIZING_FIT:
    {
      shrinkable.push_back(&child);
      break;
    }

    default:
      break;
    }
  }

  if (element->direction == COLUMN)
  {
    for (int i = 0; i < element->children.size(); i++)
    {
      UiElement child = element->children[i];
      remainderSize -= child.size.height.value;
    }

    while (remainderSize > 0 && growable.size() > 0)
    {
      float32 smallest = growable[0]->size.height.value;
      float32 secondSmallest = INFINITY;
      float32 sizeToAdd = remainderSize;

      // find smallest and second smallest
      for (int i = 0; i < growable.size(); i++)
      {
        float32 height = growable[i]->size.height.value;
        if (height < smallest)
        {
          secondSmallest = smallest;
          smallest = height;
        }

        if (height > smallest)
        {
          secondSmallest = Min(secondSmallest, height);
        }
      }

      if (secondSmallest != INFINITY)
      {
        sizeToAdd = secondSmallest - smallest;
      }

      sizeToAdd = Min(sizeToAdd, remainderSize / (float32)growable.size());

      for (int i = 0; i < growable.size(); i++)
      {
        if (growable[i]->size.height.value == smallest)
        {
          growable[i]->size.height.value += sizeToAdd;
          remainderSize -= sizeToAdd;
        }
      }
    }

    while (remainderSize < 0 && shrinkable.size() > 0)
    {
      float32 largest = shrinkable[0]->size.height.value;
      float32 secondLargest = 0;
      float32 sizeToRemove = remainderSize;

      for (int i = 0; i < shrinkable.size(); i++)
      {
        float32 height = shrinkable[i]->size.height.value;
        if (height > largest)
        {
          secondLargest = largest;
          largest = height;
        }
        if (height < largest)
          secondLargest = Max(secondLargest, height);
      }

      if (secondLargest != 0)
      {
        sizeToRemove = secondLargest - largest;
      }

      sizeToRemove = Max(sizeToRemove, remainderSize / (float32)shrinkable.size());

      for (int i = 0; i < shrinkable.size(); i++)
      {
        if (shrinkable[i]->size.height.value == largest)
        {
          float32 previousHeight = shrinkable[i]->size.height.value;
          shrinkable[i]->size.height.value += sizeToRemove; // sizeToRemove is negative
          remainderSize -= (shrinkable[i]->size.height.value - previousHeight);

          if (shrinkable[i]->size.height.value <= shrinkable[i]->size.minHeight)
          {
            shrinkable[i]->size.height.value = shrinkable[i]->size.minHeight;
            shrinkable.erase(shrinkable.begin() + i--);
          }
        }
      }
    }
  }
  else if (growable.size() > 0 && remainderSize > 0)
  {
    for (int i = 0; i < growable.size(); i++)
    {
      growable[i]->size.height.value = remainderSize;
    }
  }

  // Recursion
  for (int i = 0; i < element->children.size(); i++)
  {
    GrowChildElementsHeight(&element->children[i]);
  }
}

// TODO(Carlos): Add column main/cross axis alignment;
void PositionElementAndChildren(UiElement *element)
{
  float32 lastY = element->position.y + element->padding.top;
  float32 lastX = element->position.x + element->padding.left;

  // mais axis
  if (element->direction == ROW && element->mainAxisAlignment != ALIGNMENT_START)
  {
    float32 totalChildrenWidth = ElementChildrenGap(element);

    for (int i = 0; i < element->children.size(); i++)
    {
      totalChildrenWidth += element->children[i].size.width.value;
    }

    float32 lastXAlignmentOffset =
        (element->size.width.value - (element->padding.right + element->padding.left)) - totalChildrenWidth;
    if (element->mainAxisAlignment == ALIGNMENT_CENTER)
    {
      lastXAlignmentOffset = (lastXAlignmentOffset / 2);
    }

    lastX += lastXAlignmentOffset;
  }

  for (int i = 0; i < element->children.size(); i++)
  {
    UiElement &child = element->children[i];
    child.position.x = lastX;
    child.position.y = lastY;

    if (element->direction == ROW)
    {
      lastX += element->gap + child.size.width.value;

      if (element->crossaxisAlignment != ALIGNMENT_START)
      {
        float32 yAlignmentOffset =
            (element->size.height.value - (element->padding.top + element->padding.bottom)) -
            child.size.height.value;

        if (element->crossaxisAlignment == ALIGNMENT_CENTER)
        {
          yAlignmentOffset = yAlignmentOffset / 2;
        }

        child.position.y +=
            yAlignmentOffset;
      }
    }
    if (element->direction == COLUMN)
    {
      lastY += element->gap + child.size.height.value;
    }

    PositionElementAndChildren(&child);
  }
}

void RenderElementAndChildren(UiElement *element)
{
  if (element->text)
  {
    DrawText(element->textFont, element->text,
             element->position.x, element->position.y,
             element->size.width.value, element->textColor, TextAlign_Left, TextVAlign_Top);
  }
  else if (element->imagePath)
  {
    DrawImage(loadedImages[element->imagePath],
              element->position.x, element->position.y,
              element->size.width.value, element->size.height.value);
  }
  else
  {
    DrawFillRect(element->position.x,
                 element->position.y,
                 element->size.width.value, element->size.height.value, element->backgroundColor);
  }

  for (int i = 0; i < element->children.size(); i++)
  {
    UiElement child = element->children[i];
    RenderElementAndChildren(&child);
  }
}

void FixParentPointers(UiElement *element)
{
  for (int i = 0; i < element->children.size(); i++)
  {
    element->children[i].parent = element;
    FixParentPointers(&element->children[i]);
  }
}

void WrapTexts(UiElement *element)
{
  for (int i = 0; i < element->children.size(); i++)
  {
    WrapTexts(&element->children[i]);
  }

  if (!element->text)
  {
    return;
  }

  float32 wrappedHeight = MeasureText(element->textFont, element->text, element->size.width.value).height;
  element->size.height.value = wrappedHeight;
  element->size.minHeight = wrappedHeight;
}

// TODO(Carlos): Do this on a different thread
void LoadImages(UiElement *element)
{
  if (element->imagePath)
  {
    currentImages[element->imagePath] = true;
    if (!loadedImages[element->imagePath])
    {
      loadedImages[element->imagePath] = DrawLoadImage(element->imagePath);
    }
  }

  for (int i = 0; i < element->children.size(); i++)
  {
    element->children[i].parent = element;
    LoadImages(&element->children[i]);
  }
}

// TODO(Carlos): Do this on a different thread
void UnloadImages()
{
  for (const auto &[imagePath, image] : loadedImages)
  {
    // image still on frame
    if (currentImages[imagePath])
    {
      continue;
    }

    DrawDestroyImage(loadedImages[imagePath]);
    loadedImages.erase(imagePath);
  }
}

void Render()
{
  root = &stack[0];
  LoadImages(root);
  UnloadImages();
  currentImages.clear();

  FixParentPointers(root);

  // TODO(Carlos): Change to queue instead of recursion.
  GrowChildElements(root);
  WrapTexts(root);
  FitSizeHeight(root);
  GrowChildElementsHeight(root);

  PositionElementAndChildren(root);

  RenderElementAndChildren(root);

  Events();

  memset(stack, 0, sizeof(stack));
  stackTop = 0;
}

void CloseElement()
{
  stackTop--;
  UiElement &element = stack[stackTop];

  if (element.parentIndex != -1)
  {
    UiElement &parent = stack[element.parentIndex];
    parent.children.push_back(element);
    element.parent = &parent;
  }

  FitSizeWidth(&element);

  // is root element
  if (element.parentIndex == -1)
  {
    // TODO(Carlos) Render breaks when there are no elements.
    Render();
  }
}

void HandleHover(UiElement *element)
{
}

void HandleHoverInit(UiElement *element)
{
  return HandleHover(element);
}

UiElement *Current()
{
  return &stack[stackTop];
}

// void IsHovering()
// {
//   eventsTop++;
//   void fn = HandleHover(&stack[stackTop])
//       events[eventsTop] = &;
// }

UiElement *FindElementByIdStep(const wchar_t *id, UiElement *element)
{
  if (element->id && std::wcscmp(element->id, id) == 0)
  {
    return element;
  }

  for (int i = 0; i < element->children.size(); i++)
  {
    UiElement *found = FindElementByIdStep(id, &element->children[i]);
    if (found)
    {
      return found;
    }
  }

  return 0;
}

UiElement *FindElementById(const wchar_t *id)
{
  return FindElementByIdStep(id, root);
}

bool PointerOver(const wchar_t *id)
{
  UiElement *element = FindElementById(id);
  if (!element)
  {
    return false;
  }

  bool isInXBounds =
      (platformState->xMousePos >= element->position.x) &&
      (platformState->xMousePos <= element->position.x + element->size.width.value);
  bool isInYBounds =
      (platformState->yMousePos >= element->position.y) &&
      (platformState->yMousePos <= element->position.y + element->size.height.value);

  return isInXBounds && isInYBounds;
}
