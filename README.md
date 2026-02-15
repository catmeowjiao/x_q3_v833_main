# !此项目为2次制作!
* 原项目地址
https://github.com/RobinNotBad/lvgl_v833_q3

## 项目部分是用AI写的
**代码写的很神奇**
~~能跑起来就行嘻嘻~~ <br>

## 运行平台
**soc 志强v833 armv7 32bit**  
推荐在**阿尔法蛋q3词典笔**上运行  

可以实现简易的文件浏览、MP3、小游戏等功能

## 怎么从项目源代码编译
* 建议使用**linux系统** 
* 克隆项目到一个空文件夹中  
* 安装交叉编译器
> 例如armhf musl gcc编译器
* 将项目文件中` build.sh `的文件路径改为你的编译器路径<br>
* 运行 ` ./build.sh `
* 目标文件为` demo `

## 其他
* 字体使用了：阿里巴巴普惠体 Medium、FontAwesome 5 Free Solid，为了在lvgl中正常使用图标，对这两个字体进行了合并。若要换用自己的字体，可以使用FontForge软件，将FontAwesome中 #61440 之后的所有图标复制到现有字体中，再进行大小缩放。


## Platform  
**Allwinner V833 ARMv7 32-bit SoC**  
Recommended to run on **Alpha Egg Q3 dictionary pen**  

It can implement simple file browsing, MP3 playback, mini-games, and other functions.

## How to compile the source code  
* It is recommended to use a **Linux system**  
* Clone the project into an empty folder  
* Install a cross-compiler  
> For example, the armhf musl gcc compiler  
* Change the file path of `build.sh` in the project to your compiler's path<br>
* Run ` ./build.sh `  
* The target file is ` demo `

## Miscellaneous  
* Fonts used: Alibaba PuHuiTi Medium, FontAwesome 5 Free Solid. To use icons properly in LVGL, these two fonts have been merged. If you want to use your own fonts, you can use FontForge to copy all icons after #61440 in FontAwesome into your existing font, and then adjust the size scaling.
