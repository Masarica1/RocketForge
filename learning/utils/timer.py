import time
from collections.abc import Callable
from functools import wraps


class Timer:
    time_dict : dict[str, float]
    base_start: float|None = None

    @staticmethod
    def init():
        Timer.time_dict = {}

    @staticmethod
    def record[**P, R](func: Callable[P, R]) -> Callable[P, R]:
        @wraps(func)
        def wrapper(*args: P.args, **kwargs: P.kwargs) -> R:
            start = time.monotonic()
            result = func(*args, **kwargs)
            time_spanded = (time.monotonic() - start)

            func_name = func.__name__.strip("_")
            Timer.time_dict[func_name] = Timer.time_dict.get(func_name, 0.0) + time_spanded
            return result
        return wrapper

    @staticmethod
    def base[**P, R](func: Callable[P, R]) -> Callable[P, R]:
        @wraps(func)
        def wrapper(*args: P.args, **kwargs: P.kwargs) -> R:
            Timer.base_start = time.monotonic()
            return func(*args, **kwargs)

        return wrapper


    @staticmethod
    def get_data() -> dict[str, float]:
        total = (time.monotonic() - Timer.base_start) if Timer.base_start is not None else sum(Timer.time_dict.values())

        return {
            f'optimization/{key}' : data / float(total)
            for key, data in Timer.time_dict.items()
        } | {
            'optimization/other' : 1 - sum(Timer.time_dict.values()) / total
        }


