"""Submit local abracadabra jobs

Usage: runjobs.py [--no-capture-output --DELETE-OLD-MC] JOBDIR NTOT NPAR

Arguments:
  FILES  Directory containing job description macros, and where results will
         collected
  NTOT   The number of jobs that should be run in total
  NPAR   The maximum number of jobs that should be running simultaneously

Options:
  --no-capture-output  Don't hide job's output from std-err/out
  --DELETE-OLD-MC      Remove `MC-*.h5` before starting runs.

Setting a limit on the number of jobs being run simultaneously prevents
throttling of the system.

Running a larger number of smaller jobs, rather than a smaller number of large
jobs, allows inspection and analysis of intermediate results before the full
production is finished.
"""

from sys import argv
from time import time
from pathlib import Path, PurePath
from functools import wraps
from subprocess import run
from threading import Thread, Semaphore
from queue import Queue
from docopt import docopt

def file_to_string(path):
    return ''.join(open(path).readlines())

args = docopt(__doc__)

job_dir        = Path(args['JOBDIR'])
NTOT           =  int(args['NTOT'])
NPAR           =  int(args['NPAR'])
DELETE_MC      = args['--DELETE-OLD-MC']; print(DELETE_MC)
template_run   = file_to_string(job_dir / 'run.mac')


def report_progress(fn):
    """Decorator reporting when a function starts running, returns, and the time it
    took.

    Requires the decorated function to have a keyword-argumen `n`"""
    @wraps(fn)
    def report_progress(*args, **kwds):
        n = kwds['n']
        print(f'starting {n:3}')
        start = time()
        fn(*args, **kwds)
        stop = time()
        print(f'                {n:3} done in {int(stop - start)}s')
    return report_progress


def with_semaphore(semaphore):
    "Decorator limiting the number of jobs running simultaneously"
    def with_semaphore(fn):
        @wraps(fn)
        def with_semaphore(*args, **kwds):
            with semaphore:
                fn(*args, **kwds)
        return with_semaphore
    return with_semaphore


@with_semaphore(Semaphore(value=NPAR))
@report_progress
def run_one_job(*, n, capture_output):
    filename_model = f'{job_dir}/model.mac'
    filename_run   = f'{job_dir}/run-{n}.mac'
    with open(filename_run, 'w') as file_run:
        file_run.write(template_run.format(run_number=n, job_dir=job_dir))
    cmd = f'just run-full-path {filename_model} {filename_run}'
    result = run(cmd, shell=True, capture_output=capture_output)
    if result.returncode != 0:
        save_run_output(result, job_dir, n)


def save_run_output(result, job_dir, n):
    with open(job_dir / f'job-{n}.stdout', 'wb') as stdout_file:
        stdout_file.write(result.stdout)
    with open(job_dir / f'job-{n}.stderr', 'wb') as stderr_file:
        stderr_file.write(result.stderr)


def clean_up_old_junk():
    junk_files = f'{job_dir}/*.std{{err,out}} {job_dir}/run-*.mac'
    cmd = f'rm -f {junk_files}'
    if DELETE_MC:
        cmd = f'{cmd} {job_dir}/MC-*.h5'
    print(cmd)
    run(cmd, shell=True)


clean_up_old_junk()

# Launch jobs
from time import sleep
threads = []
for n in range(NTOT):
    t = Thread(target = run_one_job,
               kwargs = dict(n              = n,
                             capture_output = not args['--no-capture-output']))
    sleep(0.01) # Seems to help avoid problems at job startup
    t.start()
    threads.append(t)

# Wait for all jobs to finish
for thread in threads: thread.join()
print('All done')
