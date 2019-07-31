@echo off
if "%1"=="C" goto Clean
if "%1"=="c" goto Clean
setlocal EnableDELAYEDEXPANSION
set _MSBuild_=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe
if not exist "!_MSBuild_!" goto _NO_MSBuild_
set _ProjectName_=midifile.vcxproj
title 编译 %_ProjectName_%···
"%_MSBuild_%" %_ProjectName_% /p:Configuration=Debug;Platform=x86
"%_MSBuild_%" %_ProjectName_% /p:Configuration=Debug;Platform=x64
"%_MSBuild_%" %_ProjectName_% /p:Configuration=Release;Platform=x86
"%_MSBuild_%" %_ProjectName_% /p:Configuration=Release;Platform=x64
set _ProjectName_=MIDIPlayer.vcxproj
title 编译 %_ProjectName_%···
"%_MSBuild_%" %_ProjectName_% /p:Configuration=Debug;Platform=x86
"%_MSBuild_%" %_ProjectName_% /p:Configuration=Debug;Platform=x64
"%_MSBuild_%" %_ProjectName_% /p:Configuration=Release;Platform=x86
"%_MSBuild_%" %_ProjectName_% /p:Configuration=Release;Platform=x64
title 已完成，请检查窗口中的输出结果。
goto End
:_NO_MSBuild_
echo 你需要安装 VS2019 才能运行该脚本。
goto End
:End
pause
goto EOL
:Clean
cd midifile
rd /s /q lib
rd /s /q lib_d
rd /s /q lib_x64
rd /s /q lib_x64_d
rd /s /q Debug
rd /s /q Release
rd /s /q x64_Debug
rd /s /q x64_Release
cd..
rd /s /q Debug
rd /s /q lib
rd /s /q lib_d
rd /s /q lib_x64
rd /s /q lib_x64_d
rd /s /q Release
rd /s /q x64
echo 完成。
ping -n 2 127.8>nul
:EOL
