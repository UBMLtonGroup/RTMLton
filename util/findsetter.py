#!/usr/bin/env python
from __future__ import print_function
import re

import sys

def lwl(n, l):
    return """  fprintf(stderr, "[{} {}]\\n");  """.format(n, l.strip())

def line(n, l):
    l = re.sub(r"[\\\"]", " ", l.strip())
    return """  fprintf(stderr, "[{}] {}\\n");  """.format(n, l)

ln = 0
fn = sys.argv[1]
lookfor = sys.argv[2]
print("Looking for {} in {}".format(lookfor, fn))

with open(fn, "r") as f:
    for l in f:
        l.strip()
        ln += 1
        if "ChunkSwitch" in l:
            break

    for l in f:
        ln += 1
        l = l.strip()
        if l.startswith("L_"): # or 'case ' in l or 'default:' in l:
            ol = l
            for l in f:
                l = l.strip()
                if re.search(lookfor, l) is not None:
                    nl = re.sub(r'\/\*.*\*\/', "", l)
                    print(ol, nl)
                    break
