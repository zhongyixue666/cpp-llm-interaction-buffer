//大模型 Agent 推理优化

#include <iostream>
#include <vector> //存储多用户张量数据
#include <memory> //智能地管理内存的指针,管理大模型张量堆内存
#include <mutex> //线程安全,多用户并发请求
#include <random>
#include <chrono> //性能计时,推理耗时优化

//定义Agent交互数据结构体,模拟tensor
struct LLMInteraction {
	float user_embedding[4]; //embedding，嵌入，把文字 / 数字等离散信息，转换成大模型能理解的连续向量（张量），4 维是为了代码简化，实际大模型更高维
	int agent_action; //比如“生成回复”“调用工具”
	bool is_finished; //交互是否结束

	//构造函数,初始化交互数据.结构体的构造函数,名字必须和结构体名完全一致,不同函数不能重名
	LLMInteraction(float emb[4], int action, bool finished):agent_action(action), is_finished(finished) {
		// 拷贝张量数据（堆内存操作基础，对应大模型张量拷贝）
		for (int i = 0; i < 4; ++i) {
			user_embedding[i] = emb[i];
		}
	}
};

//大模型Agent的交互数据缓冲区,线程安全 + 内存优化
class LLMInteractionBuffer {
private:
	//堆，存储交互数据，智能指针自动释放
	std::unique_ptr<std::vector<LLMInteraction>> buffer_;
	//缓冲区最大容量,模拟大模型显存上限
	size_t max_capacity_; //无符号整数类型，大小随系统自动适配，能完整表示当前系统的内存最大长度，而unsigned int固定4字节，系统中无法表示超过 4GB 的内存大小
	//当前存储的交互数
	size_t current_size_;
	//互斥锁,多用户并发读写时保证数据安全
	std::mutex buffer_mutex_;
	//生成随机数
	std::mt19937 random_gen_; //std::mt19937是 C++ 标准库的高性能随机数生成器,比rand()更稳定，适合高性能计算场景

public:
	// 构造函数：初始化缓冲区,预分配内存，避免大模型推理时频繁扩容
	LLMInteractionBuffer(size_t capacity) :max_capacity_(capacity), current_size_(0), random_gen_(std::random_device{}()) {
		//！！！内存优化--堆上创建vector，预分配容量，减少显存申请/拷贝开销
		buffer_ = std::make_unique<std::vector<LLMInteraction>>();//无参构造
		buffer_->reserve(capacity);//给 vector 提前分配能存capacity个LLMInteraction的内存，避免后续push_back时频繁扩容，只占空间，不创建元素
		std::cout << "大模型交互缓冲区初始化：预分配" << capacity << "条交互数据内存" << std::endl;
	}

	// 1. 添加单条用户交互数据
	void add_interaction(const LLMInteraction& interaction) {
		//!!!加锁
		std::lock_guard<std::mutex> lock(buffer_mutex_);
		
		if (current_size_ < max_capacity_) {
			buffer_->push_back(interaction);
			current_size_++;
		}
		else {
			//!!! 显存满了：环形覆盖最旧数据
			buffer_->at(current_size_ % max_capacity_) = interaction;
		}
	}

	//2. 批量采样交互数据
	std::vector<LLMInteraction> sample_batch(size_t batch_size) {
		//线程安全核心,创建互斥锁对象lock，构造时自动锁定buffer_mutex_,lock_guard会在函数结束时自动解锁，不用手动写unlock，更安全
		std::lock_guard<std::mutex> lock(buffer_mutex_);
		std::vector<LLMInteraction> batch;//空的vector，用来存放采样后的批量数据,batch是局部变量，存在栈上，但里面的元素（LLMInteraction）存在堆上

		//性能优化，给batch提前分配能存batch_size个元素的堆内存
		batch.reserve(batch_size);
		
		if (current_size_ < batch_size) {
			std::cout << "当前缓冲区的交互数据不足，无法采样批量" << std::endl;
			return batch;//返回空的batch,函数执行结束
		}

		//随机采样,创建均匀分布的随机数生成器
		std::uniform_int_distribution<size_t> dist(0, current_size_ - 1);
		//循环采样
		for (size_t i = 0; i < batch_size;++i) {
			size_t idx = dist(random_gen_);//生成随机索引
			batch.push_back(buffer_->at(idx));
		}

		return batch;//返回采样好的批量数据
	}
};

//模拟用户输入文本后，大模型将文本转换成embedding张量的过程
LLMInteraction generate_random_interaction() {
	//定义4维float数组，模拟用户输入的embedding张量，实际大模型是1024/2048维
	float emb[4] = {
		(float)rand() / RAND_MAX,
		(float)rand() / RAND_MAX,
		(float)rand() / RAND_MAX,
		(float)rand() / RAND_MAX
	};

	// 随机生成Agent动作：rand()%3 → 取0/1/2,分别对应生成回复/调用工具/结束对话
	int action = rand() % 3;
	// 10%概率标记交互结束：rand()%100生成0-99的数，<10的概率是10%
	// 模拟用户结束对话（比如退出聊天界面）的场景
	bool finished = (rand() % 100) < 10;

	return LLMInteraction(emb, action, finished);
}

int main() {
	//初始化缓冲区，容量1000,模拟大模型显存上限
	LLMInteractionBuffer llm_buffer(1000);
	//模拟100个用户并发添加数据
	const int USER_COUNT = 100;
	//记录开始时间，纳秒级
	auto start = std::chrono::high_resolution_clock::now();

	for (int i = 0;i < USER_COUNT;++i) {
		LLMInteraction interaction = generate_random_interaction();
		llm_buffer.add_interaction(interaction);
	}

	auto end = std::chrono::high_resolution_clock::now();
	//计算耗时,duration_cast是类型转换，把高精度时间差转成微秒级，方便阅读
	auto cost = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	std::cout << "处理" << USER_COUNT << "个用户交互耗时：" << cost.count() << "微秒" << std::endl;

	//批量采样32条数据
	std::vector<LLMInteraction> batch = llm_buffer.sample_batch(32);
	std::cout << "批量采样大小：" << batch.size() << std::endl;

	// 边界检查：如果batch不为空（采样成功）
	// empty()是vector的成员函数，判断是否有元素,为空返回true
	if (!batch.empty()) {
		std::cout << "第一条采样数据：Agent动作=" << batch[0].agent_action << std::endl;
	}

	return 0;
}