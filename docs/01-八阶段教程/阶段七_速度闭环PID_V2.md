---
title: 阶段七 主教程 速度闭环PID
tags:
  - RM电控
  - STM32/阶段七
  - STM32/PID
  - STM32/闭环
version: V2
updated: 2026-10-23
code_base:
  - FreeRTOS小车/USER/main.c
  - FreeRTOS/include/FreeRTOSConfig.h
---

> [!danger] 🚨 版本交接指令
> 请将旧版 `阶段七_主教程_速度闭环PID_V1.md` 移入回收站，保存此 `_V2` 文件，并把 `Debug_阶段七_速度闭环PID_V1.md` 与全库双链改指本文件。

# 阶段七 · 主教程 · 速度闭环 PID

> [!tip] 配套错题本
> 本阶段排错专用：[[Debug_阶段七_速度闭环PID_V2]]

---

## 📡 本章前置知识点雷达

> [!info] 预习指南：在啃代码之前，先认识这些“硬核名词”
> 本章围绕以下 5 个概念展开。

| 知识点名称 | 一句话通俗解释 | 在本章代码中的角色 |
| :--- | :--- | :--- |
| **PI 控制** | 按误差实时修正：P 管快慢、I 消误差。 | `PID_Calc`（只有 P、I） |
| **积分饱和** | 误差长期同号，积分越堆越大、输出顶死。 | `PID_INT_MAX=100.0f` 夹紧 |
| **采样周期** | 每 20 ms 算一次，误差是离散的。 | `vTaskDelay(pdMS_TO_TICKS(20))` |
| **位置式 PI** | 直接算“输出该是多少”。 | `out = kp*e + ki*Σe` |
| **波形整定** | 把目标/实际画成曲线看好坏。 | 工程里没有（本章给补法） |

---

## 一、本阶段任务与得分点

| 任务 | 要求 | 现状 |
|:--:|---|---|
| 1 | 速度闭环（固定目标）+ VOFA 整定 | 闭环有；VOFA 打印 0 处 |
| 2 | 目标可调（按键/串口） | 目标来自姿态连续映射 |
| 3 | 手势控速（IMU 映射） | 已实现（姿态→target→PID） |

---

## 二、代码结构总览

| 位置 | 内容 | 行号 |
|---|---|---|
| 全局中间量 | `pwm_L_temp`、`pwm_R_temp` | 20–21 |
| PID 常量 | `PID_OUT_MAX=100`、`PID_INT_MAX=100` | 59–60 |
| PID 结构体 | `kp, ki, kd, target, integral, prev_err` | 62–67 |
| 全局实例与副本 | `pid_left/right`、`g_target_*`、`g_act_*`、`g_pwm_disp_*` | 100–103 |
| 初始化 | `PID_Init(..., 2.0f, 1.50f, 0.0f)` | 166–167 |
| 计算 | `PID_Calc` | 77–90 |
| 周期 | 20 ms | 303 |
| Tick | `configTICK_RATE_HZ = 1000` | `FreeRTOSConfig.h:97` |

```text
target(0~100) ─→ err = target - feedback ─→ integral += err（夹 ±100）
                    │                          │
                    └→ out = kp*err + ki*integral ┘（夹 ±100）
                              ↓
                    起转补偿（±30 下限）→ Motor_SetPWM
                              ↓
                    编码器 → g_act（feedback）
```

---

## 三、逐行精讲

### 3.1 PID 常量与结构体（main.c 59–67 行）

```c
#define PID_OUT_MAX   100.0f
#define PID_INT_MAX   100.0f    

typedef struct {
    float kp, ki, kd;
    float target;
    float integral;
    float prev_err;
} PID_t;
```

【第 59 行】输出上限 100，对应 PWM 满量程。
【第 60 行】积分上限 100。
【第 62 行】结构体定义。
【第 63 行】三个系数（`kd` 存了但公式没用）。
【第 64 行】目标值，每周期外部写入。
【第 65 行】积分累加值。
【第 66 行】上次误差（只写不读）。
【第 67 行】类型名 `PID_t`。

> [!abstract] 人话翻译
> 结构体看着是完整 PID，实际只用 P 和 I；`PID_t` 有两份实例（左右各一），互不干扰。

### 3.2 PID 初始化（main.c 69–75 行）

