#!/usr/bin/env bash

for i in {0..8}
do
    curl http://localhost:7080/ --output "load-${i}.ogg" &
done

sleep 10

pkill curl
wait $(jobs -p)
