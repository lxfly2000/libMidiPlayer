因为我做的另一个程序中需要用静态库，所以就做了这个静态版本的 libMidiPlayer.
---
# libMidiPlayer
A VERY easy-to-use library for midi files playing back in programs.

# LICENSE
You can use this library in any case with the following copyright information included in your products.
```Text
libMidiPlayer  Copyright (C) 2016 lxfly2000
```
Please notice that this library is provided with NO WARRANTY.

# MIDIPlayer 说明
这是一个非常易用的 MIDI 文件播放库，你可以用它在程序中作为 MIDI 的 BGM 播放器。

## 特性
* 使用简单，只需几个常见的函数即可实现 MIDI 文件播放。
* 支持 A——B 区段无缝循环。（这是制作该库的主要目的，因为网上现有的库大多都是用的 MCI，这种方式并不能实现 A——B 循环，即使能循环也都是有非常严重的卡顿的）
* 可以自由控制播放与暂停。

## 使用方法
静态库编译好后，在你的项目中进行以下设置 (Visual Studio, 以下提到的路径均为 MIDIPlayer 的相对路径):
* C/C++, 常规，附加包含目录：`libMidiPlayer`; `libMidiPlayer\midifile\include`
* 链接器，常规，附加库目录：
   * x86 Debug: `libMidiPlayer\lib_d`; `libMidiPlayer\midifile\lib_d`
   * x86 Release: `libMidiPlayer\lib`; `libMidiPlayer\midifile\lib`
   * x64 Debug: `libMidiPlayer\lib_x64_d`; `libMidiPlayer\midifile\lib_x64_d`
   * x64 Release: `libMidiPlayer\lib_x64`; `libMidiPlayer\midifile\lib_x64`
* 链接器，输入，附加依赖项：`MidiPlayer.lib`; `midifile.lib`; `WinMM.lib`

然后在代码中引用 `MidiPlayer.h`, 定义一个 MidiPlayer 变量即可。
```C++
#include "MidiPlayer.h"
int main()
{
   MidiPlayer player;
   player.LoadFile("bgm.mid");
   player.SetLoop(6680, 98842);
   player.Play();
   //……
}
```

## 测试程序
本程序的 testmain.cpp 是测试用程序，编译好后打开它，可以在程序中选择播放文件并设置循环。

## 应用程序示例
* [VisualMIDIPlayer](https://pan.baidu.com/s/1b0Tkce)
