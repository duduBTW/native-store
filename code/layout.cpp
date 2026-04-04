#include "store.h"

float Max(float a, float b)
{
  return a > b ? a : b;
}

UiElement stack[256];
int stackTop = 0;

void OpenElement(UiElement config)
{
  config.parentIndex = stackTop > 0 ? stackTop - 1 : -1;
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

void CloseElement(UiElement config)
{
  stackTop--;
  UiElement &element = stack[stackTop];

  SetSizingValuePlus(&element.size.width, element.padding.left + element.padding.right);
  SetSizingValuePlus(&element.size.height, element.padding.top + element.padding.bottom);

  float32 childGap = (element.children.size() - 1) * element.gap;
  switch (element.direction)
  {
  case ROW:
  {
    SetSizingValuePlus(&element.size.width, childGap);
    break;
  }
  case COLUMN:
  {
    SetSizingValuePlus(&element.size.height, childGap);
    break;
  }
  default:
    break;
  }

  if (element.parentIndex == -1)
  {
    return;
  }

  UiElement &parent = stack[element.parentIndex];
  parent.children.push_back(element);

  switch (element.direction)
  {
  case ROW:
  {
    SetSizingValuePlus(&parent.size.width, element.size.width.value);
    SetSizingValue(&parent.size.height, Max(parent.size.height.value, element.size.height.value));
    break;
  }
  case COLUMN:
  {
    SetSizingValuePlus(&parent.size.height, element.size.height.value);
    SetSizingValue(&parent.size.width, Max(parent.size.width.value, element.size.width.value));
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

  for (int i = 0; i < element->children.size(); i++)
  {
    UiElement &child = element->children[i];
    if (child.size.width.type == SIZING_GROW)
    {
      // Todo(Carlos): Add support for multiple grow items.
      child.size.width.value += remainderSize;
      break;
    }
  }

  // Recursion
  for (int i = 0; i < element->children.size(); i++)
  {
    UiElement child = element->children[i];
    GrowChildElements(&child);
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
  DrawFillRect(element->position.x,
               element->position.y,
               element->size.width.value, element->size.height.value, element->backgroundColor);

  for (int i = 0; i < element->children.size(); i++)
  {
    UiElement child = element->children[i];
    RenderElementAndChildren(&child);
  }
}

void Render()
{
  UiElement root = stack[0];

  // TODO(Carlos): Change to queue instead of recursion.
  GrowChildElements(&root);

  PositionElementAndChildren(&root);
  RenderElementAndChildren(&root);

  // Clean up
  stack[256] = {};
  stackTop = 0;
}
