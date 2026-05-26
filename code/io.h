#include <stdint.h>

struct file_handle;
int32_t CreateFileHandle(const char *fileName, file_handle *fileHandle);
int32_t GetFileSize(file_handle *fileHandle, uint64_t *fileSize);
int32_t SeekFile(file_handle *fileHandle, long distanceToMove);
int32_t IOWriteFile(file_handle *fileHandle, void *data, uint32_t bytesToWrite);
int32_t IOCloseFileHandle(file_handle *fileHandle);
int32_t IOReadFile(file_handle *fileHandle, void *data, uint32_t bytesToRead);