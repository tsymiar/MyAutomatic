#!/usr/bin/env python3
# scripts/utils/model_utils.py
"""模型加载和处理工具函数"""
import torch
from transformers import AutoTokenizer, AutoModelForCausalLM
from peft import PeftModel


def load_tokenizer(model_path):
    try:
        tokenizer = AutoTokenizer.from_pretrained(
            model_path,
            trust_remote_code=True,
            use_fast=True
        )
        return tokenizer
    except Exception as e:
        raise RuntimeError(f"加载 tokenizer 失败: {e}")


def load_base_model(model_path, load_in_4bit=False, torch_dtype=torch.float16):
    try:
        model_kwargs = {
            "torch_dtype": torch_dtype,
            "device_map": "auto",
            "trust_remote_code": True,
        }

        if load_in_4bit:
            from transformers import BitsAndBytesConfig

            quantization_config = BitsAndBytesConfig(
                load_in_4bit=True,
                bnb_4bit_compute_dtype=torch_dtype,
                bnb_4bit_use_double_quant=True,
                bnb_4bit_quant_type="nf4",
            )
            model_kwargs["quantization_config"] = quantization_config

        model = AutoModelForCausalLM.from_pretrained(
            model_path,
            **model_kwargs
        )

        # 设置模型为评估模式
        model.eval()

        return model

    except ImportError:
        raise RuntimeError(
            "加载 4-bit 模型需要 bitsandbytes 库。"
            "请运行: pip install bitsandbytes"
        )
    except Exception as e:
        raise RuntimeError(f"加载模型失败: {e}")


def load_lora_model(base_model_path, lora_path=None, load_in_4bit=False):
    # 加载 tokenizer
    tokenizer = load_tokenizer(base_model_path)

    # 加载基础模型
    model = load_base_model(base_model_path, load_in_4bit=load_in_4bit)

    # 如果有 LoRA 权重，加载它们
    if lora_path is not None:
            try:
                model = PeftModel.from_pretrained(
                    model,
                    lora_path,
                    torch_dtype=model.dtype,
                    device_map="auto"
                )
                # 合并权重以提高推理速度
                try:
                    model = model.merge_and_unload()
                    model.eval()  # 合并后确保模型处于评估模式
                except ValueError:
                    # 如果模型不支持合并，则继续使用原始 PEFT 模型
                    model.eval()
                except Exception as e:
                    raise RuntimeError(f"合并 LoRA 权重失败: {e}")
            except Exception as e:
                raise RuntimeError(f"加载 LoRA 权重失败: {e}")
    return model, tokenizer
