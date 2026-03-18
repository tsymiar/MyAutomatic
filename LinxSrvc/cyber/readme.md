# Cyber 项目说明

## 项目结构

```
cyber/
├── .env                      # 环境变量
├── data/                     # 数据目录
│   ├── raw/                  # 原始聊天记录导出
│   ├── processed/            # 清洗后的数据文件（train.txt, check.txt）
│   └── dataset.py            # 数据集类
├── params/                   # 参数配置
│   └── training_config.yaml  # 训练配置文件
├── models/                   # 模型目录
│   └── base_model/           # 基础模型（本地或HF下载）
├── scripts/                  # 脚本目录
│   ├── prepare_data.py       # 数据清洗脚本
│   ├── llm_train.py          # 训练主脚本
│   ├── inference.py          # 推理脚本
│   └── ollama_to_hf.py       # Ollama 模型转换为 Hugging Face 格式
├── utils/                    # 工具包
│   ├── __init__.py           # 工具包初始化
│   ├── data_utils.py         # 数据处理工具函数
│   └── model_utils.py        # 模型加载/更新工具函数
├── outputs/                  # 输出目录（训练模型检查点、日志等）
├── requirements.txt          # 依赖库
└── OLLAMA_TO_GUFF.md         # Ollama 模型转换详细指南
```

## 快速开始

### 1. 安装依赖

```bash
pip install -r requirements.txt
```

### 2. 准备数据

将聊天记录导出为纯文本，每行格式如 `我: 你好` 或 `对方: 在吗`。

运行数据清洗脚本：

```bash
python scripts/prepare_data.py
```

### 3. 准备模型

#### 选项 A: 从 Hugging Face 下载

修改 `params/training_config.yaml` 中的 `model_name`。

#### 选项 B: 从 Ollama 模型转换

如果您已有本地 Ollama 模型，可以将其转换为 Hugging Face 格式：

```bash
# 基础用法
python scripts/ollama_to_hf.py \
  --ollama-model qwen3.5:4b \
  --output models/hf/qwen \
  --hf-model Qwen/Qwen-7B

# 查看完整使用指南
cat OLLAMA_CONVERSION_GUIDE.md
```

然后在 `params/training_config.yaml` 中将 `local_model_path` 设置为转换后的路径。

### 4. 开始训练

```bash
python scripts/llm_train.py --config params/training_config.yaml
```

### 5. 测试推理

```bash
python scripts/inference.py --base_model /path/to/model --lora_path ./outputs/best_model
```

## 配置建议

- 根据实际显存调整 `batch_size`
- 如果显卡支持 bfloat16，可开启 `bf16: true`
- 对于大模型，建议开启 `load_in_4bit: true` 以节省显存

## 工程特色

- **模块化设计**：数据集、工具函数、模型加载分离，易于维护
- **多格式支持**：支持 raw、sharegpt、alpaca 格式，适应不同来源
- **集成 PEFT/LoRA**：高效微调，支持 4bit 量化
- **混合精度与分布式**：兼容单卡/多卡训练，支持 fp16/bf16
- **早停与检查点**：自动保存最佳模型，防止过拟合
- **日志与 TensorBoard**：便于监控训练过程
- **Ollama 模型集成**：支持将本地 Ollama 模型转换为 Hugging Face 格式

## 详细文档

- [Ollama 模型转换指南](OLLAMA_CONVERSION_GUIDE.md)
