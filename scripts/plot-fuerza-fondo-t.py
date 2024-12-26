#!/usr/bin/python3

import numpy as np
import argparse
from scipy.stats import linregress
import matplotlib.pyplot as plt
from math import log, exp
import glob

files = glob.glob('force_contact_*.dat')
files.sort()
print(len(files))

for file in files:
    fID = int(file.split('_')[-1][:-4])
    x, y, fn = np.loadtxt(file, usecols=(2, 3, 4), unpack=True)
    mask = np.abs(y) < 0.01
    print(fID, fn[mask].sum())



