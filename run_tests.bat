set ABS=%~dp0
set ABS=%ABS:~0,-1%
set CONFIG=Release
mkdir tests_tmp_dll tests_tmp_app
%ABS%\asar-tests\%CONFIG%\asar-dll-test.exe %ABS%\asar\%CONFIG%\asar.dll %ABS%\tests %ABS%\dummy_rom.sfc %ABS%\tests_tmp_dll
%ABS%\asar-tests\%CONFIG%\asar-app-test.exe %ABS%\asar\%CONFIG%\asar-standalone.exe %ABS%\tests %ABS%\dummy_rom.sfc %ABS%\tests_tmp_app
@pause