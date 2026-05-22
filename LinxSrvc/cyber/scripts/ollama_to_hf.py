#!/usr/bin/env python3
# scripts/ollama_to_hf.py
"""将本地 Ollama 模型转换为 Hugging Face 格式"""
import os
import sys
import re
import json
import shutil
import logging
import argparse
import subprocess
from pathlib import Path

# 注意: transformers 库在函数内部按需导入以加快启动速度
# 各函数在需要时通过 from transformers import ... 延迟加载

# 添加项目根目录到路径
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# 全局常量: sha256 格式文件名正则 (64位十六进制)
SHA256_PATTERN = re.compile(r'^[a-f0-9]{64}$')


def setup_logging(verbose=False):
    """设置日志"""
    level = logging.DEBUG if verbose else logging.INFO
    logging.basicConfig(
        level=level,
        format='%(asctime)s - %(levelname)s - %(message)s'
    )


def run_command(cmd, cwd=None, capture_output=True):
    """
    运行 shell 命令

    Args:
        cmd: 命令列表
        cwd: 工作目录
        capture_output: 是否捕获输出

    Returns:
        (returncode, stdout, stderr)
    """
    logging.debug(f"执行命令: {' '.join(cmd)}")

    result = subprocess.run(
        cmd,
        cwd=cwd,
        capture_output=capture_output,
        text=True,
        check=False
    )

    if result.stdout and logging.getLogger().level <= logging.DEBUG:
        for line in result.stdout.split('\n'):
            if line.strip():
                logging.debug(f"  {line}")

    if result.stderr and logging.getLogger().level <= logging.DEBUG:
        for line in result.stderr.split('\n'):
            if line.strip():
                logging.debug(f"  [stderr] {line}")

    return result.returncode, result.stdout, result.stderr


def install_llama_cpp_converter():
    # 安装 llama.cpp-to-hf 转换工具
    tools_dir = Path("./.llama-tools")
    converter_dir = tools_dir / "llama.cpp-to-hf"

    # 检查是否已安装
    if converter_dir.exists():
        logging.info(f"llama.cpp-to-hf 已安装在: {converter_dir}")
        return converter_dir

    try:
        logging.info("开始安装 llama.cpp-to-hf 工具...")
        tools_dir.mkdir(parents=True, exist_ok=True)

        # 克隆仓库
        repo_url = "https://github.com/ggerganov/llama.cpp"
        returncode, stdout, stderr = run_command(
            ["git", "clone", repo_url, str(converter_dir)]
        )

        if returncode != 0:
            logging.error(f"克隆仓库失败: {stderr}")
            return None

        logging.info(f"llama.cpp-to-hf 已安装到: {converter_dir}")
        return converter_dir

    except Exception as e:
        logging.error(f"安装失败: {e}")
        return None


