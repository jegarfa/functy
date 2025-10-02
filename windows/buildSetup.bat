@ECHO OFF
IF "%~1"=="" ECHO Error: Must specify a version number as a parameter. E.g. "buildSetup 1.0.0"
IF "%~1"=="" GOTO End
SET productversion=%1

REM Unpack the GTK libraries if they're not already there
IF NOT EXIST windows\gtk-libs ECHO Extract the gtk-libs.zip archive into the windows\gtk-libs folder first.
IF NOT EXIST windows\gtk-libs GOTO End

cd windows\wixui

REM Create the msi installer user interface
ECHO.
ECHO Creating MSI installer user interface

candle.exe -nologo functy\WixUI_Functy.wxs *.wxs
lit.exe -nologo -out WixUI_Functy.wixlib *.wixobj

cd ..\..

REM Create the MSI installer
ECHO.
ECHO Compiling MSI resources

candle windows\functy.wxs -nologo -out windows\functy.wixobj
candle windows\assets.wxs -nologo -out windows\assets.wixobj -dAssetsDir="assets"
candle windows\gtk-libs.wxs -nologo -out windows\gtk-libs.wixobj -dGtkLibDir="windows\gtk-libs"
candle windows\examples.wxs -nologo -out windows\examples.wixobj -dExamplesDir="examples"

ECHO.
ECHO Linking MSI installer

light.exe -nologo -out functy-%productversion%-win32-bin.msi windows\functy.wixobj windows\assets.wixobj windows\examples.wixobj windows\gtk-libs.wixobj "windows\wixui\WixUI_Functy.wixlib" -loc "windows\wixui\WixUI_en-us.wxl"

REM No BootStrapper is needed since the GTK libs are now installed directly with the program

REM Create the BootStrapper installer
REM ECHO Creating BootStrapper installer

REM candle windows\bootstrap\bootstrap.wxs -out windows\bootstrap\bootstrap.wixobj
REM light -out FunctySetup-%productversion%-win32-bin.exe -ext WixBalExtension windows\bootstrap\bootstrap.wixobj

:End
