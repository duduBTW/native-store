#include "store.h"

float Max(float a, float b)
{
  return a > b ? a : b;
}

float Min(float a, float b)
{
  return a < b ? a : b;
}

UiElement stack[256];
int stackTop = 0;

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

float32 ElementChildGap(UiElement *element)
{
  return (element->children.size() - 1) * element->gap;
}

void FitSizeWidth(UiElement *element)
{
  SetSizingValuePlus(&element->size.width, element->padding.left + element->padding.right);
  float32 childGap = ElementChildGap(element);

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
  float32 childGap = ElementChildGap(element);

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

  for (int i = 0; i < element->children.size(); i++)
  {
    UiElement child = element->children[i];
    remainderSize -= child.size.width.value;
  }

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

  for (int i = 0; i < element->children.size(); i++)
  {
    UiElement child = element->children[i];
    remainderSize -= child.size.height.value;
  }

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

  // Recursion
  for (int i = 0; i < element->children.size(); i++)
  {
    GrowChildElementsHeight(&element->children[i]);
  }
}

void PositionElementAndChildren(UiElement *element)
{
  float32 lastY = element->position.y + element->padding.top;
  float32 lastX = element->position.x + element->padding.left;
  for (int i = 0; i < element->children.size(); i++)
  {
    UiElement &child = element->children[i];
    child.position.x = lastX;
    child.position.y = lastY;

    if (element->direction == ROW)
    {
      lastX += element->gap + child.size.width.value;
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

void Render()
{
  UiElement *root = &stack[0];

  FixParentPointers(root);

  // TODO(Carlos): Change to queue instead of recursion.
  GrowChildElements(root);
  WrapTexts(root);
  FitSizeHeight(root);
  // GrowChildElementsHeight(root);

  PositionElementAndChildren(root);
  RenderElementAndChildren(root);

  // Clean up
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