# Usage

``` shell
python compositejob.py /full/path/to/this/directory NJOBS NPAR
```

+ `NJOBS`: total number of jobs to run
+ `NPAR` : maximum number of jobs to run simultaneously

The jobs are defined by

+ `model.mac`
+ `run.mac`

`run.mac` is used as a template in which `{job_dir}` and `{run_number}` (using
the full syntax implementend by Python's built-in `str.format`) will be replaced
with their corresponding values, to create a number of `run<N>.mac` files.

`NPAR` serves 2 purposes:

1. Avoiding throttling of the system.
2. Making `NJOBS` > `NPAR` allows splitting the work into smaller chunks, whose
   results can be inspected sooner.
   
