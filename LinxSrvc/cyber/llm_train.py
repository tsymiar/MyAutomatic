#!/usr/bin/env python3
# pip install torch torchvision transformers tensorboard tqdm pyyaml datasets
import os
import yaml
import logging
import argparse
from datetime import datetime
from tqdm import tqdm
import torch
from torch import nn, optim
from torch.utils.data import Dataset, DataLoader, DistributedSampler
from torch.utils.tensorboard import SummaryWriter
from torch.nn.parallel import DistributedDataParallel as DDP
from transformers import AutoTokenizer, AutoModelForCausalLM


class DeepSeekConfig:
    """DeepSeek模型配置类"""

    def __init__(self, config_dict):
        self.vocab_size = config_dict.get("vocab_size", 102400)
        self.hidden_size = config_dict.get("hidden_size", 4096)
        self.num_hidden_layers = config_dict.get("num_hidden_layers", 32)
        self.num_attention_heads = config_dict.get("num_attention_heads", 32)
        self.intermediate_size = config_dict.get("intermediate_size", 11008)
        self.max_position_embeddings = config_dict.get("max_position_embeddings", 4096)
        self.rope_theta = config_dict.get("rope_theta", 10000.0)
        self.tie_word_embeddings = config_dict.get("tie_word_embeddings", False)


class CustomDataset(Dataset):
    """支持CUDA加速的自定义数据集"""

    def __init__(self, data_path, tokenizer, max_length=4096, mode="train"):
        super().__init__()
        self.tokenizer = tokenizer
        self.max_length = max_length

        # 示例数据加载逻辑（需根据实际数据修改）
        with open(data_path, "r", encoding="utf-8") as f:
            self.texts = [line.strip() for line in f if line.strip()]

        # 使用CUDA加速的预处理（可选）
        if torch.cuda.is_available():
            self._cuda_preprocess()

    def _cuda_preprocess(self):
        """使用CUDA加速的预处理示例"""
        pass  # 在此添加自定义CUDA加速逻辑

    def __len__(self):
        return len(self.texts)

    def __getitem__(self, idx):
        text = self.texts[idx]
        encoding = self.tokenizer(
            text,
            max_length=self.max_length,
            padding="max_length",
            truncation=True,
            return_tensors="pt",
        )
        return {
            "input_ids": encoding["input_ids"].squeeze(),
            "attention_mask": encoding["attention_mask"].squeeze(),
            "labels": encoding["input_ids"].squeeze(),
        }


