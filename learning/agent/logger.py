import atexit
import shutil
from collections import deque
from datetime import datetime
from pathlib import Path
from typing import Any, Final
from uuid import uuid4
from zoneinfo import ZoneInfo

import numpy as np
import torch
import wandb
from rich import print as printer
from rich.progress import (
    BarColumn,
    Progress,
    TaskID,
    TaskProgressColumn,
    TextColumn,
    TimeElapsedColumn,
    TimeRemainingColumn,
)
from wandb.apis.public import Run, Runs

from learning.agent.config_model import LearningConfig

Number = int|float

GYM_KEYS: Final = ('episode_length', 'episode_reward')
WANDB_TEMP_DIR: Final = Path('./logs/temp/')

class WandbLogger:
    def __init__(self, learning_cfg: LearningConfig, max_len: int = 15) -> None:
        self.metric: dict[str, list[Number]] = {}
        self.queue: dict[str, deque[Number]] = {}
        self.best_episode_length = 0

        assert max_len > 0
        self.max_len = max_len

        assert learning_cfg.env.wandb_project_name is not None
        learning_cfg.env.exp_name = WandbLogger.check_name_overlap(learning_cfg.env.exp_name)

        self.run = wandb.init(
            entity=None,
            project=learning_cfg.env.wandb_project_name,
            name=learning_cfg.env.exp_name,
            dir='./logs/',
            config=learning_cfg.config_dump()
        )

        self.temp_dir = WANDB_TEMP_DIR / f"{datetime.now(ZoneInfo("Asia/Seoul")):%m%d_%H%M%S}_{uuid4().hex[:8]}"
        atexit.register(
            shutil.rmtree,
            self.temp_dir,
            ignore_errors=True
        )

    def add_metric(self, data: dict[str, Number]):
        for key, value in data.items():
            if (self.metric.get(key) is None):
                self.metric[key] = []

            self.metric[key].append(value)

    def add_queue(self, info: dict[str, Any]):
        for key in GYM_KEYS:
            if key not in info:
                continue

            mask = info.get(f'_{key}', np.ones(len(info[key]), dtype=bool))

            wandb_key = f'rollout/{key}'
            for value in info[key][mask]:
                if (self.queue.get(wandb_key) is None):
                    self.queue[wandb_key] = deque(maxlen=self.max_len)

                self.queue[wandb_key].append(value)

    def write_queue(self, step: int):
        log_data: dict[str, Number] = {}
        for key, value_queue in self.queue.items():
            log_data[key] = sum(value_queue) / len(value_queue)

        self.run.log(data=log_data, step=step)

    def write_metric(self, step: int):
        log_data: dict[str, Number] = {}

        for key, value_list in self.metric.items():
            log_data[key] = sum(value_list) / len(value_list)

        self.run.log(data=log_data, step=step)
        self.metric.clear()

    def save_weights(self, module: torch.nn.Module):
        if (queue := self.queue.get('rollout/episode_length')) is not None and len(queue) > 0.75*self.max_len:
            current_length = sum(queue) / len(queue)

            if (current_length > self.best_episode_length):
                self.best_episode_length = current_length

                checkpoint_path = self.temp_dir / 'checkpoint' / 'checkpoint_best.pt'
                checkpoint_path.parent.mkdir(parents=True, exist_ok=True)

                torch.save(module.state_dict(), checkpoint_path)
                self.run.save(
                    glob_str=checkpoint_path,
                    base_path=self.temp_dir,
                    policy='now'
                )

                
    def finish(self, module: torch.nn.Module|None = None, summary_data: dict[str, Number]|None = None):
        if module:
            checkpoint_path = self.temp_dir / 'checkpoint' / 'checkpoint_end.pt'
            checkpoint_path.parent.mkdir(parents=True, exist_ok=True)

            torch.save(module.state_dict(), checkpoint_path)
            self.run.save(
                glob_str=checkpoint_path,
                base_path=self.temp_dir,
                policy='now'
            )

        self.run.summary['benchmark/best_episode_length'] = self.best_episode_length
        if summary_data:
            self.run.summary.update(summary_data)

        self.run.finish()


    @staticmethod
    def check_name_overlap(display_name: str) -> str:
        previous_names: set[str] = set()

        runs: Runs = wandb.Api().runs('leeh2-yonsei/rocketforge')
        for run in runs:
            assert isinstance(run, Run)

            if run.name is not None:
                previous_names.add(run.name)

        while display_name in previous_names:
            printer(
                f'[bold blue](Logger)[/] [bold yellow]WARNING[/]: Run name {display_name} already exists in W&B\n'
                '[bold blue](Logger)[/] Do you want to overlap(yes) or change(no)?',
                end=' '
            )
            match input():
                case 'yes':
                    break
                case 'no':
                    printer(
                        '[bold blue](Logger)[/] Enter new name of run:',
                        end=' '
                    )
                    display_name = input()
                case _:
                    printer(
                        '[bold][blue](Logger)[/] you can only enter yes or no'
                    )

        return display_name


class ProgressLogger:
    def __init__(self, total: float) -> None:
        self.total: Final = total
        self.progress_bar = Progress(
            TextColumn("[bold]{task.description}:"),
            BarColumn(bar_width=120),
            TaskProgressColumn(text_format="[progress.percentage][{task.percentage:>3.0f}%]"),
            TextColumn("Elasped:"), TimeElapsedColumn(),
            TextColumn("ETA:"), TimeRemainingColumn(),
            speed_estimate_period=360
        )
        self.task_id: TaskID|None = None

    def start(self, task_name: str = 'Training'):
        self.progress_bar.start()
        self.task_id = self.progress_bar.add_task(task_name, total=self.total)

    def update(self, completed: int):
        assert self.task_id is not None
        self.progress_bar.update(self.task_id, completed=completed)

    def end(self):
        if self.task_id is None:
            return

        self.progress_bar.stop()
        self.task_id = None
