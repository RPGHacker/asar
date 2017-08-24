@echo off

if not exist test.exe g++ test.cpp ../libstr.cpp -otest.exe -s

if "%~1" == "" (
	set /p "rom=Please enter the path to an unheadered SMW ROM: "
) else (
	set "rom=%~1"
)

if not exist "temp" mkdir temp

if exist test.exe for %%a in (*.asm) do @test.exe "%%a" "%rom%"

if exist "temp/fail" (
	@echo One or multiple tests have failed!
	@pause
)

@rd /S /Q "temp"