def find_ollama_model_files(ollama_model_name):
    """
    查找 Ollama 模型在本地存储的模型文件

    Args:
        ollama_model_name: Ollama 模型名称 (如 "llama2", "mistral:7b", "qwen:latest")

    Returns:
        字典，包含模型文件路径和格式信息
    """
    home = Path.home()

    # 常见的 Ollama 模型存储位置
    ollama_roots = [
        home / ".ollama" / "models",
        home / ".cache" / "ollama" / "models",
        Path("/usr/share/ollama/models"),
    ]

    # 检查 macOS 特定位置
    if sys.platform == "darwin":
        ollama_roots.append(home / "Library" / "Application Support" / "ollama" / "models")

    # 查找 Ollama 模型根目录
    ollama_root = None
    for root_dir in ollama_roots:
        if root_dir.exists():
            ollama_root = root_dir
            logging.info(f"找到 Ollama 模型根目录: {ollama_root}")
            break

    if ollama_root is None:
        error_msg = "找不到 Ollama 模型存储目录。已尝试的位置:\n"
        error_msg += "\n".join(f"  - {root}" for root in ollama_roots)
        raise FileNotFoundError(error_msg)

    # Ollama 使用 manifest 文件存储模型信息
    manifest_dir = ollama_root / "manifests"
    if not manifest_dir.exists():
        raise FileNotFoundError(f"Ollama manifest 目录不存在: {manifest_dir}")

    # 解析模型名称为 registry/org/model 格式
    # Ollama 格式通常是: library/model:tag 或 org/model:tag
    model_parts = ollama_model_name.split("/")

    if len(model_parts) == 1:
        # 只提供模型名，如 "llama2" 或 "qwen3.5:4b"
        model_with_tag = model_parts[0]
        if ":" in model_with_tag:
            model, tag = model_with_tag.split(":")
        else:
            model, tag = model_with_tag, "latest"
    elif len(model_parts) == 2:
        # "org/model:tag" 或 "org/model" 格式
        org_model = model_parts[1]
        if ":" in org_model:
            model, tag = org_model.split(":")
        else:
            model, tag = org_model, "latest"
    else:
        raise ValueError(f"无法解析模型名称: {ollama_model_name}")

    logging.info(f"解析模型名称 -> 模型: {model}, 标签: {tag}")

    # 尝试查找 manifest 文件
    # 路径格式: manifests/registry.ollama.ai/library/model/tag
    possible_manifest_paths = [
        manifest_dir / "registry.ollama.ai" / "library" / model / tag,
    ]

    # 也尝试旧格式 (向后兼容)
    possible_manifest_paths.append(manifest_dir / "library" / model / tag)

    manifest_path = None
    for path in possible_manifest_paths:
        if path.exists():
            manifest_path = path
            logging.info(f"找到 manifest 文件: {manifest_path}")
            break

    if manifest_path is None:
        # 尝试通过 ollama list 命令获取模型信息
        logging.info("未找到 manifest 文件，尝试通过 ollama 命令查找...")
        try:
            returncode, stdout, stderr = run_command(["ollama", "list"], capture_output=True)
            if returncode == 0 and stdout:
                # 解析 ollama list 输出
                for line in stdout.split('\n'):
                    if line.strip() and not line.startswith("NAME"):
                        # 格式: NAME              ID              SIZE
                        parts = line.split()
                        if parts and ollama_model_name in parts[0]:
                            model_display_name = parts[0]
                            logging.info(f"找到模型: {model_display_name}")
                            # 重新解析找到的完整模型名
                            return find_ollama_model_files(model_display_name)
        except Exception as e:
            logging.debug(f"ollama list 命令失败: {e}")

        error_msg = f"找不到 Ollama 模型 '{ollama_model_name}'。\n"
        error_msg += f"请确保已使用 'ollama pull {ollama_model_name}' 下载模型。\n"
        error_msg += "已尝试的 manifest 路径:\n"
        error_msg += "\n".join(f"  - {p}" for p in possible_manifest_paths)
        raise FileNotFoundError(error_msg)

    # 读取 manifest 文件获取 blob 引用
    try:
        with open(manifest_path, 'r') as f:
            manifest = json.load(f)

        logging.info(f"Manifest 内容结构: {list(manifest.keys())}")
        logging.debug(f"完整 manifest 内容: {manifest}")

        # 查找 GGUF blob (layers)
        if "layers" in manifest:
            layers = manifest["layers"]
            logging.info(f"Manifest 中有 {len(layers)} 个 layers")
            for idx, layer in enumerate(layers):
                if "digest" in layer and layer["digest"].startswith("sha256:"):
                    digest_full = layer["digest"]
                    digest = digest_full.split(":")[1]
                    digest_lower = digest.lower()

                    # 尝试多种可能的 blob 路径格式
                    possible_blob_paths = [
                        # 新格式: blobs/sha256/{digest[:2]}/{digest}
                        ollama_root / "blobs" / "sha256" / digest_lower[:2] / digest_lower,
                        # 旧格式: blobs/sha256-{digest}
                        ollama_root / "blobs" / f"sha256-{digest_lower}",
                    ]

                    for blob_path in possible_blob_paths:
                        logging.info(f"检查 blob [{idx}]: {blob_path}")
                        if blob_path.exists():
                            logging.info(f"找到模型文件: {blob_path}")
                            # 检测文件格式
                            if blob_path.suffix:
                                file_format = blob_path.suffix[1:]
                            else:
                                # 无扩展名，尝试检测格式
                                try:
                                    file_format = detect_model_format(blob_path)
                                except Exception:
                                    file_format = "gguf"  # 默认假设为 GGUF

                            return {"format": file_format, "files": [blob_path], "manifest": manifest_path}
                        else:
                            logging.debug(f"Blob 文件不存在: {blob_path}")
                else:
                    logging.debug(f"Layer {idx} 没有 sha256 digest: {layer.get('digest', 'N/A')[:20]}")
        else:
            logging.warning("Manifest 文件中未找到 'layers' 字段")
            logging.info(f"Manifest 键: {list(manifest.keys())}")
            # 检查是否有其他可能的字段
            if "config" in manifest:
                logging.info("找到 config 字段")
            if "mediaType" in manifest:
                logging.info(f"Media type: {manifest['mediaType']}")

    except Exception as e:
        logging.error(f"读取 manifest 失败: {e}")
        import traceback
        logging.debug(traceback.format_exc())

    # 如果通过 manifest 找不到，尝试在 blobs 目录中搜索
    logging.info("在 blobs 目录中搜索模型文件...")

    # 搜索多种可能的 blob 存储格式
    blob_search_dirs = [
        ollama_root / "blobs" / "sha256",  # 新格式: blobs/sha256/{digest[:2]}/{digest}
        ollama_root / "blobs",             # 旧格式: blobs/sha256-{digest}
    ]

    all_model_files = []

    for search_dir in blob_search_dirs:
        if not search_dir.exists():
            continue

        if search_dir.name == "sha256":
            # 新格式: 遍历子目录（每个子目录以 digest 的前两位命名）
            for subdir in search_dir.iterdir():
                if subdir.is_dir():
                    # 搜索各种格式的模型文件
                    gguf_files = list(subdir.glob("*.gguf"))
                    safetensors_files = list(subdir.glob("*.safetensors"))
                    bin_files = list(subdir.glob("*.bin"))
                    pt_files = list(subdir.glob("*.pt"))

                    # 搜索 sha256 命名的文件（无扩展名）
                    sha256_files = []
                    for f in subdir.iterdir():
                        if f.is_file() and not f.suffix and SHA256_PATTERN.match(f.name.lower()):
                            sha256_files.append(f)

                    all_model_files.extend(gguf_files + safetensors_files + bin_files + pt_files + sha256_files)
        else:
            # 旧格式: blobs/sha256-{digest}
            for blob_file in search_dir.iterdir():
                if blob_file.is_file() and blob_file.name.startswith("sha256-"):
                    # 提取 sha256 部分
                    sha256_part = blob_file.name[7:]  # 移除 "sha256-" 前缀
                    # 检查是否为有效的 sha256
                    if SHA256_PATTERN.match(sha256_part.lower()):
                        all_model_files.append(blob_file)

    if all_model_files:
        # 按修改时间排序，取最新的
        all_model_files.sort(key=lambda x: x.stat().st_mtime, reverse=True)
        latest_file = all_model_files[0]

        # 检测文件格式
        if latest_file.suffix:
            file_format = latest_file.suffix[1:]
        else:
            # 无扩展名，尝试检测格式
            try:
                file_format = detect_model_format(latest_file)
            except Exception:
                file_format = "gguf"  # 默认假设为 GGUF

        logging.info(f"找到 {len(all_model_files)} 个模型文件，使用最新的: {latest_file}")
        logging.info(f"检测到文件格式: {file_format}")
        return {"format": file_format, "files": [latest_file], "manifest": None}

    raise FileNotFoundError(f"在 {ollama_root} 中找不到模型文件，已找到 manifest 但无法定位 blob 文件")


