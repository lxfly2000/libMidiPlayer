@echo off
set _MSBuild_=%ProgramFiles(x86)%\MSBuild\14.0\Bin\MSBuild.exe
setlocal EnableDELAYEDEXPANSION
if not exist "!_MSBuild_!" goto _NO_MSBuild_
cd midifile
set _ProjectName_=midifile.vcxproj
title ���� %_ProjectName_%������
"%_MSBuild_%" %_ProjectName_% /p:Configuration=Debug;Platform=x86
"%_MSBuild_%" %_ProjectName_% /p:Configuration=Debug;Platform=x64
"%_MSBuild_%" %_ProjectName_% /p:Configuration=Release;Platform=x86
"%_MSBuild_%" %_ProjectName_% /p:Configuration=Release;Platform=x64
cd..
set _ProjectName_=MIDIPlayer.vcxproj
title ���� %_ProjectName_%������
"%_MSBuild_%" %_ProjectName_% /p:Configuration=Debug;Platform=x86
"%_MSBuild_%" %_ProjectName_% /p:Configuration=Debug;Platform=x64
"%_MSBuild_%" %_ProjectName_% /p:Configuration=Release;Platform=x86
"%_MSBuild_%" %_ProjectName_% /p:Configuration=Release;Platform=x64
set _ProjectName_=TestMidiPlayer.vcxproj
title ���� %_ProjectName_%������
"%_MSBuild_%" %_ProjectName_% /p:Configuration=Release;Platform=x64
title ����ɣ����鴰���е���������
goto End
:_NO_MSBuild_
echo ����Ҫ��װ VS2015 �������иýű���
goto End
:End
pause
