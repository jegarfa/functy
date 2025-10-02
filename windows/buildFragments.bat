@ECHO OFF
cd windows

REM This file will build the WiX installer fragments for the assets, examples and gtk-lib files

ECHO Creating the gtk-lib fragments
heat dir "gtk-libs" -suid -gg -srd -sfrag -template fragment -cg GtkLibs -dr GtkLibFolder -indent 2 -var var.GtkLibDir -nologo -out gtk-libs.wxs

ECHO Creating the examples fragments
heat dir "..\examples" -suid -gg -srd -sfrag -template fragment -cg FunctyExamples -dr FunctyExamplesFolder -indent 2 -var var.ExamplesDir -nologo -out examples.wxs

ECHO Creating the assets fragments
heat dir "..\assets" -suid -gg -srd -sfrag -template fragment -cg FunctyAssets -dr FunctyAssetsFolder -indent 2 -var var.AssetsDir -nologo -t stripMakefiles.xsl -out assets.wxs
cd ..