def find_model_files_in_path(model_dir):
    """
    在指定目录中查找模型权重和配置文件

    Args:
        model_dir: 模型目录路径

    Returns:
        字典，包含模型文件路径信息
    """
    model_dir = Path(model_dir)

    # 查找 GGUF 文件 (Ollama 常用格式)
    gguf_files = list(model_dir.glob("*.gguf"))
    safetensors_files = list(model_dir.glob("*.safetensors"))
    bin_files = list(model_dir.glob("*.bin"))
    pt_files = list(model_dir.glob("*.pt"))

    if gguf_files:
        logging.info(f"找到 GGUF 格式模型文件: {len(gguf_files)} 个")
        return {"format": "gguf", "files": gguf_files}
    elif safetensors_files:
        logging.info(f"找到 SafeTensors 格式模型文件: {len(safetensors_files)} 个")
        return {"format": "safetensors", "files": safetensors_files}
    elif bin_files:
        logging.info(f"找到 PyTorch Bin 格式模型文件: {len(bin_files)} 个")
        return {"format": "bin", "files": bin_files}
    elif pt_files:
        logging.info(f"找到 PyTorch PT 格式模型文件: {len(pt_files)} 个")
        return {"format": "pt", "files": pt_files}
    else:
        raise FileNotFoundError("未找到支持的模型文件格式 (gguf, safetensors, bin, pt)")


