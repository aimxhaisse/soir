# How to Make Your Sample Pack

In this guide we show how to make your sample pack for `soir`.

## Preparing Samples

Sample packs can be built from a directory containing WAV files, the first step is
to convert them to 48kHz, this can be done using the `sample-convert-48khz` command:

```sh
soir sample-convert-48khz sample-pack
```

Where `sample-pack` point to a directory containing your samples.
    
!!! info

    The conversion is done in-place, your current samples will be replaced
    by 48kHz ones. This is a destructive operation so make sure you keep your
    original samples in a safe place.

## Creating the Sample Pack

In `soir`, a sample pack is composed of a reference file containing a
description of each sample and an associated directory in which actual
samples are. To generate the reference file, use the `sample-mk-pack`
command:

```sh
soir sample-mk-pack sample-pack
```

This results in a `sample-pack.pack.yaml` file.
        
## Install Pack

You can now install your pack using the `sample-install-pack` command:

```sh
soir sample-install-pack sample-pack
```

This copies the reference file and the sample directory in the
`SOIR_DIR` directory, so that it can be used in your live sessions:

```python
s = sampler.new('sample-pack')

@loop
def kick(beats=4):
    for i in range(4):
        s.play('kick')
        sleep(1)
```
