# Ollama 模型转换工具

将本地 Ollama 模型转换为 Hugging Face 格式，支持完整权重转换和简化配置转换。

## 安装依赖

```bash
pip install -r requirements.txt
pip install llama-cpp-python transformers safetensors
```

## 快速开始

### 方式一：通过 Ollama 模型名转换

```bash
python scripts/ollama_to_hf.py \
  --ollama-model qwen3.5:4b \
  --output models/hf/qwen \
  --hf-model Qwen/Qwen-7B
```

### 方式二：直接转换模型文件

```bash
python scripts/ollama_to_hf.py \
  --model-file ./models/qwen.gguf \
  --output models/hf/qwen \
  --hf-model Qwen/Qwen-7B
```

支持以下文件格式：
- `.gguf` - GGUF 格式模型
- `.safetensors` - SafeTensors 格式模型
- `.bin` - PyTorch Bin 格式模型
- `.pt` - PyTorch PT 格式模型
- **sha256 命名的文件** - 无扩展名的 blob 文件（自动检测格式）

**注意**：对于 sha256 命名的文件（如 Ollama 的 blob 文件），脚本会自动检测文件格式。

### 转换 sha256 命名的文件

```bash
python scripts/ollama_to_hf.py \
  --model-file ~/.ollama/models/blobs/sha256-aeaeda25e63... \
  --output models/hf/qwen \
  --hf-model Qwen/Qwen-7B
```

### 简化转换（仅 tokenizer 和配置）

```bash
python scripts/ollama_to_hf.py \
  --ollama-model gemma3 \
  --output models/hf/gemma3 \
  --hf-model google/gemma-7b \
  --simple-conversion
```

## 参数说明

| 参数 | 说明 |
|------|------|
| `--ollama-model` | Ollama 模型名称（支持带 tag，如 `qwen3.5:4b`） |
| `--model-file` | 直接指定模型文件路径（与 `--ollama-model` 互斥） |
| `--output` | 输出目录路径（必需） |
| `--hf-model` | 对应的 Hugging Face 模型名称（用于加载 tokenizer） |
| `--tokenizer` | 指定 tokenizer 名称（覆盖 `--hf-model`） |
| `--simple-conversion` | 仅保存 tokenizer 和配置，不转换权重 |
| `--full-conversion` | 使用完整转换（GGUF 模型默认启用） |
| `--force-install` | 强制重新安装转换工具 |
| `-v` | 显示详细日志 |

## 转换流程

### 通过 Ollama 模型名转换

1. 自动从 `~/.ollama/models/` 查找模型文件（通过 manifest 定位 blob）
2. 自动检测 blob 存储格式：
   - 新格式：`blobs/sha256/{digest[:2]}/{digest}`
   - 旧格式：`blobs/sha256-{digest}`
3. 支持 sha256 命名的无扩展名文件（自动检测格式）
4. 首次运行自动安装 `llama.cpp` 转换工具到 `~/.llama-cpp-tools/`
5. 从 Hugging Face 加载 tokenizer
6. 执行模型转换并保存到输出目录

### 直接转换模型文件

1. 自动检测模型文件格式（通过扩展名或文件头）
   - 有扩展名：根据扩展名识别（.gguf, .safetensors, .bin, .pt）
   - 无扩展名（sha256 命名）：读取文件头自动识别格式
2. 从 Hugging Face 加载 tokenizer
3. 执行模型转换并保存到输出目录

## 注意事项

- 转换大模型需要较长时间，请耐心等待
- 确保有足够的磁盘空间存储转换后的模型
- 遵守模型的许可证要求

## 相关链接

- [llama.cpp](https://github.com/ggerganov/llama.cpp)
- [Ollama](https://ollama.ai)
- [Hugging Face Transformers](https://github.com/huggingface/transformers)
