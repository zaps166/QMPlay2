@echo off

rem  mkdir lang
rem  lupdate -locations none QMPlay2.pro
lrelease QMPlay2.pro
mkdir app\lang
move lang\*.qm app\lang

qmake && mingw32-make release && del app\*.a && del app\modules\*.a & pause
