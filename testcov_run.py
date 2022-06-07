#!/usr/bin/env python3

import os
import glob
import re
import argparse
import subprocess
import zipfile
from yaml import load
try:
    from yaml import CLoader as Loader
except ImportError:
    from yaml import Loader

parser = argparse.ArgumentParser()
parser.add_argument('-o', help="Имя выходного файла")
parser.add_argument('-r', help="Путь к директории с результатами")
parser.add_argument('-b', help="Путь к директории с тестами, тесты могут лежать в поддиректориях")
parser.add_argument('-t', help="Путь к testcov")
args = parser.parse_args()

resu = open(args['o'], "w+")
res = os.path.abspath(os.getcwd() + args['r'])
benchs = os.path.abspath(os.getcwd() + args['b'])
testtool = os.path.abspath(os.getcwd() + args['t'])
files = [f for f in os.listdir(res) if re.match(r'[a-zA-Z0-9.-_]*.zip', f)]
for file in files:
    file_chop = file[:-4]
    cor = glob.glob(benchs + f"/**/{file_chop}", recursive=True)[0]
    with open(cor,'r') as stream:
        data = load(stream, Loader=Loader)
        fl = data['input_files']
        ag = glob.glob(benchs + f"/**/{fl}", recursive=True)[0]
        resu.write(f"{fl}\n")
        sp = subprocess.Popen(f'{args["t"]} --test-suite "{res}{file}" "{ag}"', shell=True, stdout=subprocess.PIPE)
        resu.write(f"{sp.stdout.read()}\n")
        resu.write("\n")
resu.close()
