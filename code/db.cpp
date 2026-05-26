#include "io.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <cstring>

inline uint32_t SafeTruncateUint64(uint64_t Value)
{
  return (uint32_t)Value;
}

// Row
#define COLUMN_GAME_NAME_SIZE 255
#define COLUMN_GAME_MINIATURE_IMAGE_PATH_SIZE 32

typedef struct Row
{
  uint32_t id;
  char name[COLUMN_GAME_NAME_SIZE + 1];
  char miniatureImagePath[COLUMN_GAME_MINIATURE_IMAGE_PATH_SIZE + 1];
};

#define size_of_attribute(Struct, Attribute) sizeof(((Struct *)0)->Attribute)

const uint32_t ID_SIZE = size_of_attribute(Row, id);
const uint32_t NAME_SIZE = size_of_attribute(Row, name);
const uint32_t MINIATURE_SIZE = size_of_attribute(Row, miniatureImagePath);
const uint32_t ROW_SIZE = ID_SIZE + NAME_SIZE + MINIATURE_SIZE;

const uint32_t ID_OFFSET = 0;
const uint32_t NAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t MINIATURE_OFFSET = NAME_OFFSET + NAME_SIZE;

void DbSerializeRow(Row *row, void *destination)
{
  char *destinationBuffer = (char *)destination;
  memcpy(destinationBuffer, &(row->id), ID_SIZE);
  memcpy(destinationBuffer + NAME_OFFSET, &(row->name), NAME_SIZE);
  memcpy(destinationBuffer + MINIATURE_OFFSET, &(row->miniatureImagePath), MINIATURE_SIZE);
}

void DbDeserializeRow(void *source, Row *row)
{
  char *sourceBuffer = (char *)source;
  memcpy(&(row->id), sourceBuffer + ID_OFFSET, ID_SIZE);
  memcpy(&(row->name), sourceBuffer + NAME_OFFSET, NAME_SIZE);
  memcpy(&(row->miniatureImagePath), sourceBuffer + MINIATURE_OFFSET, MINIATURE_SIZE);
}

// Page
#define TABLE_MAX_PAGES 100

const uint32_t PAGE_SIZE = 4096;
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

typedef struct Pager
{
  file_handle fileHandle;
  uint32_t fileSize;
  void *pages[TABLE_MAX_PAGES];
};

typedef struct Table
{
  Pager pager;
  uint32_t numRows;
};

typedef struct Cursor
{
  Table *table;
  uint32_t position;
  bool isAtTheEndOfTheTable;
};

void DbPagerOpen(const char *fileName, Pager *pager)
{
  file_handle fileHandle = {};
  if (!CreateFileHandle(fileName, &fileHandle))
  {
    printf("Error opening db file\n");
    exit(EXIT_FAILURE);
  }

  uint64_t fileSize;
  if (!GetFileSize(&fileHandle, &fileSize))
  {
    printf("Failed to get file size!\n");
    exit(EXIT_FAILURE);
  }

  pager->fileSize = SafeTruncateUint64(fileSize);
  pager->fileHandle = fileHandle;
}

void DbOpenTable(const char *fileName, Table *table)
{
  Pager pager = {};
  DbPagerOpen(fileName, &pager);
  uint32_t numRows = pager.fileSize / ROW_SIZE;

  table->pager = pager;
  table->numRows = numRows;
}

void TableStart(Table *table, Cursor *cursor)
{
  cursor->table = table;
  cursor->isAtTheEndOfTheTable = (table->numRows == 0);
}

void TableEnd(Table *table, Cursor *cursor)
{
  cursor->table = table;
  cursor->position = table->numRows;
  cursor->isAtTheEndOfTheTable = true;
}

