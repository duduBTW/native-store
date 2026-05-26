#include <Windows.h>
#include "win32_io.cpp"
#include "db.cpp"

int main()
{
  input_buffer *inputBuffer = DbNewBuffer();

  Table table = {};
  DbOpenTable("W:\\native-store\\store.db", &table);

  while (true)
  {
    DbPrintPrompt();
    DbReadInput(inputBuffer);

    if (DbIsCommand(inputBuffer))
    {
      DbDoMetaCommand(inputBuffer, &table);
    }

    Statement statement;
    if (DbPrepareStatement(inputBuffer, &statement))
    {
      DbExecuteStatement(&statement, &table);
    }
  }

  return 0;
}