from typing import Any, SupportsFloat, SupportsInt, override

import gymnasium as gym
import numpy as np
from gymnasium import spaces

from learning.environment.typing import OBS_LENGTH, ActType, ObsType
from learning.simulation.simulation import Simulation


class MonoEnv(gym.Env[ObsType, ActType]):
    def __init__(
            self,
            timeout: int,
            *,
            action_period: int = 2,
            randomize_initial_state: bool = True,
            window_size: tuple[SupportsInt, SupportsInt]|None = None
    ) -> None:
        super().__init__()

        self.sim = Simulation(timeout, action_period, window_size)
        self.observation_space = spaces.Box(low=-1., high=1., shape=(OBS_LENGTH,), dtype=np.float32)
        self.action_space = spaces.MultiBinary(3)

        self.randomize_initial_state = randomize_initial_state

    @override
    def step(self, action: ActType) -> tuple[ObsType, SupportsFloat, bool, bool, dict[str, Any]]:
        self.sim.step(action)

        return self.sim.get_obs(), self.sim.get_reward(), self.sim.get_terminated(), self.sim.get_truncated(), {}

    @override
    def reset(self, *, seed: int|None = None, options: dict[str, Any]|None = None) -> tuple[ObsType, dict[str, Any]]:
        super().reset(seed=seed)

        if self.randomize_initial_state:
            sim_seed = int(self.np_random.integers(0, 2**32, dtype=np.uint32))
            self.sim.reset(sim_seed)
        else:
            self.sim.reset(None)

        return self.sim.get_obs(), {}

    @override
    def render(self):
        self.sim.render()

    @override
    def close(self):
        self.sim.close()
    
        