```c
static void PID_Init(PID_t *p, float kp, float ki, float kd)
{
    p->kp = kp; p->ki = ki; p->kd = kd;
    p->target = 0;
    p->integral = 0;
    p->prev_err = 0;
}
```

【第 69 行】初始化函数。
【第 71 行】写三个系数。
【第 72 行】目标清零。
【第 73 行】积分清零——上电不飞车的关键。
【第 74 行】上次误差清零。

> [!abstract] 人话翻译
> 最重要的两个清零点：目标 0、积分 0；积分不清，上电瞬间会猛冲。

### 3.3 PID 计算（main.c 77–90 行）

```c
static float PID_Calc(PID_t *p, float feedback)
{
    float err = p->target - feedback;
    p->integral += err;
    if (p->integral >  PID_INT_MAX) p->integral =  PID_INT_MAX;
    if (p->integral < -PID_INT_MAX) p->integral = -PID_INT_MAX;

    float out = p->kp * err + p->ki * p->integral;
    p->prev_err = err;

    if (out > PID_OUT_MAX) out = PID_OUT_MAX;
    if (out < -PID_OUT_MAX) out = -PID_OUT_MAX;
    return out;
}
```

【第 77 行】计算函数：目标在结构体、反馈是入参。
【第 79 行】误差 = 目标 − 实际。
【第 80 行】积分累加误差（**没有乘周期**）。
【第 81–82 行】积分夹 ±100。
【第 84 行】`out = kp*err + ki*integral`——只有 P 和 I，**没有 D**。
【第 85 行】保存误差（之后没人读）。
【第 87–88 行】输出夹 ±100。
【第 89 行】返回输出。

> [!abstract] 人话翻译
> 标准位置式 PI：误差大就多给，误差长期存在就用积分补。两个夹紧是安全网，但**没有“输出饱和时暂停积分”**的抗饱和逻辑。

### 3.4 参数是怎么来的（main.c 165–167 行）

```c
    /* ★ 重点修改：把 Ki 从 0.2 提到 1.5。它能让 PWM 强顶着上 100 */
    PID_Init(&pid_left,  2.0f, 1.50f, 0.0f);
    PID_Init(&pid_right, 2.0f, 1.50f, 0.0f);
```

【第 165 行】注释记录调参历史：Ki 从 0.2 提到 1.5。
【第 166 行】左轮 Kp=2、Ki=1.5、Kd=0。
【第 167 行】右轮同参数。

> [!abstract] 人话翻译
> 加大 Ki 是为了“输出能顶到 100%”：稳态误差很小时，`out ≈ 1.5 × integral`，积分堆到约 66.7 就顶满。

### 3.5 采样周期（main.c 303 行 + FreeRTOSConfig.h 97 行）

```c
        vTaskDelay(pdMS_TO_TICKS(20));
```

```c
#define configTICK_RATE_HZ						(1000)
```

【main.c:303】控制任务每轮延时 20 ms。
【FreeRTOSConfig.h:97】节拍 1000 Hz → 1 tick = 1 ms → 20 tick = 20 ms。

> [!warning] 周期不是“精确 20 ms”
> `vTaskDelay` 只保证“至少等 20 ms”，被高优先级任务打断会更长。
> `integral += err` 没乘周期 → 周期一变积分强度就变：`Ki_cont = ki / T = 1.5 / 0.02 = 75`，改 10 ms 就变 150。

> [!abstract] 人话翻译
> 你调的“1.5”只在 20 ms 下有意义；换周期不重调 Ki，等于偷偷改了参数。

### 3.6 反馈量（main.c 261–266 行）

```c
        encL = Encoder_Get(1);
        encR = Encoder_Get(2);

        /* 将 0~20 映射为 0~100 */
        g_act_left  = (encL / 52.0f / 0.02f / 9.27666f) * 5.0f;
        g_act_right = (encR / 52.0f / 0.02f / 9.27666f) * 5.0f;
```

【第 261–262 行】读 20 ms 增量（读完清零）。
【第 265–266 行】换算成 0~100 标度。

