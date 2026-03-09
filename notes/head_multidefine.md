你提出的问题非常关键，涉及到 **C++ 中全局变量的定义与链接规则（One Definition Rule, ODR）**。

---

## ❌ 问题核心

你在 **头文件 `Hashtables.h` 中直接定义了全局变量 `g_data`**：

```cpp
// Hashtables.h
static struct {
    HMap db;
    std::vector<Conn *> fd2conn;
    DList idle_list;
    std::vector<HeapItem> heap;
} g_data;   // ← 这是 **定义（definition）**，不是声明！
```

并且 **多个 `.cpp` 文件都 `#include "Hashtables.h"`**。

### 🚨 后果：**违反 One Definition Rule (ODR)**

- 每个包含该头文件的 `.cpp` 文件在编译时，都会 **生成自己独立的 `g_data` 实例**。
- 链接阶段，这些同名但不同地址的 `g_data` **不会合并**（因为用了 `static`！），而是各自保留。
- 结果：
  - 在 `RedisServer.cpp` 中对 `g_data.heap.push_back(...)` 操作的是 **它自己的副本**
  - 在 `ZSetCommand.cpp` 中读 `g_data.heap.size()` 读的是 **另一个空的副本**
  - 所以你会看到：**明明插入了数据，却查不到；size 明明是 1，却打印 empty**

> 💥 这就是你遇到 `"g_data.heap.size() 明明是 1，却打印 heap empty"` 的根本原因！

---

## 🔍 为什么 `static` 让问题更隐蔽？

- `static` 在全局作用域表示 **内部链接（internal linkage）**，即该符号只在当前编译单元（.o 文件）可见。
- 所以链接器 **不会报“多重定义”错误**（如果是非 static 全局变量，会直接 link error）。
- 但每个 `.cpp` 都有一个 **私有的、独立的 `g_data`**，彼此完全隔离。

这比 link error 更危险——程序能运行，但逻辑完全错乱。

---

## ✅ 正确做法：**在头文件中声明，在一个 .cpp 中定义**

### 第一步：修改 `Hashtables.h` —— 只声明，不定义

```cpp
// Hashtables.h
#ifndef HASHTABLES_H
#define HASHTABLES_H

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include "RedisServer.h"
#include "util.h"
#include "Heap.h"

// ... 其他结构体定义（HNode, HTab, HMap, Entry）...

// 声明全局变量 g_data（extern 表示“在别处定义”）
extern struct GlobalData {
    HMap db;
    std::vector<Conn *> fd2conn;
    DList idle_list;
    std::vector<HeapItem> heap;
} g_data;

#endif // HASHTABLES_H
```

> ⚠️ 注意：去掉了 `static`，加上了 `extern`。

---

### 第二步：在一个 `.cpp` 文件中定义 `g_data`

例如，在 `GlobalData.cpp`（或 `RedisServer.cpp`）中：

```cpp
// GlobalData.cpp
#include "Hashtables.h"

// 定义 g_data（无 extern，无 static）
struct GlobalData g_data;
```

或者更简洁（如果不想命名类型）：

```cpp
// GlobalData.cpp
#include "Hashtables.h"

// 直接定义匿名 struct 的实例
decltype(g_data) g_data; // C++11 起支持
```

或者最简单（推荐）：

```cpp
// GlobalData.cpp
#include "Hashtables.h"

// 重新写一遍类型定义（确保与头文件一致）
struct {
    HMap db;
    std::vector<Conn *> fd2conn;
    DList idle_list;
    std::vector<HeapItem> heap;
} g_data;
```

> ✅ 这样，**整个程序只有一个 `g_data`**，所有 `.cpp` 文件通过 `extern` 引用它。

---

## 🛠️ 额外建议：给 `g_data` 类型命名

为了避免重复写 struct，建议：

```cpp
// Hashtables.h
struct GlobalData {
    HMap db;
    std::vector<Conn *> fd2conn;
    DList idle_list;
    std::vector<HeapItem> heap;
};

extern GlobalData g_data;
```

```cpp
// GlobalData.cpp
#include "Hashtables.h"
GlobalData g_data; // 定义
```

这样更清晰，也便于调试（GDB 能识别类型名）。

---

## ✅ 总结

| 你的写法 | 问题 | 正确写法 |
|--------|------|--------|
| 在头文件中 `static struct { ... } g_data;` | 每个 .cpp 有独立副本，数据不共享 | 头文件用 `extern GlobalData g_data;`，一个 .cpp 中定义 `GlobalData g_data;` |

### 🔧 修复后效果：
- 所有文件操作的是 **同一个 `g_data`**
- `heap.push_back()` 后，`heap.size()` 在任何地方都能正确反映
- 不再出现“插入了却查不到”的诡异现象

---

> 💡 **黄金法则**：  
> **永远不要在头文件中定义非 `inline`/`constexpr` 的全局变量（即使是 `static`）！**  
> 用 `extern` 声明，`.cpp` 中定义。

如果你现在就按这个方式改，你的 `heap empty` 问题应该立刻消失。