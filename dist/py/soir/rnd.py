"""

The **rnd** module provides facilities to introduce randomness in the
composition.

# Cookbook

## Get a random number between two values

``` python
rnd.between(1, 10)
```

# Reference
"""

import random


def between(begin, end):
    """Get a random number between two values.

     Args:
         begin: The lower bound.
         end: The upper bound.
    """
    return random.uniform(begin, end)
