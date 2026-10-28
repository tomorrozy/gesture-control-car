# 预编译固件

这里是已经编译好的固件，**不装 Keil 也能直接烧录**。

| 文件 | 烧到 | 对应工程 | 大小 |
|---|---|---|---|
| `手柄端_remote.hex` | 手柄板 | `遥控/Template.uvprojx` | ~40 KB |
| `小车端_car.hex` | 小车板 | `FreeRTOS小车/USER/FreeRTOS.uvprojx` | ~67 KB |

## 烧录方法

### ST-Link Utility / STM32CubeProgrammer

1. 接好 ST-Link（SWDIO / SWCLK / GND / 3.3V）
2. 打开对应 `.hex`
3. 勾选 `Verify after programming`
4. 点击 `Start` / `Download`
5. 断电重新上电

### Keil

`Flash → Download`（需先打开对应工程，或直接用 `Project → Open` 后 F8）。

## ⚠️ 注意

- 固件是**某一时刻的快照**，源码改动后不会自动更新。
  自己改了代码请重新编译，不要用这里的旧固件。
- 烧录前确认**芯片容量匹配**：
  - 手柄端固件针对**中容量** STM32F103（64~128 KB Flash）
  - 小车端固件针对**高容量** STM32F103ZE（512 KB Flash）
  容量不匹配会烧录失败或运行异常。
- 手柄端上电后**保持静止 1~2 秒**，等陀螺仪零偏校准完成，否则角度会漂移。
