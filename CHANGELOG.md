# 更新日志 / Changelog

格式遵循 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/)，
版本号遵循 [语义化版本](https://semver.org/lang/zh-CN/)。

## [Unreleased]

### 计划中
- 无线断连超时自动停车
- 手柄端电池电压检测与低电提示

## [1.0.0] - 2026-10-28

首个公开版本，包含完整的体感遥控小车实现。

### 新增
- 手柄端：MPU6050 姿态解算 + 角度限幅 + 5 种手势映射
- 手柄端：13 字节自定义协议（帧头 / 长度 / 双精度姿态 / XOR 校验 / 帧尾）
- 手柄端：OLED 实时显示 pitch、roll 与手势编号
- 手柄端：按键长按/短按检测框架（TIM4 1 ms 扫描）
- 小车端：FreeRTOS V9.0.0 四任务架构（start / rx / ctrl / disp），互斥量保护共享数据
- 小车端：nRF24L01 多通道接收，支持接入第二个手柄
- 小车端：姿态 → 差速目标速度映射（死区 10°，转向阈值 15°）
- 小车端：左右轮独立编码器速度闭环 PID（Kp=2.0, Ki=1.5），带启动死区补偿
- 小车端：TIM2 20 kHz PWM 电机驱动，TIM3/TIM4 编码器接口
- 小车端：OLED 显示 9 种运动状态组合及目标/实际速度
- 文档：硬件接线表、通信协议说明、参数调优指南、第三方组件声明与致谢
- 固件：两端预编译 hex，可直接烧录

### 已知问题
- 无线丢包时小车会沿用上一次姿态值继续行驶，未做超时保护
- 小车端 LED1 与 nRF24L01 MOSI 复用 PA5，LED1 实际不可用
- 手柄端 DHT22 驱动已包含但未接入主流程

[Unreleased]: https://github.com/tomorrozy/gesture-control-car/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/tomorrozy/gesture-control-car/releases/tag/v1.0.0
