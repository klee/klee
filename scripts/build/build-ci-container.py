#!/usr/bin/env python3

import yaml
import subprocess
import os
import sys

(abs_path, _) = os.path.split(os.path.abspath(__file__))

with open(os.path.join(abs_path,"../../.github/workflows/build.yaml"), 'r') as stream:
    try:
        ci_config = yaml.safe_load(stream)
        global_env = ci_config['env']
        build_jobs = ('Linux', 'Coverage')

        for job_name in build_jobs:
            job_config = ci_config['jobs'][job_name]
            job_env = job_config.get('env', {})

            for matrix_entry in job_config['strategy']['matrix']['include']:
                print("Building: {} ({})".format(job_name, matrix_entry['name']))

                build_env = os.environ.copy()

                # Copy current global build configurations
                for k,v in global_env.items():
                    build_env[k] = str(v)

                # Override with job-level values
                for k,v in job_env.items():
                    build_env[k] = str(v)

                # Override with matrix-entry-specific values
                for k,v in matrix_entry.get('env', {}).items():
                    build_env[k] = str(v)

                cmd = [os.path.join(abs_path, 'build.sh'),
                      'klee', # build KLEE and all its dependencies
                      '--docker', # using docker containers
                      '--push-docker-deps', # push containers if possible
                      '--debug', # print debug output
                      '--create-final-image', # assume KLEE is the final image
                      ]


                process = subprocess.Popen(cmd, # Assume KLEE is the final image
                                stdout=subprocess.PIPE,
                                universal_newlines=True,
                                env = build_env)

                while True:
                    output = process.stdout.readline()
                    print(output.strip())
                    return_code = process.poll()
                    if return_code != None:
                        break
                if return_code != 0:
                    print('Building image failed: {}'.format(return_code))
                    for output in process.stdout.readlines():
                        print(output.strip())
                    sys.exit(1)

    except yaml.YAMLError as exc:
        print(exc)
