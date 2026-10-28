# 第三方组件声明 / Third-Party Notices

本项目包含以下第三方开源代码与素材。它们的版权归原作者所有，**不适用本项目的 MIT 许可证**。
保留这些文件是为了保证工程可直接编译，请勿删除其中的版权声明。

---

## 1. STM32F10x 标准外设库（StdPeriph Driver）

| 项目 | 内容 |
|---|---|
| 版权 | © 2011 STMicroelectronics |
| 许可证 | MCD-ST Liberty SW License Agreement V2（BSD-3-Clause 类） |
| 位置 | `遥控/Libraries/STM32F10x_StdPeriph_Driver/`<br/>`FreeRTOS小车/STM32F10x_FWLib/`<br/>`遥控/Libraries/CMSIS/`、`FreeRTOS小车/CORE/` |
| 说明 | 允许在保留版权声明的前提下自由使用、修改、再分发 |

## 2. FreeRTOS V9.0.0

| 项目 | 内容 |
|---|---|
| 版权 | © 2016 FreeRTOS.org / Real Time Engineers Ltd. |
| 许可证 | MIT（详见 `FreeRTOS小车/FreeRTOS/` 内各文件头部声明） |
| 位置 | `FreeRTOS小车/FreeRTOS/` |
| 说明 | 工程使用的是 V9.0.0 版本，包含多种移植层（ARM_CM3 为实际使用） |

## 3. OLED 驱动与中文字模库（江协科技）

| 项目 | 内容 |
|---|---|
| 版权 | © 江协科技（jiangxiekeji.com） |
| 许可证 | 未明确声明开源许可，官方定位为**教学例程** |
| 位置 | `遥控/APP/OLED.c`、`OLED.h`、`OLED_Data.c`、`OLED_Data.h`<br/>`FreeRTOS小车/HARDWARE/LED/` 下同名文件 |
| 说明 | 仅供**学习交流**使用。如需商业用途，请自行联系原作者取得授权。<br/>字模表按 **GB2312** 双字节索引（`OLED_CHN_CHAR_WIDTH = 2`），请勿改为 UTF-8 |

## 4. CMSIS Cortex-M3 内核文件

| 项目 | 内容 |
|---|---|
| 版权 | © ARM Limited |
| 许可证 | Apache License 2.0 |
| 位置 | `遥控/Libraries/CMSIS/core_cm3.c/h`、`FreeRTOS小车/CORE/core_cm3.c/h` |

---

## 合规建议

如果计划把这个项目**商用**，至少需要处理：

1. 移除或替换江协科技 OLED 驱动（可换成自己写的 SSD1306 驱动）
2. 保留 ST 标准外设库和 FreeRTOS 的版权声明（这两个可以商用）

如果只是**学习分享 / 课程作业 / 竞赛提交**，保留本文件和原始版权注释即可。

---

## 附：关于正点原子例程

> 说明：代码层面的处理是出于**授权**考虑，与**致谢**无关。
> 本项目在开发阶段参考了正点原子的开发板例程与 FreeRTOS 移植教程，
> 因此在 [README](README.md) 的「致谢」中已明确注明。

本项目在开发阶段使用了正点原子（ALIENTEK）的 STM32 开发板与 FreeRTOS 移植例程作为学习参考。
**开源版本中不包含其代码**，涉及的部分已重写为自行实现：

| 原文件 | 处理方式 |
|---|---|
| `FreeRTOS小车/SYSTEM/sys/sys.c/.h` | 重写：位带宏与内核指令封装改为自行实现 |
| `FreeRTOS小车/SYSTEM/delay/delay.c/.h` | 重写：SysTick 延时改为自行实现 |
| `FreeRTOS小车/SYSTEM/usart/usart.c/.h` | 重写：串口初始化与 printf 重定向改为自行实现 |
| `FreeRTOS小车/HARDWARE/LED/led.c/.h` | 重写：改为自行实现 |
| `FreeRTOS小车/readme.txt` | 删除：开发板厂商的说明文档，非本项目内容 |

以上文件保留了**原有文件名和对外 API**（`PAout`/`PBin` 等位带宏、
`delay_init`/`delay_ms`/`delay_us`、`uart_init`、`LED_Init`），
因此工程文件 `.uvprojx`、`main.c` 和各驱动都不需要改动。