class Trainer:
    """支持DeepSeek的训练器"""

    def __init__(self, config, device, offload_device=torch.device("cuda:0")):
        self.config = config
        self.device = device
        self.offload_device = offload_device
        self._setup_infrastructure()
        self._initialize_components()

    def _setup_infrastructure(self):
        """创建目录和日志系统"""
        os.makedirs(self.config["training"]["out_dir"], exist_ok=True)
        os.makedirs(self.config["logging"]["log_dir"], exist_ok=True)

        # TensorBoard日志
        self.writer = SummaryWriter(self.config["logging"]["log_dir"])

        # 文件日志
        logging.basicConfig(
            filename=os.path.join(self.config["training"]["out_dir"], "training.log"),
            level=logging.INFO,
            format="%(asctime)s - %(levelname)s - %(message)s",
        )

    def _initialize_components(self):
        """初始化各组件"""
        # 分布式训练
        self.distributed = self.config["training"].get("distributed", False)
        if self.distributed:
            torch.distributed.init_process_group(backend="nccl")
            self.local_rank = int(os.environ["LOCAL_RANK"])
            self.device = f"cuda:{self.local_rank}"
            torch.cuda.set_device(self.local_rank)

        # 初始化DeepSeek组件
        if self.config["model"].get("use_deepseek", False):
            self._init_deepseek()
        else:
            self.tokenizer = AutoTokenizer.from_pretrained(
                self.config["model"]["tokenizer_path"]
            )
            self.model = AutoModelForCausalLM.from_pretrained(
                self.config["model"]["model_path"]
            ).to(self.device)

        # 数据加载
        self._init_dataloaders()

        # 优化器
        self.optimizer = optim.AdamW(
            self.model.parameters(),
            lr=self.config["training"]["learning_rate"],
            weight_decay=self.config["training"]["weight_decay"],
        )

        # 混合精度训练
        self.scaler = torch.cuda.amp.GradScaler(enabled=self.config["training"]["fp16"])

    def _init_deepseek(self):
        """初始化DeepSeek模型"""
        try:
            from deepseek.models import DeepseekLMHeadModel  # 假设存在DeepSeek官方库
        except ImportError:
            logging.error(
                "Deepseek module not found. Please install it or check the module path."
            )
            exit(1)

        # 加载配置
        model_config = DeepSeekConfig(self.config["model"]["params"])

        # 初始化模型
        self.model = DeepseekLMHeadModel(model_config).to(self.device)
        self.tokenizer = AutoTokenizer.from_pretrained(
            self.config["model"]["tokenizer_path"], use_fast=True
        )

        # 模型并行配置
        if self.config["model"].get("tensor_parallel", False):
            self.model.parallelize(self.config["model"]["tensor_parallel_degree"])

    def _init_dataloaders(self):
        """初始化数据加载器"""
        # 训练集
        train_dataset = CustomDataset(
            self.config["data"]["train_path"],
            self.tokenizer,
            max_length=self.config["data"]["max_seq_length"],
            mode="train",
        )

        # 验证集
        val_dataset = CustomDataset(
            self.config["data"]["val_path"],
            self.tokenizer,
            max_length=self.config["data"]["max_seq_length"],
            mode="val",
        )

        # 分布式采样器
        sampler = DistributedSampler(train_dataset) if self.distributed else None

        self.train_loader = DataLoader(
            train_dataset,
            batch_size=self.config["training"]["batch_size"],
            sampler=sampler,
            shuffle=(sampler is None),
            num_workers=self.config["data"]["num_workers"],
            pin_memory=True,
            persistent_workers=True,
        )

        self.val_loader = DataLoader(
            val_dataset,
            batch_size=self.config["training"]["val_batch_size"],
            num_workers=self.config["data"]["num_workers"],
            pin_memory=True,
        )

    def train(self):
        """主训练循环"""
        for epoch in range(1, self.config["training"]["epochs"] + 1):
            self.model.train()
            total_loss = 0

            progress_bar = tqdm(self.train_loader, desc=f"Epoch {epoch}")
            for batch in progress_bar:
                # 数据转移到CUDA设备
                inputs = {k: v.to(self.device) for k, v in batch.items()}

                # 混合精度训练
                with torch.cuda.amp.autocast(enabled=self.config["training"]["fp16"]):
                    outputs = self.model(**inputs)
                    loss = outputs.loss

                # 反向传播
                self.scaler.scale(loss).backward()

                # 梯度裁剪
                if self.config["training"]["grad_clip"] > 0:
                    self.scaler.unscale_(self.optimizer)
                    nn.utils.clip_grad_norm_(
                        self.model.parameters(), self.config["training"]["grad_clip"]
                    )

                # 参数更新
                self.scaler.step(self.optimizer)
                self.scaler.update()
                self.optimizer.zero_grad()

                # 记录日志
                total_loss += loss.item()
                progress_bar.set_postfix(loss=loss.item())

            # 验证与保存
            avg_loss = total_loss / len(self.train_loader)
            val_loss = self.validate()

            # 保存检查点
            if epoch % self.config["training"]["save_interval"] == 0:
                self._save_checkpoint(epoch)

            # 记录TensorBoard
            self.writer.add_scalar("Loss/train", avg_loss, epoch)
            self.writer.add_scalar("Loss/val", val_loss, epoch)

    def validate(self):
        """验证循环"""
        self.model.eval()
        total_loss = 0

        with torch.no_grad():
            for batch in tqdm(self.val_loader, desc="Validating"):
                inputs = {k: v.to(self.device) for k, v in batch.items()}
                outputs = self.model(**inputs)
                total_loss += outputs.loss.item()

        return total_loss / len(self.val_loader)

    def _save_checkpoint(self, epoch):
        """保存模型检查点"""
        checkpoint = {
            "epoch": epoch,
            "model_state_dict": self.model.state_dict(),
            "optimizer_state_dict": self.optimizer.state_dict(),
            "config": self.config,
        }

        filename = os.path.join(
            self.config["training"]["out_dir"], f"checkpoint_epoch{epoch}.pt"
        )
        torch.save(checkpoint, filename)
        logging.info(f"Checkpoint saved at epoch {epoch}")


def load_config(config_path):
    """加载配置文件"""
    with open(config_path, "r") as f:
        return yaml.safe_load(f)


def parse_args():
    """解析命令行参数"""
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--config",
        type=str,
        required=False,
        default="params.yaml",
        help="配置文件路径",
    )
    parser.add_argument("--deepseek", action="store_true", help="启用DeepSeek训练模式")
    parser.add_argument("--cuda", action="store_true", help="强制使用CUDA加速")
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()

    # 加载配置
    config = load_config(args.config)

    # 处理DeepSeek开关
    if args.deepseek:
        config["model"]["use_deepseek"] = True

    # 设备设置
    device = "cuda" if torch.cuda.is_available() and args.cuda else "cpu"

    # 初始化训练器
    trainer = Trainer(config, device)

    # 开始训练
    try:
        trainer.train()
    except KeyboardInterrupt:
        logging.info("Training interrupted, saving final checkpoint...")
        trainer._save_checkpoint("interrupted")
