# 🚗 Smart Parking Lot Management System (智能停车场收费调度系统)

基于 C++17 与 Qt6 开发的单出入口智能停车场调度系统。

## ✨ 核心特性 (Features)
* 🚀 **极速检索**: 引入 `std::unordered_map` 哈希索引，实现全局车辆状态、计费的 $O(1)$ 毫秒级查询。
* 🅿️ **科学调度**: 采用 `std::stack` (场内) 与 `std::queue` (便道排队) 结合，完美契合单行道停车场“后进先出”及便道“先进先出”的物理特性。
* 💰 **商业级计费**: 搭载向上取整的动态阶梯计费算法（区分纯电、不同排量及大型车翻倍费率）。
* 🎨 **现代 UI**: 包含多媒体开场动画 (`QMediaPlayer`) 与直观的车辆状态动态更新。
* 💾 **容灾持久化**: 支持基于 CSV 格式的本地断电数据无损恢复。

## 🛠️ 技术栈 (Tech Stack)
* **语言**: C++ 17
* **框架**: Qt 6.10.2
* **核心数据结构**: 顺序栈、链式队列、散列表(Hash)

## 📸 运行截图 (Screenshots)
<img width="1588" height="888" alt="a6651db5-f0bb-4cc9-a3c4-c55463207a46" src="https://github.com/user-attachments/assets/15b5f499-4f1f-4f27-89bb-6e0983458b02" />

><img width="1286" height="693" alt="87892af2-df28-4a99-b15b-f4d75e48ac06" src="https://github.com/user-attachments/assets/83bb56f2-2eaf-47ea-9f0e-ff9ac6624a32" />

<img width="1288" height="758" alt="c2f70ef2-48f7-48ee-9716-9fc997074c4d" src="https://github.com/user-attachments/assets/214eee75-0f65-40fe-bd65-d6dc1ffba6d9" />

