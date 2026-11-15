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
                                raise ValueError(f"第 {line_num} 行 JSON 解析失败: {e}") from e
                    return examples
                elif self.data_format == 'alpaca':
                    # 每行是一个JSON对象
                    examples = []
                    for line_num, line in enumerate(f, 1):
                        if line.strip():
                            try:
                                examples.append(json.loads(line))
                            except json.JSONDecodeError as e:
                                raise ValueError(f"第 {line_num} 行 JSON 解析失败: {e}") from e
                    return examples
                else:
                    # raw 格式：每行是一个文本
                    return [line.strip() for line in f if line.strip()]
        except UnicodeDecodeError as e:
            raise ValueError(f"文件编码错误: {e}") from e
        except IOError as e:
            raise IOError(f"读取文件失败: {e}") from e

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
            raise ValueError(f"处理第 {idx} 个样本时出错: {e}") from e

    def _process_raw(self, idx):
        """处理 raw 格式数据"""
        text = self.examples[idx]

        enc = self.tokenizer(
            text,
            truncation=True,
            max_length=self.max_length,
            return_tensors='pt'
        )

        input_ids = enc['input_ids'][0]
        attention_mask = enc['attention_mask'][0]

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
        """ShareGPT：仅 assistant 回复计损失（offset_mapping 一次定位，O(n)）。"""
        conv = self.examples[idx]['conversations']
        human_role = self.sharegpt_config.get('human_role', 'human')
        assistant_role = self.sharegpt_config.get('assistant_role', 'gpt')

        parts = []
        masked_spans = []        # 需置 -100 的字符区间
        cursor = 0
        for turn in conv:
            role = turn.get('from')
            if role == human_role:
                seg = f"User: {turn['value']}\n"
                masked_spans.append((cursor, cursor + len(seg)))
            elif role == assistant_role:
                seg = f"Assistant: {turn['value']}\n"
            else:
                continue
            parts.append(seg)
            cursor += len(seg)
        tail = "Assistant:"
        parts.append(tail)
        masked_spans.append((cursor, cursor + len(tail)))
        full_text = "".join(parts)

        try:
            enc = self.tokenizer(
                full_text,
                truncation=True,
                max_length=self.max_length,
                return_tensors='pt',
                return_offsets_mapping=True,
            )
        except (ValueError, NotImplementedError):
            # 无 offset 时回退逐段累积
            return self._process_sharegpt_slow(idx)

        input_ids = enc['input_ids'][0]
        attention_mask = enc['attention_mask'][0]
        labels = input_ids.clone()
        offsets = enc.get('offset_mapping')
        if offsets is None:
            return self._process_sharegpt_slow(idx)
        for tok_idx, (tok_start, tok_end) in enumerate(offsets[0].tolist()):
            if tok_start == tok_end:          # 特殊 token 保留
                continue
            if any(tok_start < m_end and tok_end > m_start
                   for m_start, m_end in masked_spans):
                labels[tok_idx] = -100
        return {
            'input_ids': input_ids,
            'attention_mask': attention_mask,
            'labels': labels,
        }

    def _process_sharegpt_slow(self, idx):
        """无 offset 时的回退实现。"""
        conv = self.examples[idx]['conversations']
        human_role = self.sharegpt_config.get('human_role', 'human')
        assistant_role = self.sharegpt_config.get('assistant_role', 'gpt')

        text_parts = []
        label_parts = []
        for turn in conv:
            if turn['from'] == human_role:
                text_parts.append(f"User: {turn['value']}\n")
                label_parts.append(None)      # 不计算损失
            elif turn['from'] == assistant_role:
                text_parts.append(f"Assistant: {turn['value']}\n")
                label_parts.append(True)      # 计算损失
        text_parts.append("Assistant:")
        label_parts.append(False)
        full_text = "".join(text_parts)

        enc = self.tokenizer(
            full_text,
            truncation=True,
            max_length=self.max_length,
            return_tensors='pt',
        )
        input_ids = enc['input_ids'][0]
        attention_mask = enc['attention_mask'][0]
        labels = input_ids.clone()

        if len(text_parts) > 1:
            cumulative_text = ""
            token_positions = []
            for part in text_parts:
                part_start = len(self.tokenizer(
                    cumulative_text, return_tensors='pt', add_special_tokens=False
                )['input_ids'][0])
                cumulative_text += part
                part_end = len(self.tokenizer(
                    cumulative_text, return_tensors='pt', add_special_tokens=False
                )['input_ids'][0])
                token_positions.append((part_start, part_end))
            for (pos, should_label) in zip(token_positions, label_parts):
                if not should_label:
                    start_pos, end_pos = pos
                    if start_pos < len(labels):
                        end_pos = min(end_pos, len(labels))
                        labels[start_pos:end_pos] = -100

        return {
            'input_ids': input_ids,
            'attention_mask': attention_mask,
            'labels': labels,
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
            max_length=self.max_length,
            return_tensors='pt'
        )

        input_ids = enc['input_ids'][0]
        attention_mask = enc['attention_mask'][0]

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
