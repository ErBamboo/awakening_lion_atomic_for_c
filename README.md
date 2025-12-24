# AW Atomics: 一个轻量级、Header-Only、跨平台 C 语言原子操作库

## 1. 架构与设计思想

`AW Atomics` 旨在为 C 语言开发者提供一套统一、高性能且易于使用的原子操作接口，消除不同编译器和标准版本之间的差异。

### 1.1 分层架构

该库采用清晰的分层设计，以确保极高的可维护性和扩展性：

- **基础适配层 (`aw_atomic_base.h`)**：负责编译器检测（MSVC, GCC, Clang, AC5/AC6）和标准环境探测（C11 `stdatomic.h`），定义了统一的内存序枚举 `aw_memory_order` 和原子变量修饰宏 `aw_atomic_t`。
- **后端实现层 (`aw_atomic_gcc.h`, `aw_atomic_msvc.h`, `aw_atomic_ac5.h`)**：针对不同编译器调用对应的内置函数或内联汇编。
- **核心接口层 (`aw_atomic.h`)**：提供统一的函数式宏 API，如 `aw_load` 和 `aw_cas`。它利用 `_Generic`或复杂的宏逻辑实现泛型支持，自动识别 8/16/32/64 位及指针类型。
- **简化应用层 (`aw_atomic_simple.h`)**：针对最常用的内存模型（Acquire/Release）封装了更短的 API（如 `aw_load_acq`），降低使用门槛。

### 1.2 核心设计理念

- **Header-only (纯头文件)**：无需编译，只需将头文件加入项目路径即可使用。
- **C11 优先与回退机制**：在支持 C11 以上的环境下优先调用标准 `stdatomic.h`，在旧编译器或嵌入式环境（如 ARMCC AC5）下自动回退到编译器特有的原子实现。
- **类型安全**：通过 `_Generic` 选择匹配宽度的后端函数，确保在 MSVC 等非泛型环境中依然保持类型转换的正确性。
- **高性能自旋优化**：内置 `aw_cpu_pause()`，针对不同架构（x86 的 `pause`，ARM 的 `yield`）提供自旋锁优化指令。

------

## 2. API 使用文档

### 2.1 变量声明与初始化

使用 `aw_atomic_t(type)` 定义原子变量，并使用 `AW_ATOMIC_VAR_INIT` 进行初始化。

```c
#include "aw_atomic_simple.h"

// 声明原子整型和指针
aw_atomic_int_t counter = AW_ATOMIC_VAR_INIT(0);
aw_atomic_ptr_t data_ptr = AW_ATOMIC_VAR_INIT(NULL);
```

### 2.2 基础原子操作 (`aw_atomic.h`)

这些 API 需要显式指定 `aw_memory_order`。

| **函数**                                | **功能描述**           |
| --------------------------------------- | ---------------------- |
| `aw_load(ptr, order)`                   | 原子读取               |
| `aw_store(ptr, val, order)`             | 原子写入               |
| `aw_exchange(ptr, val, order)`          | 原子交换（返回旧值）   |
| `aw_cas(ptr, exp_ptr, des, succ, fail)` | 强比较并交换 (CAS)     |
| `aw_fetch_add/sub(ptr, val, order)`     | 加法/减法原子操作      |
| `aw_fetch_and/or/xor(ptr, val, order)`  | 按位与/或/异或原子操作 |
| `aw_thread_fence(order)`                | 线程级内存屏障         |
| `aw_signal_fence(order)`                | 编译器级内存屏障       |

### 2.3 简化版 API (`aw_atomic_simple.h`)

该层封装了最常用的同步模型（Release/Acquire），推荐在大多数场景下使用。

#### 读取与写入 (Load/Store)

- `aw_load_acq(ptr)`: 获取语义读取，确保后续操作不重排至此之前。
- `aw_store_rel(ptr, val)`: 释放语义写入，确保此前操作已对其他线程可见。
- `aw_load_rlx(ptr)` / `aw_store_rlx(ptr, val)`: 松散模型，仅保证原子性。

#### 比较与交换 (CAS)

- `aw_cas_ar(ptr, exp_ptr, des)`: 强同步 CAS（Acquire-Release），适用于大多数无锁算法。
  - 成功时：`AW_MO_ACQ_REL`。
  - 失败时：`AW_MO_ACQUIRE`。

#### 算术与逻辑

- `aw_inc_ar(ptr)` / `aw_dec_ar(ptr)`: 原子加/减 1，全屏障保证。
- `aw_faa_rlx(ptr, val)`: 原子加并返回旧值，无顺序保证。
- `aw_for_ar(ptr, val)`: 原子按位“或”操作，常用于设置状态位。

### 2.4 内存屏障 (Fences)

- `aw_fence_ar()`: 线程级 Acquire-Release 屏障。
- `aw_fence_seq()`: 最强级别的全内存屏障 (Sequential Consistency)。
- `aw_compiler_barrier()`: 仅防止编译器重排的信号屏障。

------

## 3. 支持的编译器与架构

- **GCC / Clang**: 完美支持，利用 `__atomic` 内置函数。
- **MSVC**: 支持 x86/x64，利用 `_Interlocked` 系列指令。
- **ARMCC (AC5 / AC6)**: 完美支持嵌入式开发环境。
- **C11以上**: 自动检测并支持标准 `stdatomic.h`。
