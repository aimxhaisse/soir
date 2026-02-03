"""
The **rnd** module provides facilities to introduce randomness in the
composition.

@public

``` python
rnd.between(1, 10)
```
"""

import random


def between(begin: float, end: float) -> float:
    """Get a random number between two values.

    @public

    Args:
         begin: The lower bound.
         end: The upper bound.
    """
    return random.uniform(begin, end)


def one_in(chance: int) -> bool:
    """Returns True with a chance of 1 in `chance`.

    @public

    Args:
         chance: The chance of returning True.
    """
    return random.randint(0, chance) == 0
