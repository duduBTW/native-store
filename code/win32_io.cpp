#include <Windows.h>
#include "io.h"

struct file_handle
{
  HANDLE data;
};

int32_t CreateFileHandle(const char *fileName, file_handle *fileHandle)
{
  fileHandle->data = CreateFileA(
      fileName,
      GENERIC_READ | GENERIC_WRITE,
      FILE_SHARE_READ | FILE_SHARE_WRITE,
      0,
      OPEN_ALWAYS,
      0,
      0);

  if (fileHandle->data == INVALID_HANDLE_VALUE)
  {
    return 0;
  }

  return -1;
}

int32_t IOCloseFileHandle(file_handle *fileHandle)
{
  if (!CloseHandle(fileHandle))
  {
    return 0;
  }

  return -1;
}

int32_t GetFileSize(file_handle *fileHandle, uint64_t *fileSize)
{
  LARGE_INTEGER winFileSize;
  if (!GetFileSizeEx(fileHandle->data, &winFileSize))
  {
    return 0;
  }

  *fileSize = winFileSize.QuadPart;
  return -1;
}

int32_t SeekFile(file_handle *fileHandle, long distanceToMove)
{
  if (SetFilePointer(fileHandle->data, distanceToMove, 0, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
  {
    return 0;
  }

  return -1;
}

int32_t IOWriteFile(file_handle *fileHandle, void *data, uint32_t bytesToWrite)
{
  DWORD bytesWritten;
  if (!WriteFile(fileHandle->data, data, bytesToWrite, &bytesWritten, 0) || bytesToWrite != bytesWritten)
  {
    return 0;
  }

  return -1;
}

int32_t IOReadFile(file_handle *fileHandle, void *data, uint32_t bytesToRead)
{
  DWORD BytesRead;
  if (!ReadFile(fileHandle->data, data, bytesToRead, &BytesRead, 0))
  {
    return 0;
  }

  return -1;
}