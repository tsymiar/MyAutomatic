#!/usr/bin/env python3
# scripts/prepare_data.py
import os
import sys
import json
import argparse

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from scripts.utils.data_utils import clean_text, convert_raw_to_sharegpt

# 将原始对话文本转换为ShareGPT格式并保存为JSON格式
def prepare_txt(input_file, output_file, self_name='我', other_name='对方'):
    # 验证输入文件存在性
    if not os.path.exists(input_file):
        raise FileNotFoundError(f"输入文件不存在: {input_file}")

    # 确保输出目录存在
    output_dir = os.path.dirname(output_file)
    if output_dir and not os.path.exists(output_dir):
        os.makedirs(output_dir, exist_ok=True)

    # 读取并转换数据
    try:
        with open(input_file, 'r', encoding='utf-8') as f:
            lines = f.readlines()
    except UnicodeDecodeError as e:
        raise ValueError(f"文件编码错误: {e}")
    except IOError as e:
        raise IOError(f"读取文件失败: {e}")

    # 转换为ShareGPT格式
    try:
        conversations = convert_raw_to_sharegpt(lines, self_name, other_name)
    except Exception as e:
        raise ValueError(f"数据转换失败: {e}")

    # 写入输出文件
    try:
        with open(output_file, 'w', encoding='utf-8') as f:
            for conv in conversations:
                f.write(json.dumps(conv, ensure_ascii=False) + '\n')
    except IOError as e:
        raise IOError(f"写入输出文件失败: {e}")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='将原始对话文本转换为ShareGPT格式')
    parser.add_argument('--input', type=str, required=True, help='原始对话文件路径')
    parser.add_argument('--output', type=str, required=True, help='输出JSON文件路径')
    parser.add_argument('--self_name', type=str, default='我', help='自己的称呼')
    parser.add_argument('--other_name', type=str, default='对方', help='对方的称呼')
    args = parser.parse_args()

    try:
        prepare_txt(args.input, args.output, args.self_name, args.other_name)
        print(f"✓ 转换完成，已存入 {args.output}")
    except FileNotFoundError as e:
        print(f"✗ 错误: {e}", file=sys.stderr)
        sys.exit(1)
    except ValueError as e:
        print(f"✗ 错误: {e}", file=sys.stderr)
        sys.exit(1)
    except IOError as e:
        print(f"✗ 错误: {e}", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"✗ 未知错误: {e}", file=sys.stderr)
        sys.exit(1)
