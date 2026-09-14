import atexit
import shutil
from collections.abc import Sequence
from datetime import datetime
from pathlib import Path
from typing import Final
from uuid import uuid4
from zoneinfo import ZoneInfo

import matplotlib.pyplot as plt
import torch
import wandb
from matplotlib.axes import Axes
from matplotlib.figure import Figure
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
from learning.agent.logger.collector import LogCollector, QueueCollector
from learning.utils.timer import Timer

Number = int|float

GYM_KEYS: Final = ('episode_length', 'episode_reward')
WANDB_TEMP_DIR: Final = Path('./logs/temp/')

class WandbLogger:
    def __init__(self, learning_cfg: LearningConfig) -> None:
        self.best_score = 0
        assert learning_cfg.env.wandb_project_name is not None
        learning_cfg.env.exp_name = WandbLogger.check_name_overlap(learning_cfg.env.exp_name)

        self.run = wandb.init(
            entity=None,
            project=learning_cfg.env.wandb_project_name,
            name=learning_cfg.env.exp_name,
            dir='./logs/',
            config=learning_cfg.model_dump(mode='json')
        )

        self.temp_dir = WANDB_TEMP_DIR / f"{datetime.now(ZoneInfo("Asia/Seoul")):%m%d_%H%M%S}_{uuid4().hex[:8]}"
        atexit.register(
            shutil.rmtree,
            self.temp_dir,
            ignore_errors=True
        )


    def write(self, collector: LogCollector, step: int):
        self.run.log(data=collector.log_data, step=step)


    def save_weights(self, module: torch.nn.Module, collector: QueueCollector, benchKey: str):
        if collector.len_data.get(benchKey, 0) > collector.maxlen * 0.75:  # noqa: SIM102
            if (score := collector.log_data.get(benchKey, 0)) > self.best_score:
                self.best_score = score

                checkpoint_path = self.temp_dir / 'checkpoint' / 'checkpoint_best.pt'
                checkpoint_path.parent.mkdir(parents=True, exist_ok=True)

                torch.save(module.state_dict(), checkpoint_path)
                self.run.save(
                    glob_str=checkpoint_path,
                    base_path=self.temp_dir,
                    policy='now'
                )

    def save_timer_chart(self):
        fig : Figure
        axes : Sequence[Axes]
        fig, axes = plt.subplots(1, 2, figsize=(8, 4))

        labels = list(Timer.time_dict.keys())
        values = [v / 60.0 for v in Timer.time_dict.values()]

        axes[0].bar(labels, values)
        axes[0].set_title('Process Time Spanded')
        axes[0].set_xlabel('Process name')
        axes[0].set_ylabel('Time [min]')
        axes[0].grid(True, axis='y', linestyle='--', alpha=0.5)

        axes[1].pie(values, labels=labels, autopct="%1.1f%%")
        axes[1].set_title('Process Time Proportion [%]')
        fig.tight_layout()

        chart_path = self.temp_dir / "timer_chart.png"
        chart_path.parent.mkdir(parents=True, exist_ok=True)
        plt.savefig(chart_path, dpi=150)
        self.run.save(
            glob_str=chart_path,
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

        self.run.summary['benchmark/best_episode_length'] = self.best_score
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
                    printer('[bold blue](Logger)[/] Enter new name of run:',end=' ')
                    display_name = input()
                case _:
                    printer('[bold][blue](Logger)[/] you can only enter yes or no')

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
            speed_estimate_period=600
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