> [!warning] 用 1:13 核对（算术推导）
> 代码常数 `52 = 4 × 线数` → 反推线数 **13**。⚠️ 你确认的是**减速比 1:13**；线数尚未确认（嘉立创 BOM 我没读到），故“13 线”是**反推值**。
> 代码除的是 **9.27666** → 反馈偏大 `13/9.27666 ≈ 1.40`（高估约 40%）；输出轴每圈计数 = 52×13 = **676**。
> 修法：两处改 `13.0f`，然后重调 Ki。

> [!abstract] 人话翻译
> 反馈与目标必须同一把尺子；尺子刻度错了，PID 也能“收敛”，只是收敛到的真实速度与显示对不上。

### 3.7 交目标、算输出（main.c 268–272 行）

```c
        pid_left.target  = targetL;
        pid_right.target = targetR;

        pwm_L_temp = PID_Calc(&pid_left,  g_act_left);
        pwm_R_temp = PID_Calc(&pid_right, g_act_right);
```

【第 268–269 行】把左右目标写进各自 PID。
【第 271–272 行】用实际反馈算 PWM。

> [!abstract] 人话翻译
> 左右是两条独立闭环：目标同源、反馈独立；好处是一轮打滑不拖死另一轮，坏处是没有直行同步。

### 3.8 起转补偿与输出限幅（main.c 274–293 行）

```c
        if (targetL > 0.1f) {
            if (pwm_L_temp > 0 && pwm_L_temp < 30) pwm_L_temp = 30; 
        } else if (targetL < -0.1f) {
            if (pwm_L_temp < 0 && pwm_L_temp > -30) pwm_L_temp = -30;
        } else {
            pwm_L_temp = 0;
        }

        if (targetR > 0.1f) {
            if (pwm_R_temp > 0 && pwm_R_temp < 30) pwm_R_temp = 30;
        } else if (targetR < -0.1f) {
            if (pwm_R_temp < 0 && pwm_R_temp > -30) pwm_R_temp = -30;
        } else {
            pwm_R_temp = 0;
        }

        if (pwm_L_temp > 100)  pwm_L_temp = 100;
        if (pwm_L_temp < -100) pwm_L_temp = -100;
        if (pwm_R_temp > 100)  pwm_R_temp = 100;
        if (pwm_R_temp < -100) pwm_R_temp = -100;
```

【第 274–275 行】左轮正目标但输出太小 → 抬到 30。
【第 276–277 行】左轮负方向同理 → −30。
【第 278–280 行】目标≈0 → 输出 0。
【第 282–288 行】右轮完全同构。
【第 290–293 行】再夹 ±100。

> [!abstract] 人话翻译
> 电机低占空比转不动，所以规定“想走最少给 30%”。代价是最低速锁死 30%，低跟随调不准。
> 注意前提：补偿只在**输出符号与目标一致**时生效（`pwm>0` 且 `target>0.1`）；反馈接反导致输出反号时，这段救不了场。

### 3.9 输出到电机（main.c 295–296 行）

```c
        Motor_SetPWM(1, (int16_t)pwm_L_temp);
        Motor_SetPWM(2, (int16_t)pwm_R_temp);
```

【第 295 行】左轮：方向 + 占空比。
【第 296 行】右轮同理。

> [!abstract] 人话翻译
> PID 输出到这里变成“方向 + PWM”；前面所有限幅都是为了让参数不越界（`Motor_SetPWM` 形参是 `int8_t`）。

### 3.10 VOFA 波形打印（⚠️ 工程里没有，补法）

```c
        /* 新增：放在 PID_Calc 之后、Motor_SetPWM 之前 */
        if (++s_dbg_div >= 2) {
            s_dbg_div = 0;
            printf("%.1f,%.1f,%.1f,%.1f\n",
                   pid_left.target, g_act_left, pwm_L_temp, pwm_R_temp);
        }
```

【新增】每 2 拍打印一次目标/实际/两路 PWM。
【配套】需在任务开头加 `static uint8_t s_dbg_div = 0;`，并确认 `fputc` 已重定向（阶段一）。

> [!warning] 打印副作用
> 115200 下每行约 2.6 ms；每 20 ms 打一次约占 13% 周期，还会拉长周期。调参用，跑车关。
> `%f` 需标准库浮点打印（工程未勾 MicroLIB）；异常时改“乘 10 取整”定点打印。

> [!abstract] 人话翻译
> 没有波形就没有证据；至少打“目标、实际”两列，才能判断超调、稳态误差、响应时间。