def convert_gguf_to_hf_complete(gguf_path, output_path, hf_model_name=None, tokenizer_name=None, force_install=False):
    """
    使用 llama.cpp-to-hf 工具将 GGUF 格式模型完整转换为 Hugging Face 格式

    Args:
        gguf_path: GGUF 模型文件路径
        output_path: 输出目录路径
        hf_model_name: 对应的 Hugging Face 模型名称 (用于加载 tokenizer)
        tokenizer_name: 指定的 tokenizer 名称
        force_install: 强制重新安装转换工具
    """
    try:
        from transformers import AutoTokenizer

        logging.info("开始完整转换 GGUF 模型...")

        # 安装转换工具
        converter_dir = install_llama_cpp_converter()
        if converter_dir is None:
            raise RuntimeError("无法安装 llama.cpp-to-hf 工具")

        # 如果没有指定 tokenizer 名称，尝试从模型名称推断
        if tokenizer_name is None:
            if hf_model_name:
                tokenizer_name = hf_model_name
            else:
                raise ValueError("需要指定 Hugging Face 模型名称以加载 tokenizer")

        # 加载并保存 tokenizer
        logging.info(f"从 Hugging Face 加载 tokenizer: {tokenizer_name}")
        try:
            tokenizer = AutoTokenizer.from_pretrained(tokenizer_name, trust_remote_code=True)
            output_path = Path(output_path)
            output_path.mkdir(parents=True, exist_ok=True)
            tokenizer.save_pretrained(str(output_path))
            logging.info(f"Tokenizer 已保存到: {output_path}")
        except Exception as e:
            logging.error(f"无法加载 tokenizer: {e}")
            raise

        # 使用 llama.cpp-to-hf 进行转换
        logging.info("使用 llama.cpp-to-hf 转换模型权重...")

        # 检查转换脚本
        convert_script = converter_dir / "convert.py"
        if not convert_script.exists():
            logging.error(f"转换脚本不存在: {convert_script}")
            raise RuntimeError("llama.cpp-to-hf 工具安装不完整")

        # 执行转换
        returncode, stdout, stderr = run_command(
            ["python", str(convert_script), str(gguf_path), str(output_path)]
        )

        if returncode != 0:
            logging.error(f"转换失败: {stderr}")
            raise RuntimeError(f"llama.cpp-to-hf 转换失败，返回码: {returncode}")

        # 验证转换结果
        converted_files = list(output_path.glob("*.safetensors")) + list(output_path.glob("*.bin"))
        if not converted_files:
            logging.warning("未找到转换后的模型权重文件")
        else:
            logging.info(f"转换完成，生成的模型文件: {[f.name for f in converted_files]}")

        logging.info(f"完整转换成功！输出目录: {output_path}")

    except Exception as e:
        logging.error(f"GGUF 完整转换失败: {e}")
        raise


