<h1>soir <small>live coding environment</small>
  <div class="haiku-container">
    <i>
      No path but this—<br />
      Bell crickets singing<br />
      In the moonlight
    </i>
  </div>
</h1>

```python
@live
def setup():
    tracks.setup({"main": inst.mk_sampler()})

sp = sampler.new('basics')

@loop
def example(beats=4):
    """Welcome to Soir.
    """
    for i in range(4):
        log(f'Welcome to Soir')
        sleep(1)
```
