@echo off

mkdir ..\build_db
pushd ..\build_db
cl /std:c++latest -FC -Zi W:\native-store\code\win32_db.cpp
popd