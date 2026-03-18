#!/usr/bin/env python3
"""
test.py - 测试脚本
已集成 Ollama 转换测试
"""
import os
import sys
import logging
from pathlib import Path

# 获取 cyber 目录的绝对路径
cyber_dir = Path(__file__).parent.absolute()

# 确保 cyber 目录在 Python 路径中
if str(cyber_dir) not in sys.path:
    sys.path.insert(0, str(cyber_dir))

try:
    from scripts.ollama_to_hf import (
        setup_logging,
        run_command,
        install_llama_cpp_converter,
        find_ollama_model_files,
        find_model_files_in_path,
        convert_ollama_model
    )
except ImportError as e:
    print(f"错误: 无法导入 ollama_to_hf 模块: {e}")
    print(f"当前 Python 路径: {sys.path}")
    print(f"cyber 目录: {cyber_dir}")
    print(f"scripts 目录是否存在: {(cyber_dir / 'scripts').exists()}")
    sys.exit(1)


def test_logging():
    """测试日志设置"""
    print("测试 1: 日志设置")
    setup_logging(verbose=True)
    logging.info("日志测试 - INFO")
    logging.debug("日志测试 - DEBUG")
    print("✓ 日志设置测试通过\n")
    return True


def test_run_command():
    """测试命令执行"""
    print("测试 2: 命令执行")
    returncode, stdout, stderr = run_command(["echo", "test"])
    assert returncode == 0, f"命令执行失败，返回码: {returncode}"
    assert "test" in stdout, f"输出不符合预期: {stdout}"
    print("✓ 命令执行测试通过\n")
    return True


def test_install_converter():
    """测试转换工具安装检查"""
    print("测试 3: 转换工具安装检查")
    try:
        converter_dir = install_llama_cpp_converter()
        if converter_dir:
            print(f"✓ 转换工具已安装在: {converter_dir}\n")
            return True
        else:
            print("⚠ 转换工具安装失败或 Git 不可用\n")
            return False
    except Exception as e:
        print(f"⚠ 转换工具安装检查跳过: {e}\n")
        return False


def test_model_directory_search():
    """测试模型目录搜索"""
    print("测试 4: 模型目录搜索")
    try:
        # 尝试搜索一个常见的模型名称
        model_info = find_ollama_model_files("llama3.2:latest")
        print(f"✓ 找到模型: {model_info}\n")
        return True
    except FileNotFoundError as e:
        print(f"⚠ 未找到模型 (预期行为): {e}\n")
        return False  # 这是预期的，不算失败
    except Exception as e:
        print(f"⚠ 模型搜索出错: {e}\n")
        return False


def test_model_files_detection():
    """测试模型文件格式检测"""
    print("测试 5: 模型文件格式检测")
    try:
        # 测试一些已知目录
        test_dirs = []

        # 尝试查找 cyber 目录中的数据目录
        if cyber_dir.exists():
            for subdir in cyber_dir.iterdir():
                if subdir.is_dir() and not subdir.name.startswith('.'):
                    try:
                        model_info = find_model_files_in_path(subdir)
                        if model_info and model_info.get("files"):
                            test_dirs.append((subdir.name, model_info))
                    except FileNotFoundError:
                        pass

        if test_dirs:
            for dir_name, model_info in test_dirs:
                print(f"  {dir_name}: {model_info['format']} 格式, {len(model_info['files'])} 个文件")
            print("✓ 模型文件格式检测测试通过\n")
            return True
        else:
            print("⚠ 未找到任何模型文件\n")
            return False  # 这是预期的
    except Exception as e:
        print(f"✗ 模型文件格式检测测试失败: {e}\n")
        return False


def test_help_command():
    """测试帮助命令"""
    print("测试 6: 帮助命令")
    try:
        returncode, stdout, stderr = run_command(
            [sys.executable, str(cyber_dir / "scripts" / "ollama_to_hf.py"), "--help"]
        )
        if returncode == 0:
            print("✓ 帮助命令测试通过\n")
            return True
        else:
            print(f"✗ 帮助命令测试失败: {stderr}\n")
            return False
    except Exception as e:
        print(f"✗ 帮助命令测试跳过: {e}\n")
        return False


def run_all_tests():
    """运行所有测试"""
    print("=" * 60)
    print("LinxSrvc Cyber 模块测试")
    print("=" * 60)
    print()

    tests = {
        "日志设置": test_logging,
        "命令执行": test_run_command,
        "转换工具安装": test_install_converter,
        "模型目录搜索": test_model_directory_search,
        "模型文件格式检测": test_model_files_detection,
        "帮助命令": test_help_command,
    }

    results = {}
    for test_name, test_func in tests.items():
        try:
            results[test_name] = test_func()
        except Exception as e:
            print(f"✗ {test_name}测试出错: {e}\n")
            results[test_name] = False

    # 打印测试结果摘要
    print("=" * 60)
    print("测试结果摘要")
    print("=" * 60)
    print()

    passed = sum(1 for v in results.values() if v is True)
    failed = sum(1 for v in results.values() if v is False)
    skipped = sum(1 for v in results.values() if v is None)

    for test_name, result in results.items():
        if result is True:
            print(f"✓ {test_name}")
        elif result is False:
            print(f"✗ {test_name}")
        else:
            print(f"⚠ {test_name} (跳过)")

    print()
    print(f"总计: {len(results)} 个测试")
    print(f"通过: {passed} 个")
    print(f"失败: {failed} 个")
    print(f"跳过: {skipped} 个")
    print()
    
    if failed == 0:
        print("🎉 所有测试通过！")
        print("=" * 60)
        return True
    else:
        print(f"⚠️ {failed} 个测试失败，请检查错误信息")
        print("=" * 60)
        return False


if __name__ == "__main__":
    try:
        success = run_all_tests()
        sys.exit(0 if success else 1)
    except KeyboardInterrupt:
        print("\n\n测试被用户中断")
        sys.exit(130)
    except Exception as e:
        print(f"\n\n测试过程出错: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
