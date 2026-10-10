---
title: Cyber 微调框架
---

基于 PEFT/LoRA 的聊天记录微调框架（把「我 / 对方」的聊天导出微调成一个会说话的模型）。

## 项目结构

> 与实际目录一一对应，供核对（`models/*` 与 `outputs` 被 `.gitignore` 忽略，克隆后需要自备权重）。

```text
cyber/
├── .env                          # 环境变量（当前为空文件）
├── .gitmodules                   # 子模块：.llama-tools/llama.cpp-to-hf
├── data/                         # 数据目录
│   ├── dataset.py                # ConversationDataset，支持 raw / sharegpt / alpaca
│   ├── raw/chat.txt              # 原始聊天记录导出：每行「我: …」或「对方: …」
│   └── processed/
│       ├── train.txt             # 训练集
│       └── check.txt             # 验证集
├── models/
│   ├── base_model/               # 基础模型（本地或 HF 下载）
│   └── hf/qwen/                  # 转换得到的 HF 权重/分词文件示例
├── params/
│   └── training_config.yaml      # 训练配置
├── scripts/                      # 脚本目录
│   ├── prepare_data.py           # 原始文本 → ShareGPT
│   ├── llm_train.py              # 训练主脚本
│   ├── inference.py              # 交互式推理
│   ├── ollama_to_hf.py           # Ollama ↔ Hugging Face 格式互转
│   └── utils/
│       ├── __init__.py
│       ├── data_utils.py         # clean_text / convert_raw_to_sharegpt
│       └── model_utils.py        # load_tokenizer / load_base_model / load_lora_model
├── test.py                       # 自测脚本
├── requirements.txt              # 依赖库
└── OLLAMA_TO_GUFF.md             # Ollama 模型转换详细指南
```

## 快速开始

### 1. 安装依赖

```bash
# torch / transformers / datasets / accelerate / peft / trl /
# bitsandbytes / tensorboard / ...
pip install -r requirements.txt
```

### 2. 准备数据

原始文件每行一句话，所属角色由前缀决定：

```text
我: 今天吃什么
对方: 随便
```

转换成 ShareGPT 格式（`--input` 与 `--output` **都是必填参数**，少给会直接报 argparse 错误）：

```bash
python scripts/prepare_data.py \
  --input  data/raw/chat.txt \
  --output data/processed/train.txt \
  --self_name 我 --other_name 对方
```

产出文件是**每行一个 JSON 对象**（JSONL）。它虽然叫 `.txt`，但 `training_config.yaml`
里 `data_format: "sharegpt"`，加载时会按 JSON 逐行解析——**扩展名不代表纯文本**，别当纯文本打开另存。

同样的命令再生成一份 `data/processed/check.txt` 作为验证集（`data.val_path` 指向它）。

### 3. 准备模型

配置在 `params/training_config.yaml`：

| 字段 | 当前值 | 说明 |
|------|--------|------|
| `model.local_model_path` | `~/.ollama/models/…/deepseek-r1/8b-…` | 存在时优先 |
| `model.model_name` | `deepseek-ai/DeepSeek-R1-Distill-Llama-8B` | 缺失时从 HF 下载 |
| `model.load_in_4bit` | `false` | 显存紧张时改为 `true` |
| `model.use_deepseek` | `false` | 需要 DeepSeek 官方实现时开启 |

已有本地 Ollama 模型可以先转成 HF 格式：

```bash
python scripts/ollama_to_hf.py \
  --ollama-model deepseek-r1:8b \
  --output models/hf/deepseek \
  --hf-model deepseek-ai/DeepSeek-R1-Distill-Llama-8B

cat OLLAMA_TO_GUFF.md      # 完整参数说明
```

然后把 `model.local_model_path` 指向转换结果。

### 4. 开始训练

```bash
python scripts/llm_train.py --config params/training_config.yaml
# 多卡时由 launcher 传入 --local_rank <n>
```

关键训练参数（`params/training_config.yaml`）：

- **data**：`max_seq_length 1024`、`num_workers 4`、`data_format sharegpt`、
  `human_role human` / `assistant_role gpt`
- **training**：`epochs 10`、`batch_size 2`、`val_batch_size 4`、`lr 2e-4`、
  `weight_decay 0.01`、`grad_clip 1.0`、`fp16 true`、`bf16 false`、
  `early_stop_patience 3`、`save_interval 1`、`out_dir outputs`
- **lora**：`r 16`、`lora_alpha 32`、`lora_dropout 0.05`，`target_modules` 为
  `q/k/v/o/gate/up/down_proj` 共 7 个
- **scheduler**：`type linear`、`warmup_steps 100`
- **logging**：`log_dir outputs/logs`（TensorBoard）

### 5. 测试推理

```bash
python scripts/inference.py \
  --base_model /path/to/base_model \
  --lora_path outputs/best_model \
  --max_history 10 --max_new_tokens 512 \
  --temperature 0.7 --top_p 0.9 --top_k 50
# 显存不足时加 --load_in_4bit
```

## 配置建议

- 显存不够就先降 `batch_size`，再考虑 `load_in_4bit: true`（BitsAndBytes 量化）。
- 显卡支持 bfloat16 时可设 `bf16: true`；与 `fp16` 不必同时开。
- `models/*`、`outputs`、`logs` 都被 `.gitignore` 忽略，仓库里只有配置与脚本；新环境要重新下载基座或执行第 3 步。

## 工程特色

- **模块化设计**：`data/dataset.py` 管数据，`scripts/utils/` 管文本清洗与模型加载，`scripts/` 只留流程脚本。
- **多格式支持**：`raw` / `sharegpt` / `alpaca` 三种 `data_format`，换语料不用改训练代码。
- **集成 PEFT/LoRA**：7 个投影层做低秩微调，可量化加载，训完 `merge_and_unload` 合成完整模型。
- **早停与检查点**：`early_stop_patience: 3` + `save_interval: 1`，自动保留最佳权重。
- **可观测**：训练日志进 `outputs/logs`，支持 TensorBoard 与 tqdm 进度条。
- **Ollama 互通**：`ollama_to_hf.py` 支持把本地 Ollama 模型转成 HF 格式再微调。

## 详细文档

- [Ollama 模型转换指南](OLLAMA_TO_GUFF.md)
