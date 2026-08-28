from typing import Any, SupportsFloat

import gymnasium as gym


class Monitor(gym.Wrapper):
    def __init__(self, env: gym.Env):
        super().__init__(env)
        self.episode_length = 0.
        self.episode_reward = 0.

    def reset(self, *, seed: int | None = None, options: dict[str, Any] | None = None) -> tuple[Any, dict[str, Any]]:
        obs, info = super().reset(seed=seed, options=options)

        info['episode_length'] = self.episode_length
        info['episode_reward'] = self.episode_reward

        self.episode_length = self.episode_reward = 0
        return obs, info


    def step(self, action: Any) -> tuple[Any, SupportsFloat, bool, bool, dict[str, Any]]:
        obs, reward, ter, tru, info = super().step(action)

        self.episode_length += 1.0
        self.episode_reward += float(reward)

        return obs, reward, ter, tru, info
    