void DbPageFlush(Pager *pager, uint32_t pageIndex, uint32_t bytesToWrite)
{
  if (!pager->pages[pageIndex])
  {
    printf("Tried to flush null page\n");
    exit(EXIT_FAILURE);
  }

  if (!SeekFile(&pager->fileHandle, pageIndex * PAGE_SIZE))
  {
    printf("Could not seek to page on flush %d\n", pageIndex);
    exit(EXIT_FAILURE);
  }

  if (!IOWriteFile(&pager->fileHandle, pager->pages[pageIndex], bytesToWrite))
  {
    printf("Failed to flush file, could not write page %d\n", pageIndex);
    exit(EXIT_FAILURE);
  }

  free(pager->pages[pageIndex]);
  pager->pages[pageIndex] = NULL;
}

void CloseDb(Table *table)
{
  Pager pager = table->pager;
  uint32_t fullPagesCount = table->numRows / ROWS_PER_PAGE;
  for (uint32_t i = 0; i < fullPagesCount; i++)
  {
    if (!pager.pages[i])
    {
      continue;
    }

    DbPageFlush(&pager, i, PAGE_SIZE);
  }

  uint32_t additionalRows = table->numRows % ROWS_PER_PAGE;
  if (additionalRows > 0 && pager.pages[fullPagesCount])
  {
    DbPageFlush(&pager, fullPagesCount, additionalRows * ROW_SIZE);
  }

  IOCloseFileHandle(&pager.fileHandle);
}

void *DbGetPage(Pager *pager, uint32_t pageIndex)
{
  if (pageIndex > TABLE_MAX_PAGES)
  {
    printf("Tried to fetch page number out of bounds. %d > %d\n", pageIndex,
           +TABLE_MAX_PAGES);
    exit(EXIT_FAILURE);
  }

  if (pager->pages[pageIndex] == NULL)
  {
    void *page = malloc(PAGE_SIZE);
    pager->pages[pageIndex] = page;

    uint32_t totalNumPages = pager->fileSize / PAGE_SIZE;
    if (pager->fileSize % PAGE_SIZE)
    {
      totalNumPages++;
    }

    if (pageIndex > totalNumPages)
    {
      printf("Page %d is outside of the bounds, max is %d\n", pageIndex,
             totalNumPages);
      exit(EXIT_FAILURE);
    }

    if (!SeekFile(&pager->fileHandle, pageIndex * PAGE_SIZE))
    {
      printf("Could not seek to page %d\n", pageIndex);
      exit(EXIT_FAILURE);
    }

    if (!IOReadFile(&pager->fileHandle, pager->pages[pageIndex], PAGE_SIZE))
    {
      printf("Failed to read db file\n");
      exit(EXIT_FAILURE);
    }
  }

  return pager->pages[pageIndex];
}

void *DbCursorValue(Cursor *cursor)
{
  uint32_t rowNum = cursor->position;
  uint32_t pageIndex = rowNum / ROWS_PER_PAGE;
  void *page = DbGetPage(&cursor->table->pager, pageIndex);
  uint32_t rowOffset = rowNum % ROWS_PER_PAGE;
  uint32_t byteOffset = ROW_SIZE * rowOffset;
  return (uint8_t *)page + byteOffset;
}

void DbCursorAdvance(Cursor *cursor)
{
  cursor->position++;
  cursor->isAtTheEndOfTheTable = cursor->position >= cursor->table->numRows;
}

typedef struct input_buffer
{
  char *buffer;
  size_t buffer_length;
  size_t input_length;
};

typedef enum
{
  STATEMENT_INSERT,
  STATEMENT_SELECT
} statement_type;

typedef struct
{
  statement_type type;
  Row rowToInsert;
} Statement;

input_buffer *DbNewBuffer()
{
  input_buffer *buffer = (input_buffer *)malloc(sizeof(input_buffer));

  buffer->buffer_length = 1024;
  buffer->buffer = (char *)malloc(buffer->buffer_length);
  buffer->input_length = 0;

  return buffer;
}

size_t DbDBGetLine(char **lineptr, size_t *n, FILE *stream)
{
  return 0;
}

void DbPrintPrompt()
{
  printf("db > ");
}

