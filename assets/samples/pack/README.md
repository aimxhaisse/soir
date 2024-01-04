# Pack format for the sampler

We follow the following convention for loop-based packs:

```
|--------+---------|
| Type   | Notes   |
|--------+---------|
| Kick   | [0,9]   |
| Snare  | [10,19] |
| Cymbal | [20,29] |
| HH     | [30,39] |
| Bass   | [40,49] |
| Synths | [50,59] |
| Shaker | [60,69] |
| Noise  | [70,79] |
```

This is meant to change a lot as we get used to it, it's just based on
the first pack we are using which had those components.
