# libMidiPlayer
A VERY easy-to-use library for midi files playing back in programs.

# MIDIPlayer 说明
这是一个非常易用的 MIDI 文件播放库，你可以用它在程序中作为 MIDI 的 BGM 播放器。

## 特性
* 使用简单，只需几个常见的函数即可实现 MIDI 文件播放。
* 支持 A——B 区段无缝循环。（这是制作该库的主要目的，因为网上现有的库大多都是用的 MCI，这种方式并不能实现 A——B 循环，即使能循环也都是有非常严重的卡顿的）
* 可以自由控制播放与暂停。

## 测试程序
本程序的 testmain.cpp 是测试用程序，编译好后打开它，可以在程序中选择播放文件并设置循环。

## 示例 MIDI:
[東方紅魔郷ルーネイトエルフ.mid](http://pan.baidu.com/s/1c2BIp9m)

循环起始点：6680，循环结束点：98842

## 如果你不知道怎么编译库或者想直接使用
请在上面的链接中下载 MIDIPlayer.7z, 里面有完整的 lib 和测试程序。

## 许可协议
本程序使用了 [craigsapp](https://github.com/craigsapp/midifile) 的代码，使用时请遵循下面的许可。
```Text
Copyright (c) 1999-2015, Craig Stuart Sapp
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer. 
2. Redistributions in binary form must reproduce the above copyright notice,
   and the following disclaimer in the documentation and/or other materials 
   provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```
