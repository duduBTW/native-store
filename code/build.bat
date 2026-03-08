@echo off

mkdir ..\build
pushd ..\build
cl -DHANDMADE_SLOW=1 -DHANDMADE_INTERNAL=1 -FC -Zi W:\native-store\code\win32_store.cpp user32.lib Gdi32.lib
popd