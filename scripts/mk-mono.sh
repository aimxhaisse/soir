#!/usr/bin/env bash

TMP_SAMPLE="/tmp/tmp.wav"

if [ "$#" -ne 1 ]; then
    echo "usage: $0 <directory>"
fi

for SAMPLE in $(find $1 -name '*.wav')
do
    rm -f ${TMP_SAMPLE}
    ffmpeg -i $SAMPLE -ac 1 -ar 48000 ${TMP_SAMPLE}
    mv $TMP_SAMPLE $SAMPLE
done

# ffmpeg -i stereo.flac -ac 1 mono.flac