def convert_gguf_to_hf(
    gguf_path,
    output_path,
    hf_model_name=None,
    tokenizer_name=None,
    use_full_conversion=True,
    force_install=False
):
    """
    将 GGUF 格式模型转换为 Hugging Face 格式

    Args:
        gguf_path: GGUF 模型文件路径
        output_path: 输出目录路径
        hf_model_name: 对应的 Hugging Face 模型名称（用于加载 tokenizer）
        tokenizer_name: 指定的 tokenizer 名称
        use_full_conversion: 是否使用完整转换（默认 True）
        force_install: 强制重新安装转换工具
    """
    try:
        if use_full_conversion:
            # 使用完整的 llama.cpp-to-hf 工具链
            logging.info("使用完整转换模式...")
            convert_gguf_to_hf_complete(
                gguf_path, output_path, hf_model_name, tokenizer_name, force_install
            )
        else:
            # 使用简化转换模式
            logging.info("使用简化转换模式...")
            from transformers import AutoTokenizer
            from llama_cpp import Llama

            # 如果没有指定 tokenizer 名称，尝试从模型名称推断
            if tokenizer_name is None:
                if hf_model_name:
                    tokenizer_name = hf_model_name
                else:
                    raise ValueError("需要指定 Hugging Face 模型名称以加载 tokenizer")

            # 加载 tokenizer
            logging.info(f"从 Hugging Face 加载 tokenizer: {tokenizer_name}")
            try:
                tokenizer = AutoTokenizer.from_pretrained(tokenizer_name, trust_remote_code=True)
            except Exception as e:
                logging.error(f"无法加载 tokenizer: {e}")
                raise

            # 加载 GGUF 模型
            logging.info(f"加载 GGUF 模型: {gguf_path}")
            llm = Llama(
                model_path=str(gguf_path),
                n_gpu_layers=-1,
                verbose=False
            )

            # 获取模型配置
            model_config = {
                "architectures": ["LlamaForCausalLM"],
                "hidden_size": llm.model.params.n_embd,
                "num_hidden_layers": llm.model.params.n_layer,
                "num_attention_heads": llm.model.params.n_head,
                "vocab_size": llm.model.params.n_vocab,
                "rms_norm_eps": llm.model.params.f_norm_eps,
                "max_position_embeddings": llm.model.params.n_ctx,
                "torch_dtype": "float16",
                "transformers_version": "4.36.0"
            }

            logging.info(f"模型配置: {model_config}")

            # 保存 tokenizer
            output_path = Path(output_path)
            output_path.mkdir(parents=True, exist_ok=True)

            logging.info(f"保存 tokenizer 到: {output_path}")
            tokenizer.save_pretrained(str(output_path))

            # 保存配置文件
            config_path = output_path / "config.json"
            with open(config_path, 'w', encoding='utf-8') as f:
                json.dump(model_config, f, indent=2)

            logging.info(f"配置文件已保存到: {config_path}")
            logging.warning("简化模式未转换模型权重文件。使用 --full-conversion 进行完整转换。")

    except ImportError:
        raise RuntimeError(
            "GGUF 转换需要 llama-cpp-python 库。\n"
            "请运行: pip install llama-cpp-python\n"
            "或者: CMAKE_ARGS=\"-DGGML_CUDA=on\" pip install "
            "llama-cpp-python (支持 CUDA)"
        )
    except Exception as e:
        logging.error(f"GGUF 转换失败: {e}")
        raise


