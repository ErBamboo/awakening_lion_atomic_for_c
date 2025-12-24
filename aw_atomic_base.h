#ifndef AW_ATOMIC_BASE_H
#define AW_ATOMIC_BASE_H

#include "port/awlf_port_compiler.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// 1. C 标准与编译器检测
// ============================================================================

// 检测 C11 及以上版本的原子操作支持
// 如果定义了 AW_USE_STDATOMIC，后续将直接使用标准库接口
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_ATOMICS__)
    #define AW_USE_STDATOMIC
#endif

// 编译器类型检测
#if defined(_MSC_VER)
    #define AW_COMPILER_MSVC
#elif defined(__ARMCC_VERSION)
    // ARM Compiler 6 (基于 Clang)
    #if __ARMCC_VERSION >= 6000000
        #define AW_COMPILER_AC6
        #define AW_COMPILER_GCC_LIKE
        #define AW_COMPILER AW_COMPILER_AC6
    #else
        // ARM Compiler 5
        #define AW_COMPILER_AC5
        #define AW_COMPILER AW_COMPILER_AC5
    #endif
#elif defined(__CC_ARM)
    // ARM Compiler 5 (标准宏)
    #define AW_COMPILER_AC5
    #define AW_COMPILER AW_COMPILER_AC5
#elif defined(__clang__) || defined(__GNUC__)
    #define AW_COMPILER_GCC_LIKE
    #define AW_COMPILER AW_COMPILER_GCC_LIKE
#else
    #error "port/compiler: [Err]: Unsupported compiler"
#endif



/**
 * AC6 (基于 Clang) 完美兼容 GCC 原子 Builtin
 * 因此代码是和gcc一样的
 * MSVC则引入特殊依赖
 */
#if defined(AW_COMPILER_MSVC)
    #include <windows.h>
    #include <intrin.h>
#elif defined(AW_COMPILER_GCC_LIKE) || defined(AW_COMPILER_AC6) || defined(AW_COMPILER_AC5)
    // AW_NOTE("It's a test")
#else
    // 如果既不是 C11 也不支持已知编译器，则无法编译
    #ifndef AW_USE_STDATOMIC
        #error "aw_atomics: Unsupported compiler and C standard version < C11"
    #endif
#endif

// ============================================================================
// 2. 内存模型定义 (Memory Order)
// ============================================================================

#ifdef AW_USE_STDATOMIC
    // --- C11 标准库模式 ---
    #include <stdatomic.h>

    // 直接映射标准类型，确保类型兼容性
    typedef memory_order aw_memory_order;

    // 映射宏常量
    #define AW_MO_RELAXED memory_order_relaxed
    #define AW_MO_CONSUME memory_order_consume
    #define AW_MO_ACQUIRE memory_order_acquire
    #define AW_MO_RELEASE memory_order_release
    #define AW_MO_ACQ_REL memory_order_acq_rel
    #define AW_MO_SEQ_CST memory_order_seq_cst

#else
    // --- 传统/回退模式 ---
    // 定义与 C11 类似的枚举，供特定编译器的实现使用
    typedef enum {
        AW_MO_RELAXED = 0,
        AW_MO_CONSUME = 1,
        AW_MO_ACQUIRE = 2,
        AW_MO_RELEASE = 3,
        AW_MO_ACQ_REL = 4,
        AW_MO_SEQ_CST = 5
    } aw_memory_order;

#endif

// ============================================================================
// 3. 原子类型定义 (统一声明方式)
// ============================================================================

/*
 * 用于声明原子变量的宏。
 * * 示例:
 * aw_atomic_int_t counter = AW_ATOMIC_VAR_INIT(0);
 * aw_atomic_long_t flag;
 * * 原理:
 * - C11: 使用 _Atomic 关键字修饰 (例如 _Atomic int)。
 * - MSVC/Legacy: 使用 volatile 关键字修饰 (例如 volatile int)，
 * 因为这些编译器的原子操作通常接受 volatile 指针。
 */

#ifdef AW_USE_STDATOMIC
    // C11 标准模式
    #define aw_atomic_t(type)       _Atomic(type)
    #define AW_ATOMIC_VAR_INIT(val) (val)       // ATOMIC_VAR_INIT(val)在C11中是标准，但在C17后就被放弃了，因此采用直接返回值
#else
    // MSVC / GCC Legacy / AC5 模式
    // 在这些编译器中，原子操作函数通常期望传入 volatile 指针
    #define aw_atomic_t(type)       volatile type
    #define AW_ATOMIC_VAR_INIT(val) (val)
#endif

// 常用原子类型别名
typedef aw_atomic_t(int)                aw_atomic_int_t;
typedef aw_atomic_t(unsigned int)       aw_atomic_uint_t;
typedef aw_atomic_t(long)               aw_atomic_long_t;
typedef aw_atomic_t(unsigned long)      aw_atomic_ulong_t;
typedef aw_atomic_t(long long)          aw_atomic_llong_t;
typedef aw_atomic_t(unsigned long long) aw_atomic_ullong_t;
typedef aw_atomic_t(void*)              aw_atomic_ptr_t;
typedef aw_atomic_t(size_t)             aw_atomic_size_t;


// ============================================================================
// 4. 辅助宏
// ============================================================================
#define AW_INLINE static inline

// CPU Pause (用于自旋锁优化)
// 注意: <stdatomic.h> 不包含 pause/yield 指令，仍需依赖编译器内置函数
AW_INLINE void aw_cpu_pause(void) {
#if defined(AW_COMPILER_MSVC)
    _mm_pause();
#elif defined(AW_COMPILER_GCC_LIKE)
    #if defined(__i386__) || defined(__x86_64__)
        __asm__ __volatile__("pause");
    #elif defined(__aarch64__) || defined(__arm__)
        __asm__ __volatile__("yield");
    #endif
#elif defined(AW_COMPILER_AC5)
    __yield();
#else
    // 无法识别编译器时，如果是 C11 模式则留空，不影响逻辑正确性，只影响自旋效率
    (void)0; 
#endif
}

#endif // AW_ATOMIC_BASE_H