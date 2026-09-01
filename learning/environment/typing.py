from typing import Final

import jaxtyping as jxt
import numpy as np

OBS_LENGTH: Final = 12
ObsType = jxt.Float32[np.ndarray, f"{OBS_LENGTH}"]

ACT_LENGTH: Final = 3
ActType = jxt.Float32[np.ndarray, f"{ACT_LENGTH}"]