def convert_safetensors_to_hf(
    model_files,
    output_path,
    hf_model_name=None,
    tokenizer_name=None,
    use_full_conversion=True
):
    """
    将 SafeTensors 格式模型转换为 Hugging Face 格式

    Args:
        model_files: SafeTensors 模型文件列表
        output_path: 输出目录路径
        hf_model_name: 对应的 Hugging Face 模型名称
        tokenizer_name: 指定的 tokenizer 名称
        use_full_conversion: 是否使用完整转换（加载模型结构）
    """
    try:
        from transformers import AutoModelForCausalLM, AutoTokenizer, AutoConfig

        logging.info("开始转换 SafeTensors 模型...")

        # 确定使用的 tokenizer
        if tokenizer_name is None:
            tokenizer_name = hf_model_name
        if tokenizer_name is None:
            raise ValueError("需要指定 Hugging Face 模型名称以加载 tokenizer")

        # 加载 tokenizer
        logging.info(f"从 Hugging Face 加载 tokenizer: {tokenizer_name}")
        tokenizer = AutoTokenizer.from_pretrained(tokenizer_name, trust_remote_code=True)

        # 创建输出目录
        output_path = Path(output_path)
        output_path.mkdir(parents=True, exist_ok=True)

        # 根据转换模式选择不同的处理方式
        if use_full_conversion and hf_model_name:
            # 完整转换模式：加载模型结构和配置
            logging.info(f"从 Hugging Face 加载模型配置: {hf_model_name}")
            config = AutoConfig.from_pretrained(hf_model_name, trust_remote_code=True)

            # 加载模型结构（不加载权重，仅获取配置）
            logging.info("加载模型结构...")
            try:
                model = AutoModelForCausalLM.from_pretrained(
                    hf_model_name,
                    config=config,
                    trust_remote_code=True,
                    low_cpu_mem_usage=True  # 减少内存占用
                )
                logging.info(f"模型配置: {model.config.to_dict()}")
            except Exception as e:
                logging.warning(f"无法加载模型结构: {e}，使用基础配置")
                model = None
        else:
            # 简化模式：仅复制文件并保存基础配置
            logging.info("使用简化转换模式，仅复制文件...")
            model = None

            # 加载模型配置（如果可用）
            if hf_model_name:
                logging.info(f"从 Hugging Face 加载模型配置: {hf_model_name}")
                config = AutoConfig.from_pretrained(hf_model_name, trust_remote_code=True)
            else:
                # 如果没有指定 HF 模型名称，尝试从模型文件推断
                logging.warning("未指定 Hugging Face 模型名称，尝试从模型文件加载配置...")
                model_path = model_files[0].parent
                config = AutoConfig.from_pretrained(str(model_path), trust_remote_code=True)

        # 复制模型文件（两种模式共用）
        logging.info(f"复制模型文件到: {output_path}")
        for model_file in model_files:
            dest = output_path / model_file.name
            shutil.copy2(str(model_file), str(dest))
            logging.info(f"已复制: {model_file.name}")

        # 保存 tokenizer 和配置
        logging.info("保存 tokenizer 和配置...")
        tokenizer.save_pretrained(str(output_path))
        config.save_pretrained(str(output_path))

        if model is not None:
            # 完整转换模式额外保存模型配置
            model.config.save_pretrained(str(output_path))
            logging.info(f"模型配置已保存到: {output_path}")

        logging.info(f"转换完成！输出目录: {output_path}")

    except Exception as e:
        logging.error(f"SafeTensors 转换失败: {e}")
        raise


