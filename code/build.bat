@echo off

mkdir ..\build
pushd ..\build
cl /std:c++latest -FC -Zi W:\native-store\code\win32_store.cpp ^
  /I "W:\native-store\packages\include" ^
  /link "W:\native-store\packages\x64\WebView2LoaderStatic.lib" ^
  user32.lib Gdi32.lib ole32.lib advapi32.lib shlwapi.lib version.lib
popd