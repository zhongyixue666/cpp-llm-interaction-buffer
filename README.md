# cpp-llm-interaction-buffer
🏆 项目简介
一个面向大模型 Agent 与强化学习（RL）场景的高性能 C++ 交互缓冲区。实现线程安全的多用户交互数据存储与批量采样，贴合工业级内存优化与并发设计。是大模型推理引擎核心模块（如 TensorBuffer / ReplayBuffer）的极简实战项目。
🏆 Project Intro
A high-performance C++ interaction buffer for LLM Agents and Reinforcement Learning scenarios.It implements thread-safe storage and batch sampling of multi-user interaction data, with industrial-grade memory optimization and concurrency design.A minimal practice for core modules of large model inference engines.

✨ 项目亮点 (Highlights)
1. 贴合大模型推理场景 (Real-world LLM Scenarios)
场景模拟：用 user_embedding[4] 模拟大模型的输入张量（Tensor），承载用户输入特征。
逻辑对齐：核心逻辑与大模型推理引擎的张量缓冲区 / 显存复用完全一致。
接口适配：批量采样接口 (sample_batch) 直接对接大模型批量推理（Batch Inference）流程。
2. 工业级内存优化 (Industrial Memory Optimization)
预分配内存：使用 vector::reserve() 提前分配堆内存，避免 push_back 时频繁扩容，减少显存申请与拷贝开销。
智能指针：std::unique_ptr 自动管理堆内存生命周期，杜绝内存泄漏（Memory Leak）。
环形缓冲区：容量满时覆盖最旧数据，实现显存无限循环利用，适配大模型显存有限的约束。
3. 线程安全设计 (Thread-safe Concurrency)
互斥锁保护：std::mutex 保证多用户并发读写时的数据一致性。
RAII 锁管理：std::lock_guard 利用 RAII 机制自动解锁，避免死锁，符合高并发服务标准。
4. 高性能与可观测 (High Performance & Observability)
高精度计时：内置 std::chrono 性能统计，实测处理 100 个用户交互仅需 75 微秒。
语言优势：充分体现 C++ 在高性能计算场景下，相比 Python 的速度优势。

📁 代码结构 (Code Structure)
LLMInteraction(交互数据结构体，模拟 Tensor)  
LLMInteractionBuffer(核心缓冲区类)
add_interaction(线程安全添加数据)
sample_batch(批量采样，供 Agent 推理)
generate_random_interaction(模拟用户输入)
main()(程序入口：模拟多用户并发 + 性能统计)

🚀 运行效果 (Screenshot & Demo)
1. 运行截图 (Running Screenshot)
<img width="1322" height="446" alt="Image" src="https://github.com/user-attachments/assets/ff088b9d-04da-427e-99d2-6508b49bd641" />

2. 控制台输出 (Console Output)
text
大模型交互缓冲区初始化：预分配1000条交互数据内存
处理100个用户交互耗时：75微秒
批量采样大小：32
第一条采样数据：Agent动作=2

💡 心得体会 (Personal Reflection)
手动实现 LLMInteractionBuffer，实践了 C++ 的 unique_ptr、mutex 和 vector 高级用法，更深刻理解了大模型推理引擎底层的核心逻辑。
资源管理：如何自动管理内存，避免泄漏；
并发安全：如何在高并发下保证数据不乱；
性能极致：如何通过 reserve 和环形结构压榨硬件性能。
