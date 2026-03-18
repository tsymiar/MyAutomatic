#!/usr/bin/env python3
# scripts/train.py
import os
import sys
import yaml
import logging
import argparse
from tqdm import tqdm
import torch
import torch.nn as nn
from torch.utils.data import DataLoader, DistributedSampler
from torch.utils.tensorboard import SummaryWriter
from transformers import get_linear_schedule_with_warmup
from peft import LoraConfig, get_peft_model, prepare_model_for_kbit_training

# 添加项目根目录到路径
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from data.dataset import ConversationDataset
from scripts.utils.model_utils import load_base_model, load_tokenizer

def load_config(config_path):
    """加载并验证配置文件"""
    if not os.path.exists(config_path):
        raise FileNotFoundError(f"配置文件不存在: {config_path}")

    with open(config_path, 'r', encoding='utf-8') as f:
        config = yaml.safe_load(f)

    # 验证必需的配置字段
    required_sections = ['model', 'training', 'data', 'logging', 'scheduler', 'lora']
    for section in required_sections:
        if section not in config:
            raise ValueError(f"配置文件缺少必需的节: {section}")

    # 验证 training 节的必需字段
    required_training_keys = ['out_dir', 'batch_size', 'epochs', 'lr', 'weight_decay', 'grad_clip', 'early_stop_patience', 'save_interval']
    for key in required_training_keys:
        if key not in config['training']:
            raise ValueError(f"配置文件 training 节缺少必需字段: {key}")

    # 添加默认值
    config['training']['gradient_accumulation_steps'] = config['training'].get('gradient_accumulation_steps', 1)
    config['training']['val_batch_size'] = config['training'].get('val_batch_size', config['training']['batch_size'])
    config['data']['num_workers'] = config['data'].get('num_workers', 4)

    return config

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--config', type=str, default='params/training_config.yaml')
    parser.add_argument('--local_rank', type=int, default=-1, help='分布式训练本地进程号')
    return parser.parse_args()

