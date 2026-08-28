from typing import Annotated, Any, Literal

from pydantic import BaseModel, Field, computed_field, model_validator


class LearningConfig(BaseModel):
    env_config: 'EnvConfig'
    training_config: 'TrainingConfig'

    @property
    def env(self):
        return self.env_config

    @property
    def training(self):
        return self.training_config

    def config_dump(self, mode: Literal['python', 'json'] = 'json') -> dict[str, Any]:
        return self.env.model_dump(mode=mode) | self.training.model_dump(mode=mode)


class EnvConfig(BaseModel):
    exp_name: Annotated[str, Field(exclude=True)]
    """the name of experiments"""
    wandb_project_name: Annotated[str|None, Field(exclude=True)] = None
    """project name of W&B"""
    progress_bar: Annotated[bool, Field(exclude=True)] = True
    """use progress bar on cmd"""

    seed: int|None = None
    """seed of envronment"""
    randomize_initial_state: bool = True
    """determine whether randomize initial state of game or not"""
    torch_deterministic: bool = True
    """determine whether enable cuDNN or not"""
    device: Annotated[str, Field(exclude=True)] = 'cuda'
    """device which PPO runs on"""

    timeout: Annotated[int, Field(gt=0)] = 120 * 180
    """max time step of simulation"""
    action_period: Annotated[int, Field(gt=0)] = 2
    """simulation update count per action"""

    @computed_field
    @property
    def timeout_step(self) -> int:
        return self.timeout // self.action_period


class TrainingConfig(BaseModel):
    total_timesteps: int
    """total timesteps of the experiments"""

    learning_rate: float = 2e-4
    """the learning rate of the optimizer"""
    num_envs: int = 1
    """the number of parallel game environments"""
    num_steps: int = 2048
    """the number of steps to run in each environment per policy rollout"""
    anneal_lr: bool = False
    """Toggle learning rate annealing for policy and value networks"""
    gamma: float = 0.995
    """the discount factor gamma"""

    gae_lambda: float = 0.95
    """the lambda for the general advantage estimation"""
    num_minibatches: int = 32
    """the number of mini-batches"""
    update_epochs: int = 10
    """the K epochs to update the policy"""
    norm_adv: bool = True
    """Toggles advantages normalization"""
    clip_coef: float = 0.2
    """the surrogate clipping coefficient"""
    clip_vloss: bool = False
    """Toggles whether or not to use a clipped loss for the value function, as per the paper."""
    ent_coef: float = 0.02
    """coefficient of the entropy"""
    vf_coef: float = 0.5
    """coefficient of the value function"""
    max_grad_norm: float = 0.5
    """the maximum norm for the gradient clipping"""
    target_kl: float|None = None
    """the target KL divergence threshold"""

    @model_validator(mode='after')
    def validate_args(self) -> "TrainingConfig":
        batch_size = self.num_envs * self.num_steps

        assert self.num_envs > 0
        assert self.num_steps > 0
        assert self.num_minibatches > 0
        assert self.update_epochs > 0
        assert batch_size % self.num_minibatches == 0
        return self

    @property
    def lr(self) -> float:
        return self.learning_rate

    @computed_field
    @property
    def batch_size(self) -> int:
        return self.num_envs * self.num_steps

    @computed_field
    @property
    def minibatch_size(self) -> int:
        return self.batch_size // self.num_minibatches

    @computed_field
    @property
    def num_iterations(self) -> int:
        return self.total_timesteps // self.batch_size

    @computed_field
    @property
    def effective_timestep(self) -> int:
        return self.num_iterations * self.batch_size
