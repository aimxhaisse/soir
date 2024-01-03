#!/usr/bin/env bash

for i in {0..8}
do
    curl http://localhost:7080/ --output "load-${i}.ogg" &
done

sleep 10

kill -9 $(jobs -p)
wait $(jobs -p)
