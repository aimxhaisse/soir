#!/usr/bin/env python3

import dataclasses
import jinja2
import typer
import os
import sys


import typer


MIDI_NOTE_MIN_VALUE = 1
MIDI_NOTE_MAX_VALUE = 127


TEMPLATE = """\
name: {{ name }}
samples:
  {%- for sample in samples %}
  - name: {{ sample.name }}
    note: {{ sample.midi_note }}
    path: {{ sample.full_path }}
  {%- endfor %}
"""


app = typer.Typer()


@dataclasses.dataclass
class Sample:
    name: str
    midi_note: int
    full_path: str


def normalize_name(name: str):
    for char in ["-", "(", ")", "[", "]", "{", "}", ":", ";", ",", ".", "!", "?"]:
        name = name.replace(char, "_")
    return name.lower()
    

def get_samples_from_directory(directory: str):
    samples = []
    midi_note = MIDI_NOTE_MIN_VALUE
    names = set()
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith(".wav"):
                full_path = os.path.join(root, file)
                name = normalize_name(os.path.splitext(file)[0])
                if name in names:
                    raise ValueError(f"Collision in sample names: {name} already exist")
                names.add(name)
                samples.append(Sample(name=name, full_path=full_path, midi_note=midi_note))
                midi_note += 1
                if midi_note > MIDI_NOTE_MAX_VALUE:
                    raise ValueError(f"Too many samples in directory {directory}")
    return samples
 

@app.command()
def mk_sample_pack(name: str, directory: str, output: str):
    samples = get_samples_from_directory(directory)
    template = jinja2.Template(TEMPLATE)
    rendered = template.render(name=name, samples=samples)

    with open(output, "w") as f:
        f.write(rendered)


if __name__ == '__main__':
    app()
