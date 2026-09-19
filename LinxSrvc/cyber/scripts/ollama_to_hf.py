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

# 系统敏感目录列表 — 禁止 subprocess 传入这些路径
_FORBIDDEN_PATH_PREFIXES = [
    "/etc", "/boot", "/sys", "/proc", "/dev",
]

# 允许执行的程序白名单 — subprocess 仅用于这些可信程序, 且始终 shell=False
_ALLOWED_BINARIES = {
    "git",
    "ollama",
    "python",
    "python3",
}


def _validate_safe_path(target):
    """校验路径不指向系统敏感目录，防止意外操作"""
    resolved = Path(target).resolve()
    resolved_str = str(resolved)
    for prefix in _FORBIDDEN_PATH_PREFIXES:
        if resolved_str == prefix or resolved_str.startswith(prefix + os.sep):
            raise ValueError(f"拒绝访问系统敏感路径: {target}")


def setup_logging(verbose=False):
    """设置日志"""
    level = logging.DEBUG if verbose else logging.INFO
    logging.basicConfig(
        level=level,
        format='%(asctime)s - %(levelname)s - %(message)s'
    )


def run_command(cmd, cwd=None, capture_output=True):
    """
    运行 shell 命令（仅接受可信输入）

    安全约束:
    - cmd 必须为非空 list[str]，禁止传入用户可控字符串拼接的命令
    - 使用 list 形式确保 shell=False，防止命令注入
    - cwd 若提供则必须为已存在的目录

    Args:
        cmd: 命令列表 (断言为非空 list of str)
        cwd: 工作目录 (可选)
        capture_output: 是否捕获输出

    Returns:
        (returncode, stdout, stderr)

    Raises:
        ValueError: cmd 无效或 cwd 不安全
    """
    # 输入校验: cmd 必须为非空字符串列表
    if not isinstance(cmd, list) or len(cmd) == 0:
        raise ValueError("cmd 必须为非空的字符串列表")
    if not all(isinstance(arg, str) for arg in cmd):
        raise ValueError("cmd 中所有元素必须为字符串")

    # 可执行文件白名单校验: 拒绝白名单之外的程序 (含用户可控输入)
    if os.path.basename(cmd[0]) not in _ALLOWED_BINARIES:
        raise ValueError(f"不允许执行的程序: {cmd[0]}")

    # cwd 校验: 若提供则必须存在且为目录
    if cwd is not None:
        cwd_path = Path(cwd)
        if not cwd_path.is_dir():
            raise ValueError(f"cwd 不是有效目录: {cwd}")
        _validate_safe_path(cwd_path)

    logging.debug(f"执行命令: {' '.join(cmd)}")

    result = subprocess.run(
        cmd,
        cwd=cwd,
        capture_output=capture_output,
        text=True,
        check=False,
        shell=False
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
    # 使用脚本所在目录作为基准，避免 CWD 可控时指向恶意路径
    script_dir = Path(__file__).resolve().parent
    tools_dir = script_dir / ".llama-tools"
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
        returncode, _, stderr = run_command(
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


def _locate_ollama_root():
    """定位本地 Ollama 的模型存储根目录"""
    home = Path.home()

    ollama_roots = [
        home / ".ollama" / "models",
        home / ".cache" / "ollama" / "models",
        Path("/usr/share/ollama/models"),
    ]

    if sys.platform == "darwin":
        ollama_roots.append(home / "Library" / "Application Support" / "ollama" / "models")

    for root_dir in ollama_roots:
        if root_dir.exists():
            logging.info(f"找到 Ollama 模型根目录: {root_dir}")
            return root_dir

    error_msg = "找不到 Ollama 模型存储目录。已尝试的位置:\n"
    error_msg += "\n".join(f"  - {root}" for root in ollama_roots)
    raise FileNotFoundError(error_msg)


def _parse_model_name(ollama_model_name):
    """把 library/model:tag 或 org/model:tag 解析为 (model, tag)"""
    model_parts = ollama_model_name.split("/")

    if len(model_parts) == 1:
        model_with_tag = model_parts[0]
    elif len(model_parts) == 2:
        model_with_tag = model_parts[1]
    else:
        raise ValueError(f"无法解析模型名称: {ollama_model_name}")

    if ":" in model_with_tag:
        model, tag = model_with_tag.split(":")
    else:
        model, tag = model_with_tag, "latest"

    logging.info(f"解析模型名称 -> 模型: {model}, 标签: {tag}")
    return model, tag


def _locate_manifest(manifest_dir, model, tag):
    """按新/旧两种路径格式查找 manifest，返回 (manifest_path, 已尝试路径列表)"""
    possible_manifest_paths = [
        manifest_dir / "registry.ollama.ai" / "library" / model / tag,
        manifest_dir / "library" / model / tag,
    ]

    for path in possible_manifest_paths:
        if path.exists():
            logging.info(f"找到 manifest 文件: {path}")
            return path, possible_manifest_paths

    return None, possible_manifest_paths


def _lookup_model_via_ollama_list(ollama_model_name):
    """manifest 缺失时，回退到 `ollama list` 输出里匹配的完整模型名"""
    logging.info("未找到 manifest 文件，尝试通过 ollama 命令查找...")
    try:
        returncode, stdout, _ = run_command(["ollama", "list"], capture_output=True)
        if returncode != 0 or not stdout:
            return None
        for line in stdout.split('\n'):
            if not line.strip() or line.startswith("NAME"):
                continue
            parts = line.split()
            if parts and ollama_model_name in parts[0]:
                logging.info(f"找到模型: {parts[0]}")
                return parts[0]
    except Exception as e:
        logging.debug(f"ollama list 命令失败: {e}")
    return None


def _detect_file_format(path):
    """按扩展名或文件头判断模型格式，检测失败时回退为 gguf"""
    if path.suffix:
        return path.suffix[1:]
    try:
        return detect_model_format(path)
    except Exception:
        return "gguf"


def find_ollama_model_files(ollama_model_name):
    """
    查找 Ollama 模型在本地存储的模型文件

    Args:
        ollama_model_name: Ollama 模型名称 (如 "llama2", "mistral:7b", "qwen:latest")

    Returns:
        字典，包含模型文件路径和格式信息
    """
    ollama_root = _locate_ollama_root()

    manifest_dir = ollama_root / "manifests"
    if not manifest_dir.exists():
        raise FileNotFoundError(f"Ollama manifest 目录不存在: {manifest_dir}")

    model, tag = _parse_model_name(ollama_model_name)
    manifest_path, possible_manifest_paths = _locate_manifest(manifest_dir, model, tag)

    if manifest_path is None:
        full_name = _lookup_model_via_ollama_list(ollama_model_name)
        if full_name:
            # 用 `ollama list` 返回的完整模型名重新解析
            return find_ollama_model_files(full_name)

        error_msg = f"找不到 Ollama 模型 '{ollama_model_name}'。\n"
        error_msg += f"请确保已使用 'ollama pull {ollama_model_name}' 下载模型。\n"
        error_msg += "已尝试的 manifest 路径:\n"
        error_msg += "\n".join(f"  - {p}" for p in possible_manifest_paths)
        raise FileNotFoundError(error_msg)

    result = _scan_manifest_layers(manifest_path, ollama_root)
    if result is not None:
        return result

    return _scan_blobs_directory(ollama_root)


def _scan_manifest_layers(manifest_path, ollama_root):
    """读取 manifest 并定位第一个真实存在的 blob 文件"""
    try:
        with open(manifest_path, 'r') as f:
            manifest = json.load(f)
    except Exception as e:
        logging.error(f"读取 manifest 失败: {e}")
        import traceback
        logging.debug(traceback.format_exc())
        return None

    logging.info(f"Manifest 内容结构: {list(manifest.keys())}")
    logging.debug(f"完整 manifest 内容: {manifest}")

    if "layers" not in manifest:
        logging.warning("Manifest 文件中未找到 'layers' 字段")
        logging.info(f"Manifest 键: {list(manifest.keys())}")
        if "config" in manifest:
            logging.info("找到 config 字段")
        if "mediaType" in manifest:
            logging.info(f"Media type: {manifest['mediaType']}")
        return None

    layers = manifest["layers"]
    logging.info(f"Manifest 中有 {len(layers)} 个 layers")
    for idx, layer in enumerate(layers):
        digest_full = layer.get("digest", "")
        if not digest_full.startswith("sha256:"):
            logging.debug(f"Layer {idx} 没有 sha256 digest: {digest_full[:20]}")
            continue

        digest_lower = digest_full.split(":")[1].lower()
        # 新格式: blobs/sha256/{digest[:2]}/{digest}；旧格式: blobs/sha256-{digest}
        possible_blob_paths = [
            ollama_root / "blobs" / "sha256" / digest_lower[:2] / digest_lower,
            ollama_root / "blobs" / f"sha256-{digest_lower}",
        ]

        for blob_path in possible_blob_paths:
            logging.info(f"检查 blob [{idx}]: {blob_path}")
            if not blob_path.exists():
                logging.debug(f"Blob 文件不存在: {blob_path}")
                continue
            logging.info(f"找到模型文件: {blob_path}")
            return {
                "format": _detect_file_format(blob_path),
                "files": [blob_path],
                "manifest": manifest_path,
            }

    return None


def _collect_blob_files(ollama_root):
    """在 blobs 目录中收集所有可能是模型权重的文件"""
    all_model_files = []

    # 新格式: blobs/sha256/{digest[:2]}/{digest}；旧格式: blobs/sha256-{digest}
    for search_dir in (ollama_root / "blobs" / "sha256", ollama_root / "blobs"):
        if not search_dir.exists():
            continue

        if search_dir.name == "sha256":
            for subdir in search_dir.iterdir():
                if not subdir.is_dir():
                    continue
                found = []
                for pattern in ("*.gguf", "*.safetensors", "*.bin", "*.pt"):
                    found.extend(subdir.glob(pattern))
                found.extend(
                    f for f in subdir.iterdir()
                    if f.is_file() and not f.suffix and SHA256_PATTERN.match(f.name.lower())
                )
                all_model_files.extend(found)
        else:
            for blob_file in search_dir.iterdir():
                if not blob_file.is_file() or not blob_file.name.startswith("sha256-"):
                    continue
                # 移除 "sha256-" 前缀后再校验是否为合法 sha256
                if SHA256_PATTERN.match(blob_file.name[7:].lower()):
                    all_model_files.append(blob_file)

    return all_model_files


def _scan_blobs_directory(ollama_root):
    """manifest 无法定位 blob 时，退化为在 blobs 目录中取最新的模型文件"""
    logging.info("在 blobs 目录中搜索模型文件...")

    all_model_files = _collect_blob_files(ollama_root)
    if not all_model_files:
        raise FileNotFoundError(
            f"在 {ollama_root} 中找不到模型文件，已找到 manifest 但无法定位 blob 文件"
        )

    # 按修改时间排序，取最新的
    all_model_files.sort(key=lambda x: x.stat().st_mtime, reverse=True)
    latest_file = all_model_files[0]
    file_format = _detect_file_format(latest_file)

    logging.info(f"找到 {len(all_model_files)} 个模型文件，使用最新的: {latest_file}")
    logging.info(f"检测到文件格式: {file_format}")
    return {"format": file_format, "files": [latest_file], "manifest": None}


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

        # 路径安全性校验: 拒绝系统敏感路径
        gguf_path = Path(gguf_path).resolve()
        output_path = Path(output_path).resolve()
        _validate_safe_path(gguf_path)
        _validate_safe_path(output_path)

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
        returncode, _, stderr = run_command(
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
    model_format=None,
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
        model_format: 指定模型格式 (gguf/safetensors/bin/pt, 自动检测)
        use_full_conversion: 是否使用完整转换 (默认 True)
        force_install: 强制重新安装转换工具
    """
    try:
        # 查找 Ollama 模型文件
        logging.info(f"查找 Ollama 模型: {ollama_model_name}")
        model_info = find_ollama_model_files(ollama_model_name)

        # 如果指定了格式且与检测到的格式不一致，尝试转换
        if model_format and model_info["format"] != model_format:
            logging.warning(
                f"指定的格式 '{model_format}' 与检测到的格式 '{model_info['format']}' 不一致，"
                f"将尝试从模型目录中查找 {model_format} 文件"
            )
            # 尝试从 manifest 路径查找父目录并搜索指定格式
            if "manifest" in model_info and model_info["manifest"]:
                search_dir = model_info["manifest"].parent.parent.parent.parent
                try:
                    format_info = find_model_files_in_path(search_dir)
                    if format_info["format"] == model_format:
                        model_info = format_info
                except FileNotFoundError:
                    pass

        # 根据格式进行转换
        detected_format = model_info["format"]
        model_files = model_info["files"]

        logging.info(f"模型格式: {detected_format}")
        logging.info(f"模型文件: {[str(f) for f in model_files]}")

        if detected_format == "gguf":
            convert_gguf_to_hf(
                model_files[0],
                output_path,
                hf_model_name=hf_model_name,
                tokenizer_name=tokenizer_name,
                use_full_conversion=use_full_conversion,
                force_install=force_install
            )
        elif detected_format in ["safetensors", "bin", "pt"]:
            convert_safetensors_to_hf(
                model_files,
                output_path,
                hf_model_name=hf_model_name,
                tokenizer_name=tokenizer_name,
                use_full_conversion=use_full_conversion
            )
        else:
            raise ValueError(f"不支持的模型格式: {detected_format}")

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
                model_format=args.format,
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