### 3.11 全局实例、数据交换与局部变量（main.c 20–21、100–103、213–215、221、298–301 行）

```c
float pwm_L_temp;
float pwm_R_temp ;
...
PID_t pid_left, pid_right;
float g_target_left = 0, g_target_right = 0;
float g_act_left    = 0, g_act_right    = 0;
int16_t g_pwm_disp_L = 0, g_pwm_disp_R = 0;
...
void task_ctrl(void *p)
{
    int16_t encL, encR;
    float pitch, roll;
    float targetL, targetR;
...
    const float MIN_SPEED       = 20.0f;
```

【第 20–21 行】两路 PWM 中间量（`float`）。
【第 100 行】`PID_t pid_left, pid_right;`：两份实例。
【第 101 行】`g_target_*`：目标全局副本（显示用）。
【第 102 行】`g_act_*`：反馈全局副本。
【第 103 行】`int16_t g_pwm_disp_*`：显示用 PWM。
【第 213 行】`int16_t encL, encR;`。
【第 214–215 行】`float pitch, roll; float targetL, targetR;`。
【第 221 行】`MIN_SPEED`：声明后**再没使用**（死变量）。
【第 298–301 行】把目标与 PWM 写进全局，供 `task_disp`（308 行起）显示。

> [!abstract] 人话翻译
> 控制任务算完“抄一份”到全局，显示任务再读——两个任务唯一的通信方式（不加锁，靠 32 位读原子性）。真正的最小输出是补偿里写死的 30，不是 `MIN_SPEED`。

---

## 四、硬件与现象对照

| 操作 | 现象 | 依据 |
|---|---|---|
| 目标 50 | 实际逐渐接近 50 | 闭环 |
| 用手捏轮子 | 该轮 PWM 顶到 100，松手回落 | P+I |
| 目标 5 | 实际至少 30（起转补偿） | `:274–275` |
| 堵转后松手 | 可能猛冲一下 | 无抗饱和 |
| 打开串口 | 什么都没有 | 裸 `printf` 0 处 |

> [!warning] 待确认（防幻觉）
> 电机额定转速、堵转电流、电源能力决定“0~100 对应多少真转速”。代码里没有，**不编**。

---

## 五、本阶段缺口

| 项目 | 现状 | 要补 |
|:--:|:--:|---|
| VOFA 波形 | 无 | 3.10 打印 |
| 固定周期 | `vTaskDelay` 漂移 | 换 `vTaskDelayUntil` |
| 抗积分饱和 | 无 | 输出饱和时不累加 |
| D 项 | 未实现 | 速度环可不加，但要能解释 |
| 减速比 | 9.27666 与 1:13 不符 | 改 13，重调 Ki |

---

## 六、防幻觉审计报告

> [!success] 🛡️ 防幻觉审计报告
> 1. **代码出处核实**：是。`PID_OUT_MAX/PID_INT_MAX`、`PID_t`、`PID_Init`、`PID_Calc`、`2.0/1.5/0.0`、`vTaskDelay(20)`、`configTICK_RATE_HZ`、反馈换算、补偿、`Motor_SetPWM`、全局量与局部变量均有行号出处；打印代码标“新增”。
> 2. **库的纯洁性**：未混用，StdPeriph + FreeRTOS，无 `HAL_`。
> 3. **脑补排查**：否。未新增引脚；`Ki_cont=75`、`integral≈66.7`、`1.40 倍` 均为推导；电机参数标待确认；`13 线`标为反推值。
> 4. **本次悬空依赖**：`Encoder_Get`、`Motor_SetPWM`（阶段二）、`printf/fputc`（阶段一）。**代码级盲区：无**。

---

## 七、版本与勘误记录

| 版本 | 日期 | 变更 |
|:--:|---|---|
| V1 | 2026-10-23 | 首版 |
| V1.1 | 2026-10-23 | 修正“13 线归因”、补右轮补偿与符号前提、新增 3.11（全局量/类型/死变量） |
| **V2** | **2026-10-23** | **按宪法重建**：版本交接指令；3.8 补完整左右两套补偿代码；3.11 补显示回写；缺口表与审计报告同步 |

#RM电控 #STM32 #PID #闭环
