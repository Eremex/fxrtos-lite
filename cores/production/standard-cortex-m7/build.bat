rem Windows build script for FX-RTOS kernel.
rem By default it uses GNU toolchain. To select another one use TOOLCHAIN env
rem variable: set it to ARM or IAR to use ARM Keil MDK or IAR EWARM respectively.

@echo off
echo Cleaning up...

if exist src\*.o del /F /Q src\*.o
if exist *fxrtos.* del /F /Q *fxrtos.*
if exist *.tmp del /F /Q *.tmp

if "%1"=="clean" (
    echo OK
	exit /b 0
) 

if "%MAP_FILE%"=="" (
	if exist default.map (
		set MAP_FILE=default.map
	) else (
		set MAP_FILE=lite.map
	)
)

if "%TOOLCHAIN%"=="IAR" (
	set FX_PREP=pr.bat %%s %%s
	set CC=iccarm --cpu Cortex-M7 --fpu VFPv5_sp -e --endian=little --preinclude includes.inc -I. -o
	set AS=iasmarm -s+ -r --cpu Cortex-M4 --fpu VFPv5_sp -I. -o
	set AR=iarchive --create ../libfxrtos.a
) else (
	if "%TOOLCHAIN%"=="ARM" (
		set FX_PREP=armcc -I src -E --preinclude %%s %%s
		set CC=armcc --cpu Cortex-M7 --fpu=fpv5-sp --c99 --preinclude ./includes.inc -I. -c -o
		set AS=armasm --cpu Cortex-M7 --fpu=fpv5-sp --cpreproc --cpreproc_opts=--preinclude,includes.inc -I. -o
		set AR=armar -r ../fxrtos.lib
	) else (
		set FX_PREP=%GCC_PREFIX%gcc -E -Isrc -include %%s %%s
		set CC=%GCC_PREFIX%gcc -mcpu=cortex-m7 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -pedantic -std=c99 -O2 -include includes.inc -I. -Wall -ffunction-sections -c -o
		set AS=%GCC_PREFIX%gcc -mcpu=cortex-m7 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -I. -include includes.inc -c -o
		set AR=%GCC_PREFIX%ar rcs ../libfxrtos.a
	)
)

if not exist src (	
	mkdir src
	echo Performing dependency injection...
	%FXDJ% -p .,%FXRTOS_DIR%\components -t FXRTOS -a %MAP_FILE% -o src -l src\fxrtos.lst
	echo #define FX_METADATA^(data^) > src/includes.inc
	echo #define FX_INTERFACE^(hdr^) ^<hdr.h^> >> src/includes.inc	
)

if "%1" == "srccopy" (
    echo OK
	exit /b 0
)

call set OBJS=
cd src
echo Compiling...

for %%f in (*.c) do (
	echo Compiling %%f...
	%CC% %%~nf.o %%f
	call set OBJS=%%OBJS%% %%~nf.o
)

for %%f in (*.S) do (
	echo Assembling %%f...
	%AS% %%~nf.o %%f
	call set OBJS=%%OBJS%% %%~nf.o
)

echo Creating library...
%AR% %OBJS%
cd ..

echo Creating common header...
echo #define FX_METADATA^(data^) > FXRTOS.h
echo #define FX_INTERFACE^(file^) ^<stdint.h^> >> FXRTOS.h
for /F "tokens=*" %%A in (src\fxrtos.lst) do type src\%%A.h >> FXRTOS.h
