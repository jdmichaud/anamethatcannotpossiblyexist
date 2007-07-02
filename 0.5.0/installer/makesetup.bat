echo compiling setup package...
rm -fr wgrep.exe
rm -fr Output
copy ..\visual\msvc-8_0\Release\wgrep.exe .
"C:\Program Files\Inno Setup 5\iscc.exe" /Q /O"Output" wgrep.iss
cd Output
echo done.
