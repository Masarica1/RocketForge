import random
import time
from dataclasses import dataclass

import numpy as np
import torch
from gymnasium.vector import AutoresetMode, SyncVectorEnv
from torch import Tensor, nn, optim
from torch.distributions import Bernoulli, Independent

from learning.agent.config_model import LearningConfig
from learning.agent.logger.collector import (
    EpisodeStateCollector,
    MetricCollector,
    QueueCollector,
)
from learning.agent.logger.logger import ProgressLogger, WandbLogger
from learning.environment.mono_env import MonoEnv
from learning.environment.typing import ACT_LENGTH, OBS_LENGTH
from learning.environment.wrapper import Monitor
from learning.utils.timer import Timer


def layer_init(layer: nn.Linear, std: float = np.sqrt(2), bias_const: float = 0.0) -> nn.Linear:
    nn.init.orthogonal_(layer.weight, std)
    nn.init.constant_(layer.bias, bias_const)
    return layer

@Timer.record
def compute_gae(
    rewards: Tensor,
    values: Tensor,
    next_values: Tensor,
    terminations: Tensor,
    truncations: Tensor,
    gamma: float,
    gae_lambda: float,
) -> tuple[Tensor, Tensor]:
    advantages = torch.zeros_like(rewards)
    lastgaelam = torch.zeros_like(rewards[0])

    for t in reversed(range(rewards.shape[0])):
        # A true termination has no successor value. A time-limit truncation
        # does bootstrap from its final observation, but both signals stop the
        # GAE trace so it cannot leak into the next episode.
        bootstrap_nonterminal = 1.0 - terminations[t]
        trace_nonterminal = 1.0 - torch.logical_or(
            terminations[t].bool(), truncations[t].bool()
        ).float()
        delta = (
            rewards[t]
            + gamma * next_values[t] * bootstrap_nonterminal
            - values[t]
        )
        lastgaelam = (
            delta
            + gamma * gae_lambda * trace_nonterminal * lastgaelam
        )
        advantages[t] = lastgaelam

    return advantages, advantages + values


class Network(nn.Module):
    def __init__(self) -> None:
        super().__init__()
        self.critic = nn.Sequential(
            layer_init(nn.Linear(OBS_LENGTH, 64)),
            nn.Tanh(),
            layer_init(nn.Linear(64, 64)),
            nn.Tanh(),
            layer_init(nn.Linear(64, 1), std=1.0)
        )
        self.actor = nn.Sequential(
            layer_init(nn.Linear(OBS_LENGTH, 64)),
            nn.Tanh(),
            layer_init(nn.Linear(64, 64)),
            nn.Tanh(),
            layer_init(nn.Linear(64, ACT_LENGTH), std=0.01)
        )

    def get_value(self, x: Tensor) -> Tensor:
        return self.critic(x)

    def get_action(self, x: Tensor, deterministic: bool = False) -> Tensor:
        logits = self.actor(x)

        if deterministic:
            return (logits>=0).to(torch.float32)

        return Bernoulli(logits=logits).sample()

    def get_action_and_value(self, x, action: Tensor|None=None) -> tuple[Tensor, Tensor, Tensor, Tensor]:
        logits = self.actor(x)
        probs = Independent(Bernoulli(logits=logits), 1)

        if action is None:
            action = probs.sample()
        return action, probs.log_prob(action), probs.entropy(), self.critic(x)