void DbReadInput(input_buffer *inputBuffer)
{
  if (fgets(inputBuffer->buffer, inputBuffer->buffer_length, stdin) == NULL)
  {
    printf("Error reading input\n");
    exit(EXIT_FAILURE);
  }

  inputBuffer->input_length = strlen(inputBuffer->buffer);

  // Remove trailing newline
  if (inputBuffer->input_length > 0 &&
      inputBuffer->buffer[inputBuffer->input_length - 1] == '\n')
  {
    inputBuffer->buffer[inputBuffer->input_length - 1] = '\0';
    inputBuffer->input_length--;
  }
}

void DbDBCloseInputBuffer(input_buffer *inputBuffer)
{
  free(inputBuffer->buffer);
  free(inputBuffer);
}

void DbDoMetaCommand(input_buffer *inputBuffer, Table *table)
{
  if (strcmp(inputBuffer->buffer, ".exit") == 0)
  {
    CloseDb(table);
    exit(EXIT_SUCCESS);
  }

  printf("Unrecognized command '%s'\n", inputBuffer->buffer);
}

bool DbIsCommand(input_buffer *inputBuffer)
{
  return inputBuffer->buffer[0] == '.';
}

bool DbPrepareInsert(input_buffer *inputBuffer, Statement *statement)
{
  statement->type = STATEMENT_INSERT;

  char *keyword = strtok(inputBuffer->buffer, " ");
  char *idString = strtok(NULL, " ");
  char *name = strtok(NULL, " ");
  char *miniature = strtok(NULL, " ");

  if (idString == NULL || name == NULL || miniature == NULL)
  {
    printf("Invalid syntax '%s'\n", inputBuffer->buffer);
    return false;
  }

  int id = atoi(idString);
  if (id < 0)
  {
    printf("Invalid negative id\n");
    return false;
  }

  if (strlen(name) > COLUMN_GAME_NAME_SIZE)
  {
    printf("Name is too long\n");
    return false;
  }

  if (strlen(miniature) > COLUMN_GAME_MINIATURE_IMAGE_PATH_SIZE)
  {
    printf("Miniature is too long\n");
    return false;
  }

  statement->rowToInsert.id = id;
  strcpy(statement->rowToInsert.name, name);
  strcpy(statement->rowToInsert.miniatureImagePath, miniature);
}

bool DbPrepareStatement(input_buffer *inputBuffer, Statement *statement)
{
  if (strncmp(inputBuffer->buffer, "insert", 6) == 0)
  {
    return DbPrepareInsert(inputBuffer, statement);
  }

  if (strcmp(inputBuffer->buffer, "select") == 0)
  {
    statement->type = STATEMENT_SELECT;
    return true;
  }

  printf("Unrecognized keyword at start of '%s'.\n",
         +inputBuffer->buffer);

  return false;
}

void DbExecuteInsert(Statement *statement, Table *table)
{
  if (table->numRows >= TABLE_MAX_ROWS)
  {
    printf("Table is full");
    return;
  }

  Row *rowToInsert = &(statement->rowToInsert);
  Cursor cursor = {};

  TableEnd(table, &cursor);
  DbSerializeRow(rowToInsert, DbCursorValue(&cursor));
  table->numRows++;
}

void DbPrintRow(Row *row)
{
  printf("(%d, %s, %s)\n", row->id, row->name, row->miniatureImagePath);
}

void DbExecuteSelect(Statement *statement, Table *table)
{
  Row row;
  Cursor cursor = {};

  TableStart(table, &cursor);
  while (!cursor.isAtTheEndOfTheTable)
  {
    DbDeserializeRow(DbCursorValue(&cursor), &row);
    DbCursorAdvance(&cursor);
    DbPrintRow(&row);
  }
}

void DbExecuteStatement(Statement *statement, Table *table)
{
  switch (statement->type)
  {
  case STATEMENT_INSERT:
  {
    DbExecuteInsert(statement, table);
    break;
  }
  case STATEMENT_SELECT:
  {
    DbExecuteSelect(statement, table);
    break;
  }

  default:
    break;
  }
}
