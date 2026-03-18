# scripts/utils/data_utils.py
import re
import json

def clean_text(text):
    """基础清洗：去除多余空格、表情符号等"""
    text = re.sub(r'\s+', ' ', text)
    text = re.sub(r'\[.*?\]', '', text)  # 去除表情符号标记
    return text.strip()

def convert_raw_to_sharegpt(raw_lines, self_name='我', other_name='对方'):
    """将简单的交替对话转换为ShareGPT格式"""
    conversations = []
    current_conv = []
    for line in raw_lines:
        if ':' not in line:
            continue
        speaker, content = line.split(':', 1)
        speaker = speaker.strip()
        content = clean_text(content)
        role = 'human' if speaker == self_name else 'gpt'
        current_conv.append({"from": role, "value": content})
        # 假设每两轮为一个完整对话，可根据实际调整
        if len(current_conv) >= 2:
            conversations.append({"conversations": current_conv})
            current_conv = []
    return conversations
