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

void CloseElement(UiElement config)
{
  stackTop--;
  UiElement &element = stack[stackTop];

  // Only do this if the parent does not have a fixed size
  element.size.width += element.padding.left + element.padding.right;
  element.size.height += element.padding.top + element.padding.bottom;
  float32 childGap = (element.children.size() - 1) * element.gap;
  switch (element.direction)
  {
  case ROW:
  {
    element.size.width += childGap;
    break;
  }
  case COLUMN:
  {
    element.size.height += childGap;
    break;
  }
  default:
    break;
  }

  if (element.parentIndex == -1)
  {
    return;
  }

  // children logic

  UiElement &parent = stack[element.parentIndex];
  parent.children.push_back(element);

  switch (element.direction)
  {
  case ROW:
  {
    parent.size.width += element.size.width;
    parent.size.height += Max(parent.size.height, element.size.height);
    break;
  }
  case COLUMN:
  {
    parent.size.height += element.size.height;
    parent.size.width += Max(parent.size.width, element.size.width);
    break;
  }
  default:
    break;
  }
}

void Render()
{
  UiElement top = stack[0];
  stack[256] = {};
  stackTop = 0;
}