def convert_ollama_model(
    ollama_model_name,
    output_path,
    hf_model_name=None,
    tokenizer_name=None,
    format=None,
    use_full_conversion=True,
    force_install=False
):
    """
    将 Ollama 模型转换为 Hugging Face 格式

    Args:
        ollama_model_name: Ollama 模型名称
        output_path: Hugging Face 格式模型输出路径
        hf_model_name: 对应的 Hugging Face 模型名称 (可选)
        tokenizer_name: 指定的 tokenizer 名称 (可选)
        format: 指定模型格式 (gguf/safetensors/bin/pt, 自动检测)
        use_full_conversion: 是否使用完整转换 (默认 True)
        force_install: 强制重新安装转换工具
    """
    try:
        # 查找 Ollama 模型文件
        logging.info(f"查找 Ollama 模型: {ollama_model_name}")
        model_info = find_ollama_model_files(ollama_model_name)

        # 如果指定了格式且与检测到的格式不一致，尝试转换
        if format and format != model_info["format"]:
            logging.warning(
                f"指定的格式 '{format}' 与检测到的格式 '{model_info['format']}' 不一致，"
                f"将尝试从模型目录中查找 {format} 文件"
            )
            # 尝试从 manifest 路径查找父目录并搜索指定格式
            if "manifest" in model_info and model_info["manifest"]:
                search_dir = model_info["manifest"].parent.parent.parent.parent
                try:
                    format_info = find_model_files_in_path(search_dir)
                    if format_info["format"] == format:
                        model_info = format_info
                except FileNotFoundError:
                    pass

        # 根据格式进行转换
        model_format = model_info["format"]
        model_files = model_info["files"]

        logging.info(f"模型格式: {model_format}")
        logging.info(f"模型文件: {[str(f) for f in model_files]}")

        if model_format == "gguf":
            convert_gguf_to_hf(
                model_files[0],
                output_path,
                hf_model_name=hf_model_name,
                tokenizer_name=tokenizer_name,
                use_full_conversion=use_full_conversion,
                force_install=force_install
            )
        elif model_format in ["safetensors", "bin", "pt"]:
            convert_safetensors_to_hf(
                model_files,
                output_path,
                hf_model_name=hf_model_name,
                tokenizer_name=tokenizer_name,
                use_full_conversion=use_full_conversion
            )
        else:
            raise ValueError(f"不支持的模型格式: {model_format}")

    except Exception as e:
        logging.error(f"转换失败: {e}")
        raise


def detect_model_format(model_file_path):
    """
    检测模型文件的格式

    Args:
        model_file_path: 模型文件路径

    Returns:
        模型格式字符串 (gguf/safetensors/bin/pt)
    """
    model_file_path = Path(model_file_path)

    # 首先检查文件扩展名
    suffix = model_file_path.suffix.lower()
    if suffix:
        if suffix == ".gguf":
            return "gguf"
        elif suffix == ".safetensors":
            return "safetensors"
        elif suffix == ".bin":
            return "bin"
        elif suffix == ".pt":
            return "pt"

    # 如果没有扩展名或扩展名不识别，检查文件名是否为 sha256 格式
    # sha256 格式: 64 个十六进制字符
    filename = model_file_path.name
    if SHA256_PATTERN.match(filename):
        # 可能是 Ollama 的 blob 文件，尝试读取文件头判断格式
        logging.info(f"文件名 '{filename}' 符合 sha256 格式，尝试检测文件类型...")

        try:
            with open(model_file_path, 'rb') as f:
                # 读取文件头
                header = f.read(4)

                # GGUF 文件头标识: 'GGUF' (0x4755 7546)
                if header == b'GGUF':
                    logging.info("检测为 GGUF 格式")
                    return "gguf"

                # SafeTensors 文件头标识: JSON 格式
                elif header.startswith(b'{'):
                    logging.info("检测为 SafeTensors 格式")
                    return "safetensors"

                # PyTorch bin 文件通常以特定的魔数开头
                # 尝试更多字节检测
                f.seek(0)
                header_8 = f.read(8)
                if header_8.startswith(b'\x80\x02') or header_8.startswith(b'PK\x03\x04'):
                    logging.info("检测为 PyTorch 格式")
                    return "bin"

                # 默认假设为 GGUF（Ollama blob 最常见）
                logging.warning("无法明确检测文件格式，假设为 GGUF 格式")
                return "gguf"

        except Exception as e:
            logging.warning(f"读取文件头失败: {e}，默认假设为 GGUF 格式")
            return "gguf"

    # 如果都不是，抛出错误
    raise ValueError(f"无法识别的模型文件格式: {filename}")


