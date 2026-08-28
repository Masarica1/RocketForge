import argparse
import atexit
import shutil
from collections.abc import Callable
from datetime import datetime
from pathlib import Path
from typing import Any, Final
from zoneinfo import ZoneInfo

import numpy as np
import torch
import wandb
from rich import box
from rich import print as printer
from rich.live import Live
from rich.table import Table
from stable_baselines3 import PPO
from stable_baselines3.common.env_checker import check_env
from wandb.apis.public import Run

from learning.agent.ppo import Network
from learning.environment.mono_env import MonoEnv
from learning.environment.typing import ActType, ObsType
from learning.simulation.simulation import Simulation as Sim
from learning.simulation.simulation import get_action_from_keyboard, window_should_close


def make_mdp_table(obs: ObsType, rewards: list[float]) -> Table:
    table = Table(title='mdp', box=box.ROUNDED)

    # columns
    table.add_column('Name')
    table.add_column('Value')

    # rows
    ## obs
    table.add_row('[bold cyan]Observation[/bold cyan]', '')
    table.add_row('pos_x', f'{obs[0]:.3f}')
    table.add_row('pos_y', f'{obs[1]:.3f}')
    table.add_row('vel_x', f'{obs[2]:.3f}')
    table.add_row('vel_y', f'{obs[3]:.3f}')
    table.add_row('sin', f'{obs[4]:.3f}')
    table.add_row('cos', f'{obs[5]:.3f}')
    table.add_row('ang_vel', f'{obs[6]:.3f}', end_section=True)

    ## reward
    table.add_row('[bold cyan]Reward[/bold cyan]', '')
    table.add_row('live', f'{rewards[0]:.3f}')
    table.add_row('x_ratio', f'{rewards[1]:.3f}')
    table.add_row('y_ratio', f'{rewards[2]:.3f}')
    table.add_row('ang', f'{rewards[3]:.3f}')
 
    return table


def simulation_test(
        action_provider: Callable[[ObsType], ActType],
        step_callback: Callable[[ObsType, ActType, list[float], bool, bool], None]|None = None,
        initial_state_radomization: bool = True,
        timeout: int = 120 * 100
    ):
    sim = Sim(timeout, action_period=2, window_size=(1920, 1080))
    if initial_state_radomization:
        sim.reset(seed=int(np.random.default_rng().integers(0, 2**32)))

    while not window_should_close():
        act = action_provider(sim.get_obs())

        sim.step(action=act)
        sim.render()

        if step_callback is not None:
            step_callback(sim.get_obs(), act, sim.get_reward_list(), sim.get_terminated(), sim.get_truncated())

        if sim.get_terminated():
            sim.reset(seed=int(np.random.default_rng().integers(0, 2**32)))

    sim.close()


def hueristic_test():
    def get_action(_: Any) -> ActType:
        action = get_action_from_keyboard()

        return np.array(action, dtype=np.int32)


    with Live(Table()) as live:
        simulation_test(
            action_provider=get_action,
            step_callback=lambda o, a, r, _, __ : live.update(make_mdp_table(o, r))
        )


def training_test():
    env = MonoEnv(120*120, action_period=3)  
    check_env(env)

    model = PPO(
        policy='MlpPolicy',
        env = env,
        verbose=0,
        device='cpu',
        tensorboard_log="./logs/training_test"
    )

    now = datetime.now(ZoneInfo("Asia/Seoul"))
    model.learn(
        total_timesteps=500_000,
        progress_bar=True,
        tb_log_name=f"run_{now.strftime("%m%d")}"
    )
    simulation_test(lambda obs : model.predict(obs)[0])

    with Live(Table()) as live:
        simulation_test(
            action_provider=lambda obs : model.predict(obs)[0],
            step_callback=lambda o, _, r, __, ___ : live.update(make_mdp_table(o, r))
        )


def model_test():
    parser = argparse.ArgumentParser('benchmark trained model on W&B')
    parser.add_argument('run_id', type=str, help='run name of trained model')
    parser.add_argument('--type', type=str, choices=['end', 'best'], default='best', help='choice model to use')
    parser.add_argument('--device', type=str, choices=['cuda', 'cpu'], default='cuda')
    parser.add_argument('--timeout', type=int)
    args = parser.parse_args()
    device = torch.device(args.device)

    WANDB_TEMP_PATH: Final = Path('./logs/temp')
    checkpoint_path = Path(f'checkpoint/checkpoint_{args.type}.pt')
    atexit.register(lambda : shutil.rmtree((WANDB_TEMP_PATH/checkpoint_path).parent))

    api = wandb.Api()
    run: Run = api.run(path=f'rocketforge/{args.run_id}')
    run.file(checkpoint_path.as_posix()).download(root=str(WANDB_TEMP_PATH), replace=True)

    network = Network().to(device)
    network.load_state_dict(
        state_dict=torch.load(
            f=WANDB_TEMP_PATH/checkpoint_path,
            map_location=device,
            weights_only=True
        )
    )
    network.eval()

    def action_provider(obs: ObsType) -> ActType:
        obs_tensor = torch.as_tensor(
            obs,
            dtype=torch.float32,
            device=device
        )

        with torch.inference_mode():
            action = network.get_action(obs_tensor, deterministic=True)

        return action.cpu().numpy().astype(np.int32)

    with Live(Table()) as live:
        if args.timeout:
            printer(f'[bold]INFO[/bold]: timeout option set to {args.timeout}')
            simulation_test(
                action_provider=action_provider,
                step_callback=lambda o, a, r, ter, tru : live.update(make_mdp_table(o, r)),
                timeout=args.timeout
            )
        else:
            simulation_test(
                action_provider=action_provider,
                step_callback=lambda o, a, r, ter, tru : live.update(make_mdp_table(o, r))
            )