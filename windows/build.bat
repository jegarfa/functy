@ECHO OFF
IF "%~1"=="" ECHO Error: Must specify a version number as a parameter. E.g. "build 1.0.0"
IF "%~1"=="" GOTO End
SET productversion=%1

REM Add -mconsole to both gcc calls for console output
cd src

del *.o

gcc -DFUNCTYDIR=\"./assets\" -DVERSION=\"%productversion%\" -c -Wall -mwindows -mms-bitfields -I"%GTK_BASEPATH%\include\gtk-2.0" -I"%GTK_BASEPATH%\include\cairo" -I"%GTK_BASEPATH%\include\glib-2.0" -I"%GTK_BASEPATH%\include\pango-1.0" -I"%GTK_BASEPATH%\lib\gtk-2.0\include" -I"%GTK_BASEPATH%\lib\glib-2.0\include" -I"%GTK_BASEPATH%\include\atk-1.0" -I"%GTK_BASEPATH%\include" -I"%GTK_BASEPATH%\include\gtkglext-1.0" -I"%GTK_BASEPATH%\lib\gtkglext-1.0\include" -I"%GTK_BASEPATH%\include\gdk-pixbuf-2.0" -I"..\..\freeglut\include" -I"..\..\GLee" -I"..\..\Symbolic\src" -I"..\..\libzip\lib" *.c

g++ -DFUNCTYDIR=\"./assets\" -DVERSION=\"%productversion%\" -DOPENVDB_OPENEXR_STATICLIB=\"1\" -UOPENEXR_DLL -DHALF_EXPORTS=\"1\" -c -w -mwindows -mms-bitfields -I"%GTK_BASEPATH%\include\gtk-2.0" -I"%GTK_BASEPATH%\include\cairo" -I"%GTK_BASEPATH%\include\glib-2.0" -I"%GTK_BASEPATH%\include\pango-1.0" -I"%GTK_BASEPATH%\lib\gtk-2.0\include" -I"%GTK_BASEPATH%\lib\glib-2.0\include" -I"%GTK_BASEPATH%\include\atk-1.0" -I"%GTK_BASEPATH%\include" -I"%GTK_BASEPATH%\include\gtkglext-1.0" -I"%GTK_BASEPATH%\lib\gtkglext-1.0\include" -I"%GTK_BASEPATH%\include\gdk-pixbuf-2.0" -I"..\..\freeglut\include" -I"..\..\GLee" -I"..\..\Symbolic\src" -I"..\..\libzip\lib" -I"..\..\openvdb" -I"..\..\openvdb\boost" -I"..\..\openvdb\ilmbase\Half" -I"..\..\openvdb\tbb\include" *.cpp

dlltool --output-def functy.def functy.o
dlltool --dllname functy.exe --def functy.def --output-exp functy.exp

g++ -DUSE_GLADE -g -O2 -mwindows -static *.o functy.exp -L"%GTK_BASEPATH%\lib" -Wl,-luuid -lgtkglext-win32-1.0 -lgdkglext-win32-1.0 -lglu32 -lGLee -luser32 -lkernel32 -lopengl32 -lgtk-win32-2.0 -lglib-2.0 -lgdk-win32-2.0 -lgdk_pixbuf-2.0 -limm32 -lshell32 -lole32 -latk-1.0 -lpangocairo-1.0 -lcairo -lpangoft2-1.0 -lpangowin32-1.0 -lgdi32 -lz -lpango-1.0 -lgobject-2.0 -lm -lgmodule-2.0 -lgio-2.0 -L"..\..\freeglut\lib" -lfreeglut -L"..\..\GLee" -L"..\..\libzip\lib" -lzip.dll -lintl -llibpng -L"..\..\Symbolic" -lsymbolic -L"..\..\openvdb" -lhalf -lopenvdb ..\..\openvdb\tbb.dll ..\..\openvdb\zlib1.dll -L"..\..\openvdb\boost\stage\lib" -lboost_system-mgw48-mt-1_58 -lboost_iostreams-mgw48-mt-1_58 -o ..\functy.exe
cd ..

:End
