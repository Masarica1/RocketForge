from abc import ABC, abstractmethod
from typing import Annotated, Literal, override

from pydantic import BaseModel, ConfigDict, Field


class LRConfig(BaseModel, ABC):
    model_config = ConfigDict(extra='forbid')

    @abstractmethod
    def get_current(self, ratio: float) -> float:
        ...


class Constant(LRConfig):
    type : Literal['constant']
    value: Annotated[float, Field(gt=0.0, allow_inf_nan=False)]

    @override
    def get_current(self, ratio: float) -> float:
        return self.value


class Linear(LRConfig):
    type : Literal['linear']
    start_value: Annotated[float, Field(ge=0.0, allow_inf_nan=False)]
    end_value: Annotated[float, Field(ge=0.0, allow_inf_nan=False)]
    end_ratio: Annotated[float, Field(gt=0.0, le=1.0, description='Training progress ratio at which end_value is reached')]

    @override
    def get_current(self, ratio: float) -> float:
        if ratio > self.end_ratio:
            return self.end_value
        else:
            return self.start_value + (self.end_value - self.start_value) * (ratio / self.end_ratio)


type LearningRate = Constant|Linear
