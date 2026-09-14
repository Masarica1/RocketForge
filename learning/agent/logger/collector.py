from abc import ABC, abstractmethod
from collections import deque
from collections.abc import Iterable
from typing import Any, override

import numpy as np
from numpy.typing import NDArray

from learning.simulation.simulation import EpisodeState


class LogCollector(ABC):
    @abstractmethod
    def add(self, data: dict[str, Any]) -> None:
        ...

    @property
    @abstractmethod
    def log_data(self) -> dict[str, float]:
        ...

    @property
    @abstractmethod
    def len_data(self) -> dict[str, int]:
        ...



class MetricCollector(LogCollector):
    def __init__(self) -> None:
        super().__init__()

        self._metric: dict[str, list[float]] = {}

    @override
    def add(self, data: dict[str, Any]) -> None:
        for key, value in data.items():
            try:
                value = float(value)
                self._metric[key] = self._metric.get(key, []) + [value]
            except (TypeError, ValueError):
                continue

    @property
    @override
    def log_data(self) -> dict[str, float]:
        result: dict[str, float] = {}

        for key, data_list in self._metric.items():
            if len(data_list) > 0:
                result[key] = sum(data_list) / len(data_list)

        self._metric.clear()
        return result

    @property
    @override
    def len_data(self) -> dict[str, int]:
        return {
            key: len(value)
            for key, value in self._metric.items()
        }


class QueueCollector(LogCollector):
    def __init__(self, maxlen: int = 15) -> None:
        super().__init__()

        if maxlen <= 0:
            raise ValueError(f'maxlen need to longer than 0, but {maxlen}')

        self.maxlen = maxlen
        self._queue: dict[str, deque[float]] = {}

    @override
    def add(self, data: dict[str, Any]) -> None:
        for key, value in data.items():
            if isinstance(value, float):
                self._queue[key] = self._queue.get(key, deque(maxlen=self.maxlen))
                self._queue[key].append(value)

    def add_by_info(self, info: dict[str, NDArray[Any]], keys: Iterable[tuple[str, str]]):
        for key, section_name in keys:
            if key not in info:
                continue

            mask: NDArray[np.bool_] = info[f'_{key}']
            if mask.dtype != np.bool_:
                raise TypeError(f"info['_{key}'] must be a boolean mask, got dtype={mask.dtype}")

            for value in info[key][mask]:
                try:
                    if np.isfinite(value):
                        value = float(value)
                        self.add({f'{section_name}/{key}': value})
                except (TypeError, ValueError):
                    continue
            
    @property
    @override
    def log_data(self) -> dict[str, float]:
        result: dict[str, float] = {}

        for key, data_list in self._queue.items():
            if len(data_list) > 0:
                result[key] = sum(data_list) / len(data_list)

        return result

    @property
    @override
    def len_data(self) -> dict[str, int]:
        return {
            key: len(value)
            for key, value in self._queue.items()
        }


class EpisodeStateCollector(LogCollector):
    def __init__(self, maxlen: int = 15) -> None:
        self._queue: deque[EpisodeState] = deque(maxlen=maxlen)

    @override
    def add(self, data: dict[str, Any]) -> None:
        for value in data.values():
            if isinstance(value, EpisodeState) and value != EpisodeState.Alive:
                self._queue.append(value)


    def add_by_info(self, info: dict[str, Any]):
        final_info = info.get('final_info')
        if final_info is None:
            return

        state_data = final_info.get('state')
        mask: NDArray[np.bool_] = final_info.get('_state')
        if state_data is None or mask is None:
            return

        for value in state_data[mask]:
            if not isinstance(value, EpisodeState):
                raise TypeError('info[state] has an value whose type is not EpisodeState')
            if value != EpisodeState.Alive:
                self._queue.append(value)

    @property
    @override
    def log_data(self) -> dict[str, float]:
        result: dict[str, float] = {}

        if len(self._queue) <= 0:
            return {}

        for state in EpisodeState:
            if state == EpisodeState.Alive:
                continue

            result[f'episode_state/{state.name.lower()}'] = self._queue.count(state) / len(self._queue)

        return result

    @property
    @override
    def len_data(self) -> dict[str, int]:
        result: dict[str, int] = {}

        for state in EpisodeState:
            if state == EpisodeState.Alive:
                continue

            result[f'episode_state/{state.name.lower()}'] = len(self._queue)

        return result


