import argparse
import subprocess
import time
import random
import shlex

RANDOM_LIMIT = 1000
SEED = 123456789
random.seed(SEED)

AMMUNITION = [
    'localhost:8080/api/v1/maps/map1',
    'localhost:8080/api/v1/maps'
]

SHOOT_COUNT = 100
COOLDOWN = 0.1


def start_server():
    parser = argparse.ArgumentParser()
    parser.add_argument('server', type=str)
    return parser.parse_args().server


def run(command, input=None, output=None):
    process = subprocess.Popen(shlex.split(command), stdout=output, stderr=subprocess.DEVNULL)
    return process


def stop(process, wait=False):
    if process.poll() is None and wait:
        process.wait()
    process.terminate()


def shoot(ammo):
    hit = run('curl ' + ammo, output=subprocess.DEVNULL)
    time.sleep(COOLDOWN)
    stop(hit, wait=True)


def make_shots():
    for _ in range(SHOOT_COUNT):
        ammo_number = random.randrange(RANDOM_LIMIT) % len(AMMUNITION)
        shoot(AMMUNITION[ammo_number])
    print('Shooting complete')

def run_with_perf(pid):
    return run('sudo perf record -g --call-graph dwarf -o perf.data -p ' + str(pid), output=subprocess.DEVNULL)

def execute_in_terminal(command, output=None):
    process = subprocess.Popen(command, shell=True, stdout=output, stderr=subprocess.DEVNULL)
    return process

def generate_flamegraph(perf_proc):
    while perf_proc.returncode is None:
        perf_proc.poll()
    flame_graph = execute_in_terminal('sudo perf script -i perf.data | ./FlameGraph/stackcollapse-perf.pl | ./FlameGraph/flamegraph.pl > graph.svg')
    while flame_graph.returncode is None:
        flame_graph.poll()

server = run(start_server())
perf = run_with_perf(server.pid)
make_shots()
stop(perf)
stop(server)
generate_flamegraph(perf)

time.sleep(1)
print('Job done')