class Trainer:
    def __init__(self, config, args):
        self.config = config
        self.args = args
        self.device = self._setup_device()
        self._setup_logging()
        self.writer = None  # 先初始化为 None
        try:
            self._load_model_and_tokenizer()
            self._apply_lora()
            self._setup_dataloaders()
            self._setup_optimizer_scheduler()
            self._setup_mixed_precision()
        except Exception as e:
            logging.error(f"初始化失败: {e}")
            self.cleanup()
            raise

    def cleanup(self):
        """清理资源"""
        if hasattr(self, 'writer') and self.writer is not None:
            self.writer.close()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.cleanup()
        return False

    def _setup_device(self):
        """设置训练设备"""
        if torch.cuda.is_available():
            if self.args.local_rank != -1:
                torch.distributed.init_process_group(backend='nccl')
                torch.cuda.set_device(self.args.local_rank)
                device = torch.device(f'cuda:{self.args.local_rank}')
                if self.args.local_rank == 0:
                    logging.info(f"分布式训练: {torch.distributed.get_world_size()} 个GPU")
                return device
            else:
                logging.info(f"使用CUDA: {torch.cuda.get_device_name(0)}")
                return torch.device('cuda')
        else:
            logging.warning("未检测到CUDA，使用CPU训练（速度较慢）")
            return torch.device('cpu')

    def _setup_logging(self):
        """设置日志和 TensorBoard"""
        self.out_dir = os.path.abspath(self.config['training']['out_dir'])
        self.log_dir = os.path.abspath(self.config['logging']['log_dir'])
        os.makedirs(self.out_dir, exist_ok=True)
        os.makedirs(self.log_dir, exist_ok=True)

        log_file = os.path.join(self.out_dir, 'training.log')
        logging.basicConfig(
            filename=log_file,
            level=logging.INFO,
            format='%(asctime)s - %(levelname)s - %(message)s',
            filemode='w'  # 每次训练创建新日志
        )
        self.writer = SummaryWriter(self.log_dir)
        logging.info(f"日志文件: {log_file}")
        logging.info(f"TensorBoard日志: {self.log_dir}")

    def _load_model_and_tokenizer(self):
        """加载模型和 tokenizer"""
        try:
            model_cfg = self.config['model']
            model_path = model_cfg.get('local_model_path') or model_cfg['model_name']

            logging.info(f"加载 tokenizer: {model_path}")
            self.tokenizer = load_tokenizer(model_path)

            logging.info(f"加载模型: {model_path}")
            self.model = load_base_model(
                model_path,
                load_in_4bit=model_cfg.get('load_in_4bit', False),
                torch_dtype=torch.bfloat16 if self.config['training'].get('bf16') else torch.float16
            )
            logging.info("模型和 tokenizer 加载成功")
        except Exception as e:
            logging.error(f"模型加载失败: {e}")
            raise

    def _apply_lora(self):
        if self.config['training'].get('use_lora', False):
            lora_cfg = self.config['lora']
            lora_config = LoraConfig(
                r=lora_cfg['r'],
                lora_alpha=lora_cfg['lora_alpha'],
                target_modules=lora_cfg['target_modules'],
                lora_dropout=lora_cfg['lora_dropout'],
                bias="none",
                task_type="CAUSAL_LM",
            )
            if self.config['model'].get('load_in_4bit', False):
                self.model = prepare_model_for_kbit_training(self.model)
            self.model = get_peft_model(self.model, lora_config)
            self.model.print_trainable_parameters()
            logging.info("LoRA applied.")

    def _setup_dataloaders(self):
        """设置训练和验证数据加载器"""
        try:
            data_cfg = self.config['data']

            # 验证数据文件存在性
            if not os.path.exists(data_cfg['train_path']):
                raise FileNotFoundError(f"训练数据文件不存在: {data_cfg['train_path']}")
            if not os.path.exists(data_cfg['val_path']):
                raise FileNotFoundError(f"验证数据文件不存在: {data_cfg['val_path']}")

            train_dataset = ConversationDataset(
                data_cfg['train_path'],
                self.tokenizer,
                max_length=data_cfg['max_seq_length'],
                data_format=data_cfg.get('data_format', 'raw'),
                sharegpt_config=data_cfg.get('sharegpt', {})
            )
            val_dataset = ConversationDataset(
                data_cfg['val_path'],
                self.tokenizer,
                max_length=data_cfg['max_seq_length'],
                data_format=data_cfg.get('data_format', 'raw'),
                sharegpt_config=data_cfg.get('sharegpt', {})
            )

            train_sampler = DistributedSampler(train_dataset) if self.args.local_rank != -1 else None

            self.train_loader = DataLoader(
                train_dataset,
                batch_size=self.config['training']['batch_size'],
                sampler=train_sampler,
                shuffle=(train_sampler is None),
                num_workers=data_cfg['num_workers'],
                pin_memory=True,
                drop_last=True  # 确保梯度累积时批次大小一致
            )
            self.val_loader = DataLoader(
                val_dataset,
                batch_size=self.config['training']['val_batch_size'],
                shuffle=False,
                num_workers=data_cfg['num_workers'],
                pin_memory=True,
            )

            self.train_sampler = train_sampler  # 保存引用以便后续使用
            logging.info(f"训练数据: {len(train_dataset)} 样本")
            logging.info(f"验证数据: {len(val_dataset)} 样本")
        except Exception as e:
            logging.error(f"数据加载器设置失败: {e}")
            raise

    def _setup_optimizer_scheduler(self):
        lr = float(self.config['training']['lr'])
        weight_decay = self.config['training']['weight_decay']
        self.optimizer = torch.optim.AdamW(self.model.parameters(), lr=lr, weight_decay=weight_decay)

        total_steps = len(self.train_loader) * self.config['training']['epochs']
        warmup_steps = self.config['scheduler']['warmup_steps']
        self.scheduler = get_linear_schedule_with_warmup(
            self.optimizer,
            num_warmup_steps=warmup_steps,
            num_training_steps=total_steps
        )

    def _setup_mixed_precision(self):
        self.use_fp16 = self.config['training'].get('fp16', False)
        self.scaler = torch.cuda.amp.GradScaler(enabled=self.use_fp16) if torch.cuda.is_available() else None

    def train(self):
        """训练循环"""
        best_val_loss = float('inf')
        patience_counter = 0
        gradient_accumulation_steps = self.config['training']['gradient_accumulation_steps']

        logging.info(f"开始训练，共 {self.config['training']['epochs']} 个 epoch")
        logging.info(f"梯度累积步数: {gradient_accumulation_steps}")

        for epoch in range(1, self.config['training']['epochs'] + 1):
            # 设置 sampler epoch，确保分布式训练时数据 shuffle
            if self.train_sampler is not None:
                self.train_sampler.set_epoch(epoch)

            self.model.train()
            total_loss = 0
            self.optimizer.zero_grad()

            progress_bar = tqdm(self.train_loader, desc=f'Epoch {epoch}', disable=(self.args.local_rank != 0))

            for step, batch in enumerate(progress_bar):
                input_ids = batch['input_ids'].to(self.device)
                attention_mask = batch['attention_mask'].to(self.device)
                labels = batch['labels'].to(self.device)

                # 混合精度前向传播
                autocast_device = 'cuda' if self.device.type == 'cuda' else 'cpu'
                with torch.amp.autocast(device_type=autocast_device, enabled=self.use_fp16):
                    outputs = self.model(
                        input_ids=input_ids,
                        attention_mask=attention_mask,
                        labels=labels
                    )
                    loss = outputs.loss / gradient_accumulation_steps  # 归一化损失

                # 反向传播
                if self.use_fp16 and self.scaler:
                    self.scaler.scale(loss).backward()
                else:
                    loss.backward()

                # 梯度累积：达到累积步数后更新参数
                if (step + 1) % gradient_accumulation_steps == 0:
                    # 梯度裁剪
                    if self.config['training']['grad_clip'] > 0:
                        if self.use_fp16 and self.scaler:
                            self.scaler.unscale_(self.optimizer)
                        nn.utils.clip_grad_norm_(self.model.parameters(), self.config['training']['grad_clip'])

                    # 更新参数
                    if self.use_fp16 and self.scaler:
                        self.scaler.step(self.optimizer)
                        self.scaler.update()
                    else:
                        self.optimizer.step()

                    self.optimizer.zero_grad()
                    self.scheduler.step()

                total_loss += loss.item() * gradient_accumulation_steps
                progress_bar.set_postfix(loss=loss.item() * gradient_accumulation_steps)

                # 记录学习率
                if self.args.local_rank in [0, -1] and step % 100 == 0:
                    current_lr = self.optimizer.param_groups[0]['lr']
                    self.writer.add_scalar('Learning_Rate', current_lr, epoch * len(self.train_loader) + step)

            avg_loss = total_loss / len(self.train_loader)

            # 验证
            val_loss = self.validate()

            # 只在主进程记录日志
            if self.args.local_rank in [0, -1]:
                logging.info(f'Epoch {epoch}/{self.config["training"]["epochs"]} - '
                            f'Train Loss: {avg_loss:.4f}, Val Loss: {val_loss:.4f}, '
                            f'Best Val Loss: {best_val_loss:.4f}')

                self.writer.add_scalar('Loss/train', avg_loss, epoch)
                self.writer.add_scalar('Loss/val', val_loss, epoch)

                # 保存最佳模型
                if val_loss < best_val_loss:
                    best_val_loss = val_loss
                    patience_counter = 0
                    self._save_checkpoint(epoch, is_best=True)
                    logging.info(f"保存最佳模型，验证损失: {best_val_loss:.4f}")
                else:
                    patience_counter += 1
                    if patience_counter >= self.config['training']['early_stop_patience']:
                        logging.info(f"早停触发，验证损失 {patience_counter} 轮未改善")
                        break

                # 定期保存 checkpoint
                if epoch % self.config['training']['save_interval'] == 0:
                    self._save_checkpoint(epoch)

        logging.info("训练完成！")
        if self.args.local_rank in [0, -1]:
            logging.info(f"最佳验证损失: {best_val_loss:.4f}")

    def validate(self):
        """验证模型"""
        self.model.eval()
        total_loss = 0

        with torch.no_grad():
            progress_bar = tqdm(self.val_loader, desc='Validating',
                              disable=(self.args.local_rank != 0))
            for batch in progress_bar:
                input_ids = batch['input_ids'].to(self.device)
                attention_mask = batch['attention_mask'].to(self.device)
                labels = batch['labels'].to(self.device)

                # 使用混合精度进行验证
                with torch.amp.autocast(device_type='cuda', enabled=self.use_fp16):
                    outputs = self.model(
                        input_ids=input_ids,
                        attention_mask=attention_mask,
                        labels=labels
                    )
                    total_loss += outputs.loss.item()

        # 在分布式训练中，需要同步验证损失
        if self.args.local_rank != -1:
            total_loss_tensor = torch.tensor(total_loss, device=self.device)
            torch.distributed.all_reduce(total_loss_tensor, op=torch.distributed.ReduceOp.SUM)
            total_loss = total_loss_tensor.item() / torch.distributed.get_world_size()

        return total_loss / len(self.val_loader)

    def _save_checkpoint(self, epoch, is_best=False):
        """保存模型 checkpoint"""
        if self.args.local_rank not in [0, -1]:
            return  # 只在主进程中保存

        model_to_save = self.model.module if hasattr(self.model, 'module') else self.model

        if is_best:
            save_path = os.path.join(self.out_dir, 'best_model')
        else:
            save_path = os.path.join(self.out_dir, f'checkpoint_epoch{epoch}')

        os.makedirs(save_path, exist_ok=True)

        # 保存模型和 tokenizer
        model_to_save.save_pretrained(save_path)
        self.tokenizer.save_pretrained(save_path)

        # 保存训练状态
        checkpoint_state = {
            'epoch': epoch,
            'model_state_dict': model_to_save.state_dict(),
            'optimizer_state_dict': self.optimizer.state_dict(),
            'scheduler_state_dict': self.scheduler.state_dict(),
            'best_val_loss': self.config.get('best_val_loss', float('inf')),
            'config': self.config,
        }

        # 如果使用 FP16，保存 scaler 状态
        if self.use_fp16 and self.scaler is not None:
            checkpoint_state['scaler_state_dict'] = self.scaler.state_dict()

        checkpoint_path = os.path.join(save_path, 'training_state.pt')
        torch.save(checkpoint_state, checkpoint_path)

        logging.info(f'Checkpoint saved to {save_path}')

if __name__ == '__main__':
    try:
        args = parse_args()
        config = load_config(args.config)

        # 使用上下文管理器确保资源正确清理
        with Trainer(config, args) as trainer:
            trainer.train()
    except KeyboardInterrupt:
        logging.info("训练被用户中断")
        print("\n训练被用户中断")
        sys.exit(0)
    except FileNotFoundError as e:
        logging.error(f"文件错误: {e}")
        print(f"✗ 文件错误: {e}", file=sys.stderr)
        sys.exit(1)
    except ValueError as e:
        logging.error(f"配置错误: {e}")
        print(f"✗ 配置错误: {e}", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        logging.error(f"训练失败: {e}", exc_info=True)
        print(f"✗ 训练失败: {e}", file=sys.stderr)
        sys.exit(1)
