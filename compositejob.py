from sys import argv
from pathlib import Path, PurePath
from subprocess import run
from threading import Thread

def file_to_string(path):
    return ''.join(open(path).readlines())

job_dir        = Path(argv[1])
N              =  int(argv[2])
template_run   = file_to_string(job_dir / 'run.mac')

threads = []
for n in range(N):
    filename_model = f'{job_dir}/model.mac'
    filename_run   = f'{job_dir}/run{n}.mac'
    with open(filename_run, 'w') as file_run:
        file_run.write(template_run.format(run_number=n, job_dir=job_dir))

    cmd = f'just run-full-path {filename_model} {filename_run}'
    t = Thread(target=run, args=(cmd,),
               kwargs=dict(shell=True,
                           capture_output=True))
    t.start()
    threads.append(t)


for n, thread in enumerate(threads, 1):
    thread.join()
    print(f'done {n}')
