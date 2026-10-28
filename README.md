# 手势识别小车 · Gesture-Controlled Car

> 用一块 MPU6050 做体感遥控：倾斜手柄就能控制小车前进、后退、转向。STM32 + nRF24L01 无线传输，小车端基于 FreeRTOS 做双轮编码器速度闭环 PID 控制。

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/MCU-STM32F103-blue.svg)]()
[![RTOS](https://img.shields.io/badge/RTOS-FreeRTOS%20V9.0.0-green.svg)]()
[![IDE](https://img.shields.io/badge/IDE-Keil%20MDK%205-orange.svg)]()

<!-- 录好演示 GIF 后：把图放到 docs/images/demo.gif，再去掉下面这行的注释符号 -->
<!-- ![演示](docs/images/demo.gif) -->

## 项目简介

本项目是一个**分体式**的体感控制小车，由两部分组成：

- **手柄端（遥控）**：MPU6050 采集俯仰角（pitch）与横滚角（roll），映射成 5 种手势指令，通过 nRF24L01 每 100 ms 发送一帧自定义协议数据。
- **小车端（接收）**：nRF24L01 多通道接收，校验并解包，把姿态角换算成左右轮目标速度，经编码器速度闭环 PID 输出 PWM 驱动电机。

核心思路是把「姿态 → 期望速度」和「期望速度 → 实际 PWM」拆成两层，上层只管意图，下层用 PID 保证执行稳定，所以推杆角度越大车速越快，而不是简单的开关量遥控。

## 系统架构

```mermaid
flowchart LR
    subgraph H["手柄端 STM32F103（中容量）"]
        A[MPU6050<br/>姿态解算] --> B[手势映射<br/>pitch/roll → 0~5]
        B --> C[协议打包<br/>13 字节 + XOR 校验]
        C --> D[nRF24L01 发送]
        E[OLED 显示姿态/手势]
    end

    D -.2.4G 无线.-> F

    subgraph R["小车端 STM32F103ZE（FreeRTOS）"]
        F[nRF24L01 多通道接收] --> G[协议解包 + 校验]
        G --> H1[rx 任务<br/>20 ms]
        H1 --> I[ctrl 任务<br/>差速分配 + PID 速度环]
        I --> J[电机 PWM<br/>TIM2 20 kHz]
        J --> K[左右编码器<br/>TIM3 / TIM4]
        K --> I
        I --> L[disp 任务<br/>OLED 状态显示]
    end
```

## 功能特性

- ✅ **体感控制**：静态倾角映射 5 种指令（前进 / 后退 / 左转 / 右转 / 停止），阈值 15°
- ✅ **自定义通信协议**：帧头 `0xA5` + 长度 + 双精度姿态角 + XOR 校验 + 帧尾 `0x0D`，误帧自动丢弃
- ✅ **2.4G 无线**：nRF24L01，射频通道 40；发送端地址 `0x11...`，接收端支持双通道（可接两个手柄）
- ✅ **速度闭环**：左右轮独立 PID（`Kp=2.0, Ki=1.5`），编码器反馈，解决电池电压下降后速度漂移
- ✅ **实时状态显示**：OLED 显示姿态角、运动状态（9 种组合）、目标速度、实际速度
- ✅ **RTOS 架构**：FreeRTOS V9.0.0，4 个任务按优先级调度（rx 3 > ctrl 2 > disp 1），互斥量保护共享数据
- ✅ **参数可调**：死区、转向阈值、PID 参数、屏幕翻转、左右反向全部集中在 `main.c` 顶部宏定义

## 硬件清单

| 部件 | 型号 / 规格 | 数量 | 备注 |
|---|---|---|---|
| 手柄主控 | STM32F103（中容量，如 C8T6） | 1 | Keil 工程启动文件为 `startup_stm32f10x_md.s` |
| 小车主控 | STM32F103ZE 开发板 | 1 | 高容量，启动文件 `startup_stm32f10x_hd.s` |
| 姿态传感器 | MPU6050 | 1 | 软件 I2C |
| 无线模块 | nRF24L01+ | 2 | 3.3 V 供电，**需并联 10 µF 电容** |
| 显示屏 | OLED 0.96" SSD1306 | 2 | 软件 I2C，GB2312 字模 |
| 电机 | 直流减速电机 + 霍尔编码器 | 2 | 带 AB 相编码器 |
| 驱动 | L298N / TB6612 等双路 H 桥 | 1 | 方向 + PWM 接口 |
| 温湿度 | DHT22 | 1 | 驱动已包含，**当前未接入主流程** |

> 完整原理图见 [`1.原理图/`](1.原理图/)，用**立创 EDA 专业版**打开 `.epro` 文件。

## 目录结构

```
A0006-26-258-手势识别小车/
├── 遥控/                        # 手柄端 Keil 工程
│   ├── Template.uvprojx         # ← 手柄端入口工程
│   ├── APP/                     # 应用层：MPU6050 / nRF24L01 / OLED / DHT22 / 按键 / LED
│   ├── Public/                  # SysTick / TIM4 定时 / system
│   ├── Libraries/               # ST 标准外设库 + CMSIS
│   └── User/                    # main.c / 中断
├── FreeRTOS小车/                # 小车端 Keil 工程
│   ├── USER/FreeRTOS.uvprojx    # ← 小车端入口工程
│   ├── USER/main.c              # 协议解析 + PID + 4 个任务（核心逻辑都在这里）
│   ├── HARDWARE/LED/            # Motor / PWM / Encoder / OLED / nRF24L01
│   ├── SYSTEM/                  # delay / sys / usart
│   ├── FreeRTOS/                # FreeRTOS V9.0.0 源码（第三方）
│   └── STM32F10x_FWLib/         # ST 标准外设库（第三方）
├── 1.原理图/                    # 立创 EDA 工程 (.epro)
├── docs/                        # ★ 文档中心（47 篇：教程 / 图解 / 调试 / 参考）
│   ├── 00-学习路线与工程全景.md
│   ├── 01-八阶段教程/            # 阶段一 ~ 阶段八，逐阶段实现讲解
│   ├── 02-图解字典/              # 抽象概念图解（PWM、编码器、PID…）
│   ├── 03-零基础落地包/          # 器材、工具链、供电、验收门禁
│   ├── 04-调试笔记/              # 各阶段真实踩坑记录
│   └── 05-参考资料/              # 术语词典、代码模板、硬件解析
├── firmware/                    # 预编译固件（直接烧录用）
├── .gitattributes               # ⚠️ 编码配置，勿删（见下方「编码说明」）
└── README.md
```

## 文档

项目配有 **47 篇中文文档**，从零基础到完整实现全覆盖 → **[进入文档中心](docs/README.md)**

| 我想… | 看这里 |
|---|---|
| 先搞懂整体架构 | [工程全景与学习路线](docs/00-学习路线与工程全景.md) |
| 从零开始动手做 | [零基础落地包](docs/03-零基础落地包/)　器材 → 工具链 → 供电 → 验收 |
| 看某个阶段怎么实现 | [八阶段教程](docs/01-八阶段教程/)　双机通信 → … → 手势控小车 |
| 概念看不懂 | [图解字典](docs/02-图解字典/)　PWM、编码器、互补滤波、PID |
| 做完出 bug 了 | [调试笔记](docs/04-调试笔记/)　各阶段真实踩坑记录 |
| 查名词 / 记不住代码 | [参考资料](docs/05-参考资料/)　术语词典、代码模板、Yaw 零飘 |

> 硬件怎么接、协议怎么组帧，见 [`docs/硬件与接线.md`](docs/硬件与接线.md) 与 [`docs/通信协议.md`](docs/通信协议.md)。

## 快速开始

### 1. 开发环境

| 软件 | 版本 | 说明 |
|---|---|---|
| Keil MDK | 5.x | 需装 `ARM Compiler 5` 和 `STM32F1xx_DFP` 器件包 |
| STM32F10x 标准外设库 | V3.5.0 | 工程已自带，无需另外下载 |
| 下载器 | ST-Link V2 / J-Link | 手柄端和小车端都要 |

### 2. 编译与烧录

```text
手柄端：打开 遥控/Template.uvprojx           → F7 编译（0 Error）→ F8 下载
小车端：打开 FreeRTOS小车/USER/FreeRTOS.uvprojx → F7 编译（0 Error）→ F8 下载
```

不想装 Keil？直接烧 `firmware/` 里的预编译固件：

| 文件 | 烧到 | 大小 |
|---|---|---|
| `firmware/手柄端_remote.hex` | 手柄板 | ~40 KB |
| `firmware/小车端_car.hex` | 小车板 | ~67 KB |

### 3. 使用步骤

1. **先给小车端上电**，OLED 应显示「无线模块正常」；若一直显示「无线模块错误」，检查 nRF24L01 接线与供电。
2. **再给手柄端上电**，此时保持手柄**静止水平放置 1~2 秒** —— 启动时会连续做两次陀螺仪零偏校准（`MPU6050_CalibrateGyro(200)` / `(1000)`），校准时晃动会导致角度漂移。
3. 手柄水平时小车停止；前倾前进、后仰后退、左右倾斜转向，倾角越大速度越快。
4. 手柄 OLED 实时显示 `pitch / roll` 和手势编号 `G:`，小车 OLED 显示姿态、状态、目标速度、实际速度。

## 通信协议

手柄端每 **100 ms** 发送一帧，小车端只接受长度与校验都正确的帧，否则整帧丢弃。

| 偏移 | 字段 | 类型 | 说明 |
|---|---|---|---|
| 0 | `HEAD` | `uint8` | 固定 `0xA5` |
| 1 | `LEN` | `uint8` | 固定 `0x09`（有效载荷长度） |
| 2–5 | `pitch` | `float` (LE) | 俯仰角，限制 ±80°，负值取反后传输 |
| 6–9 | `roll` | `float` (LE) | 横滚角，限制 ±80° |
| 10 | `gesture` | `uint8` | 0 停止 / 1 前进 / 2 后退 / 3 左 / 4 右 / 5 停止 |
| 11 | `CHK` | `uint8` | 从 `buf[1]` 到 `buf[10]` 逐字节 XOR |
| 12 | `TAIL` | `uint8` | 固定 `0x0D` |

总长 13 字节，但无线层实际发送/接收 32 字节缓冲（剩余补 0），接收端按 13 字节解析。详见 [docs/通信协议.md](docs/通信协议.md)。

## 控制算法

### 姿态 → 目标速度

```c
死区 DEAD_ZONE = 10°     // 小于 10° 视为停止，防止手抖
转向阈值 TURN_THRESHOLD = 15°
最大速度 MAX_SPEED = 100

speed_x = (pitch ∓ 10) / (80 - 10) * 100    // 前进/后退分量
speed_y = (roll  ∓ 15) / (80 - 15) * 100    // 转向分量

targetL = speed_x + speed_y     // 差速分配
targetR = speed_x - speed_y
```

### 速度闭环 PID

每 20 ms 执行一次，左右轮独立：

```c
err      = target - actual
integral = clamp(integral + err, ±100)
out      = clamp(Kp*err + Ki*integral, ±100)   // Kp=2.0, Ki=1.5, Kd=0

actual = 编码器脉冲 × 换算系数                  // 换算见 main.c task_ctrl
```

另外加了**启动死区补偿**：目标不为 0 但 PID 输出小于 30 时强制拉到 30，克服电机静摩擦。

## 参数调优指南

所有可调参数都在各工程 `main.c` 顶部，改完重新编译即可：

| 现象 | 改哪里 | 位置 |
|---|---|---|
| 前进/后退方向反了 | `pitch = angle.Pitch * -1.0f` 去掉负号 | `遥控/User/main.c` |
| 左右转向反了 | `REVERSE_TURN` 宏 0↔1 | `FreeRTOS小车/USER/main.c` |
| OLED 显示上下颠倒 | `SCREEN_FLIP` 宏 0↔1 | `FreeRTOS小车/USER/main.c` |
| 响应太灵敏 / 太迟钝 | `DEAD_ZONE`、`TURN_THRESHOLD` | `FreeRTOS小车/USER/main.c` `task_ctrl` |
| 车速上不去 / 抖动 | PID 的 `Kp`、`Ki` | `main()` 里 `PID_Init` |
| 小车低速不动 | 启动补偿阈值 `30` | `task_ctrl` 中两处 |
| 角度漂移 | 开机静止时间加长 / 校准次数 | `MPU6050_CalibrateGyro` 调用处 |

## 已知限制

- ⚠️ **断连不会自动停车**：无线丢包时 `task_ctrl` 会继续使用上一次的姿态值，小车会保持原速前进。实用化建议在 `task_rx` 里加超时判断（例如超过 500 ms 没收到新帧就 `g_pitch = g_roll = 0`）。
- 只做**静态倾角**映射，不是真正的手势轨迹识别；快速挥动手柄不会触发额外指令。
- 手柄端 `DHT22` 驱动已包含但未接入主流程，属于预留功能。
- PID 只用 P/I，未使用 D 项；编码器换算系数是针对当前电机实测的，换电机需重新标定。

## 编码说明（重要）

本项目源码是**混合编码**：大部分文件是 UTF-8，但 OLED 驱动、`main.c`、nRF24L01 驱动等 **43 个文件是 GBK/GB2312**。

原因是 OLED 的 `OLED_CHN_CHAR_WIDTH = 2`，中文字模表按 **GB2312 双字节**索引，改成 UTF-8 会导致 OLED 中文全部显示错误。

仓库通过 `.gitattributes` 的 `working-tree-encoding=GBK` 解决：**GitHub 上以 UTF-8 存储（中文正常显示），检出到本地自动转回 GBK 原始字节**（Keil 编译不受影响）。

因此：

- ❌ 不要用 GitHub 网页或 VS Code 把 GBK 文件另存为 UTF-8
- ✅ 新增文件如果是 Keil 创建的（GBK），请把路径加进 `.gitattributes` 的 GBK 列表
- ✅ 只改注释文字不影响编译，但请保持编码一致

## 常见问题

<details>
<summary><b>编译报错 "cannot open source input file xxx.h"</b></summary>

Keil 的 Include Paths 是相对路径，确认你没有单独移动 `Libraries/`、`SYSTEM/` 等目录。用 `Project → Options → C/C++ → Include Paths` 检查。
</details>

<details>
<summary><b>OLED 显示乱码或中文变方块</b></summary>

字模表是 GB2312 的。检查 `OLED_Data.h` 里 `OLED_CHN_CHAR_WIDTH` 是否为 `2`，以及源文件编码是否被改成了 UTF-8（见上面「编码说明」）。
</details>

<details>
<summary><b>小车 OLED 一直显示「无线模块错误」</b></summary>

1. nRF24L01 必须 **3.3 V** 供电，5 V 会烧模块
2. 模块电源脚并联 10 µF 电容，否则握手偶尔失败
3. 检查 SPI 六根线，特别是 MISO/IRQ 是输入
</details>

<details>
<summary><b>手柄能显示角度，但小车不动</b></summary>

先看手柄 OLED 的 `G:` 数值是否随姿态变化；再看两端 nRF24L01 是否在同一射频通道（代码里 `RF_CH = 40`），以及手柄发送地址是否与小车接收通道 0 地址（`0x11 0x11 0x11 0x11 0x11`）一致。
</details>

<details>
<summary><b>小车跑起来左右轮速度明显不一致</b></summary>

两个电机的减速比和编码器线数可能不同，需要分别标定 `task_ctrl` 里 `g_act_left` / `g_act_right` 的换算系数。
</details>

## 参与贡献

欢迎提 Issue 和 PR。改代码前请先看 [CONTRIBUTING.md](CONTRIBUTING.md)。

## 许可证

本项目**自己编写**的代码基于 [MIT License](LICENSE) 发布。

但项目内含第三方组件（ST 标准外设库、FreeRTOS、江协科技 OLED 驱动等），它们的版权归各自作者，不适用 MIT，详见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。

**仅供学习交流使用。** 如果你要商用，请自行确认第三方库的授权。

## 致谢

特别感谢以下项目与作者，本项目的完成离不开他们的工作：

- [正点原子 ALIENTEK](https://www.openedv.com/)（广州市星翼电子科技有限公司）
  —— 小车端使用其「精英 STM32F103」开发板，FreeRTOS 的移植与任务调度也是对照其教程学习的。
  本项目中源自该校例的代码（`SYSTEM` 层与板载 LED 驱动）已全部重写为自行实现，
  详见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)，但学习过程中的帮助是实实在在的。
- [江协科技](https://jiangxiekeji.com/) —— OLED 驱动与 GB2312 字模库
- [FreeRTOS](https://www.freertos.org/) —— 实时操作系统 V9.0.0
- [STMicroelectronics](https://www.st.com/) —— STM32F10x 标准外设库

---

如果这个项目对你有帮助，点个 ⭐ Star 支持一下。
