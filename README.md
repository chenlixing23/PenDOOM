# PenDOOM - PureDOOM Port for YoudaoDictionaryPen 2

![PenDOOM 实机运行演示](images/show.jpg)

---

###  中文

这个项目是对于有道词典笔二代适配的 DOOM 引擎移植版本，基于PureDOOM单头文件开发，对于320x170屏幕上下两边稍微有裁剪。

## 操作方式

本项目使用OTG键盘操作，↑↓←→控制方向，数字切枪，Ctrl开枪,左Shift奔跑，Esc菜单/暂停,空格开门等。

## 编译

确保你在开发机环境（如 Debian/Ubuntu）中已安装 `aarch64-linux-gnu-gcc` 交叉编译工具链。

在包含 `PenDOOM.c` 和 `PureDOOM.h` 的目录下执行：

```bash
aarch64-linux-gnu-gcc -static -O2 -s -o PenDOOM PenDOOM.c -lm
```

## 运行

将编译生成的PenDOOM与游戏文件（.wad）放于一个目录下赋予权限并运行

```bash
chmod +x PenDOOM
./PenDOOM -iwad 游戏文件.wad
```

📄 协议与致谢

本项目基于 PureDOOM 单头文件库构建。该项目遵循 id Software 的原始 DOOM 源码协议，仅供个人学习和研究使用，严禁用于任何商业盈利行为。

---

###  English

This project is a port of the DOOM engine for the Youdao Dictionary Pen 2, developed based on the PureDOOM single-header library. The original image is slightly cropped at the top and bottom to fit the 320x170 screen.

## Controls

This project uses an OTG keyboard for control. Use the arrow keys (↑↓←→) for movement, number keys for weapon switching, Ctrl to fire, Left Shift to run, Esc for the menu/pause, and Space to open doors, etc.

## Compilation

Make sure you have the `aarch64-linux-gnu-gcc` cross-compilation toolchain installed on your development machine (e.g., Debian/Ubuntu).

Execute the following command in the directory containing `PenDOOM.c` and `PureDOOM.h`:

```bash
aarch64-linux-gnu-gcc -static -O2 -s -o PenDOOM PenDOOM.c -lm
```

## Running

Place the compiled `PenDOOM` binary and your game WAD file in the same directory, grant execution permissions, and run it.

```bash
chmod +x PenDOOM
./PenDOOM -iwad your_game.wad
```

📄 License & Acknowledgments

This project is built upon the PureDOOM single-header library. It follows id Software's original DOOM source code license and is intended for personal learning and research purposes only. Commercial use is strictly prohibited.