class Agent:
    @dataclass(slots=True)
    class RolloutBuffer:
        obs: Tensor
        actions: Tensor
        logprobs: Tensor
        rewards: Tensor
        terminations: Tensor
        truncations: Tensor
        values: Tensor
        next_values: Tensor

        @classmethod
        def make(cls, batch_shape: tuple[int, int], obs_shape: tuple[int, ...], act_shape: tuple[int, ...], device: str) -> 'Agent.RolloutBuffer':
            def batch_shape_tensor():
                return torch.zeros(batch_shape, device=device)

            return cls(
                torch.zeros(batch_shape + obs_shape, device=device),
                torch.zeros(batch_shape + act_shape, device=device),
                batch_shape_tensor(),
                batch_shape_tensor(),
                batch_shape_tensor(),
                batch_shape_tensor(),
                batch_shape_tensor(),
                batch_shape_tensor(),
            )


    def __init__(self, learning_cfg: LearningConfig) -> None:

        self.training_cfg = learning_cfg.training_config
        self.env_cfg = learning_cfg.env_config
        self.envs = SyncVectorEnv(
            env_fns=[
                lambda: Monitor(MonoEnv(
                    timeout=self.env_cfg.timeout,
                    action_period=self.env_cfg.action_period,
                    randomize_initial_state=self.env_cfg.randomize_initial_state
                ))
                for _ in range(self.training_cfg.num_envs)
            ],
            autoreset_mode=AutoresetMode.SAME_STEP
        )

        # random settings
        random.seed(self.env_cfg.seed)
        np.random.seed(self.env_cfg.seed)
        torch.manual_seed(self.env_cfg.seed if self.env_cfg.seed is not None else torch.seed())
        torch.backends.cudnn.deterministic = self.env_cfg.torch_deterministic

        self.network = Network().to(self.env_cfg.device)
        self.optimizer = optim.Adam(self.network.parameters(), lr=self.training_cfg.learning_rate.get_current(0.0), eps=1e-5,)
        self.optimizer_steps = 0

        self.logger: WandbLogger|None = None
        self.queue_collector: QueueCollector|None = None
        self.metric_collector: MetricCollector|None = None
        self.state_collector: EpisodeStateCollector|None = None
        if self.env_cfg.wandb_project_name is not None:
            self.logger = WandbLogger(learning_cfg)
            self.queue_collector = QueueCollector(maxlen=20)
            self.metric_collector = MetricCollector()
            self.state_collector = EpisodeStateCollector(maxlen=20)

        self.progress_bar = ProgressLogger(total=self.training_cfg.effective_timestep)  if self.env_cfg.progress_bar else None


    def learn(self):
        try:
            if self.progress_bar:
                self.progress_bar.start()

            self._learn()
        finally:
            self.close()

    @Timer.base
    def _learn(self):
        assert self.envs.single_observation_space.shape is not None
        assert self.envs.single_action_space.shape is not None

        # aliases
        device = self.env_cfg.device

        # make buffer
        buffer = Agent.RolloutBuffer.make(
            batch_shape=(self.training_cfg.num_steps, self.training_cfg.num_envs),
            obs_shape=self.envs.single_observation_space.shape,
            act_shape=self.envs.single_action_space.shape,
            device=self.env_cfg.device
        )

        next_obs, _ = self.envs.reset(seed=self.env_cfg.seed)
        next_obs = torch.as_tensor(next_obs, dtype=torch.float32, device=device)

        global_step = 0

        for iteration in range(1, self.training_cfg.num_iterations + 1):
            iteration_start_time = time.monotonic()
            # Evaluate the schedule at the start of each rollout, as in the
            # previous anneal_lr implementation: the first iteration starts at 0.
            progress = (iteration - 1) / self.training_cfg.num_iterations
            self.optimizer.param_groups[0]['lr'] = self.training_cfg.learning_rate.get_current(progress)

            next_obs, global_step = self._collect_rollout(
                buffer=buffer,
                initial_obs=next_obs,
                global_step=global_step
            )

            # Compute GAE with separate bootstrap and trace masks.
            with torch.no_grad():
                advantages, returns = compute_gae(
                    rewards=buffer.rewards,
                    values=buffer.values,
                    next_values=buffer.next_values,
                    terminations=buffer.terminations,
                    truncations=buffer.truncations,
                    gamma=self.training_cfg.gamma,
                    gae_lambda=self.training_cfg.gae_lambda,
                )

            # flatten the batch
            b_obs = buffer.obs.reshape((-1,) + self.envs.single_observation_space.shape)
            b_logprobs = buffer.logprobs.reshape(-1)
            b_actions = buffer.actions.reshape((-1,) + self.envs.single_action_space.shape)
            b_advantages = advantages.reshape(-1)
            b_returns = returns.reshape(-1)
            b_values = buffer.values.reshape(-1)

            # optimize the policy and value network
            completed_update_epochs = self._optimize(
                b_obs=b_obs,
                b_logprobs=b_logprobs,
                b_actions=b_actions,
                b_advantages=b_advantages,
                b_returns=b_returns,
                b_values=b_values
            )

            y_pred = b_values.cpu().numpy()
            y_true = b_returns.cpu().numpy()
            var_y = np.var(y_true)
            explained_var = np.nan if var_y == 0 else 1 - np.var(y_true - y_pred) / var_y

            if self.logger:
                assert self.metric_collector is not None

                self.metric_collector.add({
                    'train/update_epochs': completed_update_epochs,
                    'train/learning_rate': self.optimizer.param_groups[0]['lr'],

                    'value/target_std': returns.std().item(),
                    'value/prediction_std': b_values.std().item(),
                    'value/prediction_mean': b_values.mean().item(),
                    'value/explained_variance': float(explained_var),

                    'rollout/advantage_std': b_advantages.std().item(),
                    'rollout/observation_std': b_obs.std(dim=0, correction=0).mean().item(),
                    'rollout/step_reward_mean': buffer.rewards.mean().item(),

                    'optimization/entire_sps' : self.training_cfg.batch_size / (time.monotonic() - iteration_start_time)
                })
                self.logger.write(self.metric_collector, step=global_step)

    @Timer.record
    def _collect_rollout(self, buffer: RolloutBuffer, initial_obs: Tensor, global_step: int) -> tuple[Tensor, int]:
        
        device = self.env_cfg.device

        next_obs = initial_obs

        for step in range(self.training_cfg.num_steps):
            step_start_time = time.monotonic()
            buffer.obs[step] = next_obs

            with torch.no_grad():
                action, logprob, _, value = self.network.get_action_and_value(next_obs)
                buffer.values[step] = value.flatten()
            buffer.actions[step] = action
            buffer.logprobs[step] = logprob

            # execute simulation & collect data
            env_next_obs, reward, termination, truncation, info = self.envs.step(action.cpu().numpy())
            done = np.logical_or(termination, truncation)
            buffer.rewards[step] = torch.as_tensor(reward, dtype=torch.float32, device=device)
            buffer.terminations[step] = torch.as_tensor(termination, dtype=torch.float32, device=device)
            buffer.truncations[step] = torch.as_tensor(truncation, dtype=torch.float32, device=device)

            # SAME_STEP returns the reset observation for completed envs.
            # Use final_obs instead when evaluating the transition's true
            # successor, which is required to bootstrap truncations.
            bootstrap_obs = np.array(env_next_obs, copy=True)
            if np.any(done):
                final_obs = info.get('final_obs')
                final_obs_mask = info.get('_final_obs')
                if final_obs is None or final_obs_mask is None or not np.all(final_obs_mask[done]):
                    raise RuntimeError('SyncVectorEnv did not provide final_obs for a completed environment')
                bootstrap_obs[done] = np.stack(final_obs[done])

            with torch.no_grad():
                buffer.next_values[step] = self.network.get_value(
                    torch.as_tensor(bootstrap_obs, dtype=torch.float32, device=device)
                ).flatten()
            next_obs = torch.as_tensor(env_next_obs, dtype=torch.float32, device=device) 
            global_step = global_step + self.training_cfg.num_envs

            if self.logger:
                assert self.queue_collector is not None
                assert self.metric_collector is not None
                assert self.state_collector is not None

                self.queue_collector.add({'optimization/simulation_sps' : self.training_cfg.num_envs / (time.monotonic() - step_start_time)})
                self.queue_collector.add_by_info(info=info, keys=[('episode_length', 'rollout'), ('episode_reward', 'rollout')])
                self.state_collector.add_by_info(info=info)
                
                self.logger.write(self.queue_collector, global_step)
                self.logger.write(self.state_collector, global_step)
                self.logger.save_weights(self.network, self.queue_collector, 'rollout/episode_length')
            if self.progress_bar:
                self.progress_bar.update(completed=global_step)

        return next_obs, global_step

    @Timer.record
    def _optimize(
            self,
            b_obs: Tensor,
            b_logprobs: Tensor,
            b_actions: Tensor,
            b_advantages: Tensor,
            b_returns: Tensor,
            b_values: Tensor
        ) -> int:

        b_inds = np.arange(self.training_cfg.batch_size)
        completed_update_epochs = 0

        for update_epoch in range(self.training_cfg.update_epochs):
            approx_kl: Tensor|None = None
            np.random.shuffle(b_inds)

            for start in range(0, self.training_cfg.batch_size, self.training_cfg.minibatch_size):
                end = start + self.training_cfg.minibatch_size
                mb_inds = b_inds[start:end]

                _, newlogprob, entropy, newvalue = self.network.get_action_and_value(
                    b_obs[mb_inds],
                    b_actions[mb_inds],
                )
                logratio = newlogprob - b_logprobs[mb_inds]
                ratio = logratio.exp()

                with torch.no_grad():
                    approx_kl = ((ratio - 1) - logratio).mean()
                    clip_frac = (
                        (ratio - 1.0).abs() > self.training_cfg.clip_coef
                    ).float().mean()

                mb_advantages = b_advantages[mb_inds]
                if self.training_cfg.norm_adv:
                    mb_advantages = (mb_advantages - mb_advantages.mean()) / (
                        mb_advantages.std() + 1e-8
                    )

                # policy loss
                pg_loss1 = -mb_advantages * ratio
                pg_loss2 = -mb_advantages * torch.clamp(
                    ratio,
                    1 - self.training_cfg.clip_coef,
                    1 + self.training_cfg.clip_coef,
                )
                pg_loss = torch.max(pg_loss1, pg_loss2).mean()

                # value loss
                newvalue = newvalue.view(-1)
                if self.training_cfg.clip_vloss:
                    v_loss_unclipped = (newvalue - b_returns[mb_inds]) ** 2
                    v_clipped = b_values[mb_inds] + torch.clamp(
                        newvalue - b_values[mb_inds],
                        -self.training_cfg.clip_coef,
                        self.training_cfg.clip_coef,
                    )
                    v_loss_clipped = (v_clipped - b_returns[mb_inds]) ** 2
                    v_loss = 0.5 * torch.max(v_loss_unclipped, v_loss_clipped).mean()
                else:
                    v_loss = 0.5 * ((newvalue - b_returns[mb_inds]) ** 2).mean()

                entropy_loss = entropy.mean()
                loss = (
                    pg_loss
                    - self.training_cfg.ent_coef * entropy_loss
                    + self.training_cfg.vf_coef * v_loss
                )

                self.optimizer.zero_grad()
                loss.backward()
                nn.utils.clip_grad_norm_(self.network.parameters(), self.training_cfg.max_grad_norm)
                self.optimizer.step()
                self.optimizer_steps += 1

                if self.logger:
                    assert self.metric_collector is not None

                    self.metric_collector.add({
                        'train/actor_loss': pg_loss.item(),
                        'train/critic_loss': v_loss.item(),
                        'train/entropy_loss': entropy_loss.item(),
                        'train/total_loss': loss.item(),
                        'train/approx_kl': approx_kl.item(),
                        'train/clip_frac': clip_frac.item(),
                    })

            completed_update_epochs = update_epoch + 1
            if (
                self.training_cfg.target_kl is not None
                and approx_kl is not None
                and approx_kl.item() > self.training_cfg.target_kl
            ):
                break

        return completed_update_epochs

    
    def close(self):
        self.envs.close()

        if self.progress_bar:
            self.progress_bar.end()
        if self.logger:
            assert self.state_collector is not None

            print(f'state collected: {len(self.state_collector._queue)}')

            self.logger.save_timer_chart()
            self.logger.finish(
                module=self.network,
                summary_data={
                    'config/max_optimizer_steps': self.training_cfg.max_optimizer_steps,
                    'config/real_optimizer_steps': self.optimizer_steps
                }
                | Timer.get_data()
            )
        self.optimizer_steps = 0