def convert_model_file(
    model_file_path,
    output_path,
    hf_model_name=None,
    tokenizer_name=None,
    use_full_conversion=True,
    force_install=False
):
    """
    直接转换指定的模型文件到 Hugging Face 格式

    Args:
        model_file_path: 模型文件路径 (GGUF/SafeTensors/BIN/PT/sha256 命名的文件)
        output_path: Hugging Face 格式模型输出路径
        hf_model_name: 对应的 Hugging Face 模型名称 (用于加载 tokenizer 和配置)
        tokenizer_name: 指定的 tokenizer 名称 (可选)
        use_full_conversion: 是否使用完整转换 (GGUF 模型默认启用)
        force_install: 强制重新安装转换工具
    """
    try:
        model_file_path = Path(model_file_path)

        if not model_file_path.exists():
            raise FileNotFoundError(f"模型文件不存在: {model_file_path}")

        # 检测模型格式
        model_format = detect_model_format(model_file_path)

        logging.info(f"检测到模型格式: {model_format}")
        logging.info(f"模型文件: {model_file_path}")

        # 根据格式进行转换
        if model_format == "gguf":
            convert_gguf_to_hf(
                model_file_path,
                output_path,
                hf_model_name=hf_model_name,
                tokenizer_name=tokenizer_name,
                use_full_conversion=use_full_conversion,
                force_install=force_install
            )
        elif model_format in ["safetensors", "bin", "pt"]:
            convert_safetensors_to_hf(
                [model_file_path],
                output_path,
                hf_model_name=hf_model_name,
                tokenizer_name=tokenizer_name
            )
        else:
            raise ValueError(f"不支持的模型格式: {model_format}")

    except Exception as e:
        logging.error(f"转换失败: {e}")
        raise


def main():
    parser = argparse.ArgumentParser(
        description="将本地 Ollama 模型或模型文件转换为 Hugging Face 格式"
    )

    # 创建互斥参数组：--ollama-model 和 --model-file
    source_group = parser.add_mutually_exclusive_group(required=True)
    source_group.add_argument(
        "--ollama-model",
        type=str,
        help="Ollama 模型名称 (如 'llama2', 'library/llama2:latest', 'mistral:7b', 'qwen:14b')"
    )
    source_group.add_argument(
        "--model-file",
        type=str,
        help="直接指定模型文件路径 (如 './models/qwen.gguf', './models/model.safetensors')"
    )

    parser.add_argument(
        "--output",
        type=str,
        required=True,
        help="输出目录路径 (Hugging Face 格式)"
    )
    parser.add_argument(
        "--hf-model",
        type=str,
        help="对应的 Hugging Face 模型名称 (用于加载 tokenizer 和配置)"
    )
    parser.add_argument(
        "--tokenizer",
        type=str,
        help="指定 tokenizer 名称 (覆盖 --hf-model)"
    )
    parser.add_argument(
        "--format",
        type=str,
        choices=["gguf", "safetensors", "bin", "pt"],
        help="指定模型格式 (默认自动检测，仅对 --ollama-model 有效)"
    )
    parser.add_argument(
        "--full-conversion",
        action="store_true",
        default=True,
        help="使用完整转换 (GGUF 模型默认启用)"
    )
    parser.add_argument(
        "--simple-conversion",
        action="store_true",
        help="使用简化转换 (仅保存 tokenizer 和配置，不转换权重)"
    )
    parser.add_argument(
        "--force-install",
        action="store_true",
        help="强制重新安装转换工具"
    )
    parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="显示详细日志"
    )

    args = parser.parse_args()

    # 确定转换模式
    use_full_conversion = args.full_conversion and not args.simple_conversion

    # 设置日志
    setup_logging(args.verbose)

    # 执行转换
    logging.info("=" * 60)
    logging.info("Ollama 模型转换工具")
    logging.info("=" * 60)

    try:
        if args.model_file:
            # 直接转换模型文件
            convert_model_file(
                args.model_file,
                args.output,
                hf_model_name=args.hf_model,
                tokenizer_name=args.tokenizer,
                use_full_conversion=use_full_conversion,
                force_install=args.force_install
            )
        else:
            # 通过 Ollama 查找并转换
            convert_ollama_model(
                ollama_model_name=args.ollama_model,
                output_path=args.output,
                hf_model_name=args.hf_model,
                tokenizer_name=args.tokenizer,
                format=args.format,
                use_full_conversion=use_full_conversion,
                force_install=args.force_install
            )

        logging.info("=" * 60)
        logging.info("转换完成！")
        logging.info(f"输出目录: {args.output}")
        logging.info("=" * 60)

    except Exception as e:
        logging.error(f"转换过程出错: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
