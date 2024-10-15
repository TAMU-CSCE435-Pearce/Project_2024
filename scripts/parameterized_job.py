#!/usr/bin/python3

# Set up a job file based on the provided parameters.
# This handles things like splitting p into nodes and tasks per node.

USAGE = './parameterized_job.py <executable path> <n> <p> <input type> [<wall_clock_time>]'

import os
import sys

if (len(sys.argv) < 5):
    print('Usage: {}'.format(USAGE))
    sys.exit(22)

exe_path = sys.argv[1]
if ('/' not in exe_path
    or not os.path.isfile(exe_path) or not os.access(exe_path, os.X_OK)):
    print('{} appears incorrectly formatted. (Example: ./mergesort)'.format(
        exe_path))
    sys.exit(22)

n = int(sys.argv[2])
p = int(sys.argv[3])
input_type = sys.argv[4]

wall_clock_limit = '00:30:00'
if (len(sys.argv) > 5):
    wall_clock_limit = sys.argv[5]


if (p <= 48):
    nodes = 1
else:
    assert p % 32 == 0, "Unexpected large p value that isn't a multiple of 32."
    nodes = p // 32

tasks_per_node = p // nodes
memory_per_node = str(8 * tasks_per_node) + 'G'

output_path = 'run_output/{}/n{}-p{}-{}'.format(
        os.path.basename(exe_path), n, p, input_type)
os.makedirs(os.path.dirname(output_path), exist_ok=True)

# Output path: implementation/n/p/input_type

with open('template.grace_job') as f:
    template = f.read()

with open('output.grace_job', 'w') as f:
    f.write(template.format(
        wall_clock_time=wall_clock_limit,
        node_count=nodes,
        tasks_per_node=tasks_per_node,
        memory_per_node=memory_per_node,
        output_path=output_path,
        process_count=p,
        executable_path=exe_path,
        array_size=n,
        input_type=input_type,
    ))
