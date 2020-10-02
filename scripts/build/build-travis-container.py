#!/usr/bin/env python3

import yaml
import subprocess
import os

(abs_path, _) = os.path.split(os.path.abspath(__file__))

with open(os.path.join(abs_path,"../../.travis.yml"), 'r') as stream:
    try:
        travis_config = yaml.safe_load(stream)
        global_env = travis_config['env']['global']
        for job in travis_config['jobs']['include']:
            if job['name'] in ["Docker", "macOS"]:
                print("Skip: {}".format(job['name']))
                continue
            print("Building: {}".format(job['name']))


            # Copy current environment
            build_env = os.environ.copy()

            build_vars = {}
            # Setup global build variables
            for e in global_env:
                (k,v) = e.split("=")
                build_vars[k] = v

            # Override with job specific values
            for e in job['env'].strip("").split(" "):
                (k,v) = e.split("=")
                build_vars[k] = v

            for k,v in build_vars.items():
                build_env[k] = v

            cmd = [os.path.join(abs_path, 'build.sh'),
                  'klee', # build KLEE and all its dependencies
                  '--docker', # using docker containers
                  '--push-docker-deps', # push containers if possible
                  '--debug', # print debug output
                  '--create-final-image', # assume KLEE is the final image
                  ]

            env_str = [k+"="+v for (k,v) in build_vars.items()]
            print("{} {}".format(" ".join(env_str)," ".join(cmd)) )

            process = subprocess.Popen(cmd, # Assume KLEE is the final image
                            stdout=subprocess.PIPE,
                            universal_newlines=True,
                            env = build_env)

            while True:
                output = process.stdout.readline()
                print(output.strip())
                return_code = process.poll()
                if return_code is not None:
                    print('Building image failed: {}'.format(return_code))
                    for output in process.stdout.readlines():
                        print(output.strip())
                    break

    except yaml.YAMLError as exc:
        print(exc)