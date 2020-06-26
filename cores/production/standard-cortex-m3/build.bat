rem Windows build script for FX-RTOS kernel.
rem By default it uses GNU toolchain. To select another one use TOOLCHAIN env
rem variable: set it to KEIL or IAR to use Keil MDK or IAR EWARM respectively.

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
	set MAP_FILE=default.map
)

if "%TOOLCHAIN%"=="IAR" (
	set FX_PREP=pr.bat %%s %%s
	set CC=iccarm --cpu=Cortex-M3 -e --fpu=None --endian=little --preinclude includes.inc -I. -o
	set AS=iasmarm -s+ -r --cpu Cortex-M3 --fpu None -I. -o
	set AR=iarchive --create ../libfxrtos.a
) else (
	if "%TOOLCHAIN%"=="KEIL" (
		set FX_PREP=armcc -I src -E --preinclude %%s %%s
		set CC=armcc --cpu Cortex-M3 --c99 --preinclude ./includes.inc -I. -c -o
		set AS=armasm --cpu Cortex-M3 --cpreproc --cpreproc_opts=--preinclude,includes.inc -I. -o
		set AR=armar -r ../fxrtos.lib
	) else (
		if "%MAP_FILE%"=="m4f.map" (
			set CPU_OPTS=-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard
		) else (
			set CPU_OPTS=-mcpu=cortex-m3
		)
		set FX_PREP=%GCC_PREFIX%gcc -E -Isrc -include %%s %%s
		set CC=%GCC_PREFIX%gcc -pedantic -std=c99 -O2 -Wall -ffunction-sections -include includes.inc %CPU_OPTS% -mthumb -I. -c -o
		set AS=%GCC_PREFIX%gcc %CPU_OPTS% -mthumb -I. -include includes.inc -c -o
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