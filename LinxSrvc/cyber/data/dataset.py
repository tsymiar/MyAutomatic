# data/dataset.py
import json
import os
# import torch
from torch.utils.data import Dataset

class ConversationDataset(Dataset):
    """对话数据集

    支持以下格式:
    - 'raw': 原始文本格式，每行一个对话
    - 'sharegpt': ShareGPT 格式，每行一个 JSON 对象
    - 'alpaca': Alpaca 格式，包含 instruction, input, output 字段
    """
    def __init__(self, file_path, tokenizer, max_length=1024, data_format='raw', sharegpt_config=None):
        """加载聊天数据集，支持 raw/sharegpt/alpaca 三种格式"""
        # 参数验证
        if not os.path.exists(file_path):
            raise FileNotFoundError(f"数据文件不存在: {file_path}")

        if data_format not in ['raw', 'sharegpt', 'alpaca']:
            raise ValueError(f"不支持的 data_format: {data_format}，必须是 'raw', 'sharegpt' 或 'alpaca'")

        self.tokenizer = tokenizer
        self.max_length = max_length
        self.data_format = data_format
        self.sharegpt_config = sharegpt_config or {}

        # 加载数据
        self.examples = self._load_data(file_path)

        # 设置 pad token
        if self.tokenizer.pad_token is None:
            self.tokenizer.pad_token = self.tokenizer.eos_token

    def _load_data(self, file_path):
        """加载数据文件"""
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                if self.data_format == 'sharegpt':
                    # 每行是一个JSON对象
                    examples = []
                    for line_num, line in enumerate(f, 1):
                        if line.strip():
                            try:
                                examples.append(json.loads(line))
                            except json.JSONDecodeError as e:
                                raise ValueError(f"第 {line_num} 行 JSON 解析失败: {e}")
                    return examples
                elif self.data_format == 'alpaca':
                    # 每行是一个JSON对象
                    examples = []
                    for line_num, line in enumerate(f, 1):
                        if line.strip():
                            try:
                                examples.append(json.loads(line))
                            except json.JSONDecodeError as e:
                                raise ValueError(f"第 {line_num} 行 JSON 解析失败: {e}")
                    return examples
                else:
                    # raw 格式：每行是一个文本
                    return [line.strip() for line in f if line.strip()]
        except UnicodeDecodeError as e:
            raise ValueError(f"文件编码错误: {e}")
        except IOError as e:
            raise IOError(f"读取文件失败: {e}")

    def __len__(self):
        return len(self.examples)

    def __getitem__(self, idx):
        """获取数据样本"""
        try:
            if self.data_format == 'sharegpt':
                return self._process_sharegpt(idx)
            elif self.data_format == 'alpaca':
                return self._process_alpaca(idx)
            else:
                return self._process_raw(idx)
        except Exception as e:
            raise ValueError(f"处理第 {idx} 个样本时出错: {e}")

    def _process_raw(self, idx):
        """处理 raw 格式数据"""
        text = self.examples[idx]

        enc = self.tokenizer(
            text,
            truncation=True,
            padding='max_length',
            max_length=self.max_length,
            return_tensors='pt'
        )

        input_ids = enc['input_ids'].squeeze()
        attention_mask = enc['attention_mask'].squeeze()

        # 对于 raw 格式，简单地将整个序列作为 labels
        # 注意：这种方式会训练模型预测整个序列，包括 user 部分
        # 如果有明确的 user/assistant 分隔，建议使用 sharegpt 格式
        labels = input_ids.clone()

        return {
            'input_ids': input_ids,
            'attention_mask': attention_mask,
            'labels': labels
        }

    def _process_sharegpt(self, idx):
        """处理 ShareGPT 格式数据

        只计算 assistant 回复部分的损失，将 user 部分的 labels 设为 -100
        """
        conv = self.examples[idx]['conversations']

        # 获取角色配置
        human_role = self.sharegpt_config.get('human_role', 'human')
        assistant_role = self.sharegpt_config.get('assistant_role', 'gpt')

        # 构建文本
        text_parts = []
        label_parts = []

        for turn in conv:
            if turn['from'] == human_role:
                # user 输入：不计算损失
                user_text = f"User: {turn['value']}\n"
                text_parts.append(user_text)
                label_parts.append(None)  # 标记为不计算损失
            elif turn['from'] == assistant_role:
                # assistant 回复：计算损失
                assistant_text = f"Assistant: {turn['value']}\n"
                text_parts.append(assistant_text)
                label_parts.append(True)  # 标记为计算损失

        # 添加最后的提示符
        text_parts.append("Assistant:")
        label_parts.append(False)  # 提示符不计算损失

        # 合并文本
        full_text = "".join(text_parts)

        # tokenize
        enc = self.tokenizer(
            full_text,
            truncation=True,
            padding='max_length',
            max_length=self.max_length,
            return_tensors='pt'
        )

        input_ids = enc['input_ids'].squeeze()
        attention_mask = enc['attention_mask'].squeeze()

        # 构建正确的 labels：只计算 assistant 回复部分
        labels = input_ids.clone()

        # 计算各部分的 token 范围
        if len(text_parts) > 1:
            cumulative_text = ""
            token_positions = []

            # 逐部分累积文本并记录对应的 token 范围
            for part in text_parts:
                part_start = len(self.tokenizer(
                    cumulative_text,
                    return_tensors='pt',
                    add_special_tokens=False
                )['input_ids'][0])

                cumulative_text += part

                part_end = len(self.tokenizer(
                    cumulative_text,
                    return_tensors='pt',
                    add_special_tokens=False
                )['input_ids'][0])

                token_positions.append((part_start, part_end))

            # 将应该忽略的部分设为 -100
            for _, (pos, should_label) in enumerate(zip(token_positions, label_parts)):
                if not should_label:
                    start_pos, end_pos = pos
                    if start_pos < len(labels):
                        end_pos = min(end_pos, len(labels))
                        labels[start_pos:end_pos] = -100

        return {
            'input_ids': input_ids,
            'attention_mask': attention_mask,
            'labels': labels
        }

    def _process_alpaca(self, idx):
        """处理 Alpaca 格式数据

        Alpaca 格式包含:
        - instruction: 指令
        - input: 输入（可选）
        - output: 输出
        """
        example = self.examples[idx]

        # 验证必需字段
        if 'instruction' not in example or 'output' not in example:
            raise ValueError("Alpaca 格式缺少必需字段 'instruction' 或 'output'")

        instruction = example['instruction']
        input_text = example.get('input', '')
        output = example['output']

        # 构建提示词
        if input_text:
            prompt = f"""Below is an instruction that describes a task, paired with an input that provides further context. Write a response that appropriately completes the request.

### Instruction:
{instruction}

### Input:
{input_text}

### Response:
"""
        else:
            prompt = f"""Below is an instruction that describes a task. Write a response that appropriately completes the request.

### Instruction:
{instruction}

### Response:
"""

        # 完整文本 = prompt + output
        full_text = prompt + output

        # tokenize 完整文本
        enc = self.tokenizer(
            full_text,
            truncation=True,
            padding='max_length',
            max_length=self.max_length,
            return_tensors='pt'
        )

        input_ids = enc['input_ids'].squeeze()
        attention_mask = enc['attention_mask'].squeeze()

        # 构建 labels：只计算 output 部分
        labels = input_ids.clone()

        # 找到 prompt 的结束位置
        prompt_enc = self.tokenizer(
            prompt,
            return_tensors='pt',
            add_special_tokens=False
        )
        prompt_len = prompt_enc['input_ids'].shape[1]

        # 将 prompt 部分设为 -100（不计算损失）
        labels[:prompt_len] = -100

        return {
            'input_ids': input_ids,
            'attention_mask': attention_mask,
            'labels': labels
        }
