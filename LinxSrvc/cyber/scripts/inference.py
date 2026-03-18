#!/usr/bin/env python3
# scripts/inference.py
import sys
import os
import argparse
import torch

# 设置项目路径
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from scripts.utils.model_utils import load_lora_model

def validate_history(history):
    """验证历史对话格式"""
    if not isinstance(history, list):
        raise ValueError("history 必须是列表")
    for turn in history:
        if not isinstance(turn, dict):
            raise ValueError("history 元素必须是字典")
        if 'user' not in turn or 'assistant' not in turn:
            raise ValueError("history 元素必须包含 'user' 和 'assistant' 字段")

def validate_generation_params(max_new_tokens, temperature, top_p, repetition_penalty):
    """验证生成参数的有效性"""
    if max_new_tokens <= 0:
        raise ValueError("max_new_tokens 必须大于 0")
    if max_new_tokens > 4096:
        print("⚠ 警告: max_new_tokens 过大可能导致显存不足")

    if temperature < 0:
        raise ValueError("temperature 必须大于等于 0")
    if temperature == 0:
        print("⚠ temperature=0 将使用贪婪解码")

    if top_p <= 0 or top_p > 1:
        raise ValueError("top_p 必须在 (0, 1] 范围内")

    if repetition_penalty < 1.0:
        raise ValueError("repetition_penalty 必须大于等于 1.0")

def clean_response(response):
    """清理模型响应，去除多余的空白和换行"""
    response = response.strip()
    # 去除开头的换行符
    while response.startswith('\n'):
        response = response[1:]
    # 去除结尾的换行符
    while response.endswith('\n'):
        response = response[:-1]
    return response

def chat(model, tokenizer, message, history=None, max_new_tokens=512, temperature=0.7,
         top_p=0.9, max_history_len=10, repetition_penalty=1.0, top_k=50):
    """
    与模型进行对话

    Args:
        model: 加载的语言模型
        tokenizer: 对应的tokenizer
        message: 用户消息
        history: 历史对话记录，格式为 [{"user": "...", "assistant": "..."}, ...]
        max_new_tokens: 生成最大token数
        temperature: 采样温度，0 表示贪婪解码
        top_p: nucleus采样参数
        max_history_len: 最大历史轮数，避免内存溢出
        repetition_penalty: 重复惩罚系数
        top_k: top-k采样参数

    Returns:
        模型响应文本
    """
    # 参数验证
    if not message or not message.strip():
        raise ValueError("消息不能为空")

    validate_generation_params(max_new_tokens, temperature, top_p, repetition_penalty)

    if history is None:
        history = []

    validate_history(history)

    # 限制历史长度，避免内存溢出
    if len(history) > max_history_len:
        history = history[-max_history_len:]

    # 构建prompt
    prompt = ""
    for turn in history:
        prompt += f"User: {turn['user']}\nAssistant: {turn['assistant']}\n"
    prompt += f"User: {message}\nAssistant:"

    try:
        inputs = tokenizer(prompt, return_tensors="pt").to(model.device)

        # 设置生成参数
        generation_kwargs = {
            "max_new_tokens": max_new_tokens,
            "pad_token_id": tokenizer.eos_token_id,
        }

        # 采样参数
        if temperature > 0:
            generation_kwargs.update({
                "temperature": temperature,
                "top_p": top_p,
                "top_k": top_k,
                "do_sample": True,
                "repetition_penalty": repetition_penalty,
            })
        else:
            # 贪婪解码
            generation_kwargs["do_sample"] = False

        outputs = model.generate(**inputs, **generation_kwargs)

        # 解码响应（只包含新生成的部分）
        response = tokenizer.decode(outputs[0][inputs['input_ids'].shape[1]:], skip_special_tokens=True)
        response = clean_response(response)
        return response

    except torch.cuda.OutOfMemoryError:
        raise RuntimeError("显存不足，请尝试减小 max_new_tokens 或使用更小的模型")
    except Exception as e:
        raise RuntimeError(f"推理失败: {e}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="数字分身推理脚本")
    parser.add_argument("--base_model", type=str, required=True, help="基础模型路径或名称")
    parser.add_argument("--lora_path", type=str, default=None, help="LoRA权重路径")
    parser.add_argument("--load_in_4bit", action="store_true", help="是否使用4bit加载")
    parser.add_argument("--max_history", type=int, default=10, help="最大历史轮数")
    parser.add_argument("--max_new_tokens", type=int, default=512, help="最大生成token数")
    parser.add_argument("--temperature", type=float, default=0.7, help="采样温度")
    parser.add_argument("--top_p", type=float, default=0.9, help="nucleus采样参数")
    parser.add_argument("--top_k", type=int, default=50, help="top-k采样参数")
    parser.add_argument("--repetition_penalty", type=float, default=1.0, help="重复惩罚系数")
    args = parser.parse_args()

    # 检查CUDA可用性
    if torch.cuda.is_available():
        print(f"✓ 检测到CUDA设备: {torch.cuda.get_device_name(0)}")
        print(f"  显存: {torch.cuda.get_device_properties(0).total_memory / 1024**3:.1f} GB")
    else:
        print("⚠ 未检测到CUDA设备，将使用CPU推理（速度较慢）")

    # 加载模型
    try:
        print("正在加载模型...")
        model, tokenizer = load_lora_model(args.base_model, args.lora_path, args.load_in_4bit)
        print("✓ 模型加载成功")
    except Exception as e:
        print(f"✗ 模型加载失败: {e}", file=sys.stderr)
        sys.exit(1)

    print("\n" + "="*50)
    print("数字分身已启动，输入 'quit' 退出")
    print("参数:")
    print(f"  max_history: {args.max_history}")
    print(f"  max_new_tokens: {args.max_new_tokens}")
    print(f"  temperature: {args.temperature}")
    print(f"  top_p: {args.top_p}")
    print(f"  top_k: {args.top_k}")
    print(f"  repetition_penalty: {args.repetition_penalty}")
    print("="*50 + "\n")

    history = []
    while True:
        try:
            user_input = input("你: ")

            # 退出命令
            if user_input.lower() in ['quit', 'exit', 'q']:
                print("再见！")
                break

            # 跳过空输入
            if not user_input.strip():
                continue

            # 推理
            try:
                response = chat(
                    model, tokenizer, user_input, history,
                    max_history_len=args.max_history,
                    max_new_tokens=args.max_new_tokens,
                    temperature=args.temperature,
                    top_p=args.top_p,
                    top_k=args.top_k,
                    repetition_penalty=args.repetition_penalty
                )
                print(f"分身: {response}\n")

                # 更新历史
                history.append({"user": user_input, "assistant": response})

            except ValueError as e:
                print(f"✗ 参数错误: {e}\n", file=sys.stderr)
            except RuntimeError as e:
                print(f"✗ 推理错误: {e}\n", file=sys.stderr)
                # 清空历史，避免累积错误
                if "显存不足" in str(e):
                    print("提示: 可以尝试减小 max_new_tokens 或历史长度，或重启程序\n")
                    history = []
            except Exception as e:
                print(f"✗ 未知错误: {e}\n", file=sys.stderr)

        except KeyboardInterrupt:
            print("\n\n检测到中断，退出程序...")
            break
        except EOFError:
            print("\n\n输入结束，退出程序...")
            break
