import unittest

import torch

from learning.agent.ppo import compute_gae


class ComputeGaeTest(unittest.TestCase):
    def test_termination_does_not_bootstrap(self) -> None:
        advantages, returns = compute_gae(
            rewards=torch.tensor([[1.0]]),
            values=torch.tensor([[2.0]]),
            next_values=torch.tensor([[7.0]]),
            terminations=torch.tensor([[1.0]]),
            truncations=torch.tensor([[0.0]]),
            gamma=0.9,
            gae_lambda=0.95,
        )

        torch.testing.assert_close(advantages, torch.tensor([[-1.0]]))
        torch.testing.assert_close(returns, torch.tensor([[1.0]]))

    def test_truncation_bootstraps_without_crossing_episode_boundary(self) -> None:
        advantages, returns = compute_gae(
            rewards=torch.tensor([[1.0], [100.0]]),
            values=torch.tensor([[2.0], [0.0]]),
            next_values=torch.tensor([[7.0], [0.0]]),
            terminations=torch.zeros((2, 1)),
            truncations=torch.tensor([[1.0], [0.0]]),
            gamma=0.9,
            gae_lambda=0.95,
        )

        # The first transition includes gamma * V(final_obs), but its GAE
        # must not include the following episode's large advantage.
        torch.testing.assert_close(advantages, torch.tensor([[5.3], [100.0]]))
        torch.testing.assert_close(returns, torch.tensor([[7.3], [100.0]]))


if __name__ == '__main__':
    unittest.main()
