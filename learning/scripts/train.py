import yaml

from learning.agent.config_model import LearningConfig
from learning.agent.ppo import Agent


def train():
    with open('config.yaml', 'r') as config_yaml:
        data = yaml.safe_load(config_yaml)

    config = LearningConfig.model_validate(data)
    agent = Agent(learning_cfg=config)

    agent.learn()
