"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\msbuild.exe" EuroscopeACARS.vcxproj /p:Configuration=Release /p:Platform=win32
copy Release\EuroscopeACARS.dll .
rd /s /q EuroscopeACARS Release