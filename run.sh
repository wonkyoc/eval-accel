#!/bin/bash
SIZE=(64 512 1024 2048)

TASKSET="taskset -c 1"
XCLBIN=build/hw/vadd.xclbin
WP=~/research/scheduling/eval-simple-rw

for ((i = 0; i < ${#SIZE[@]}; i++)); do
    for ((j = 0; j < 5; j++)); do
        C_CMD="ssh henry ${TASKSET} $WP/client -m ${SIZE[$i]}"
        CMD="${TASKSET} ./host_vadd -x ${XCLBIN} -m ${SIZE[$i]}"
        $CMD | tee -a host.log &
        host_pid="$!"
        $C_CMD >> client.log
        wait $host_pid
    done
done
