#!/usr/bin/env python
from __future__ import print_function
from subprocess import Popen, PIPE
import sys, re

"""
1. given $progname
1. find the labels where a stack writes occur in ${progname}.1.c
2. run the given program ($progname)
  2a. store list of labels as they appear (indicates execution order)
  2b. for each label found, store subsequent instructions
3. starting with last label, trace backwards and find all labels
   where a stack write occurred, printing out in reverse order
"""


def find_stack_writes(pn, lookfor="S\(.*, 16\) ="):
    ln = 0
    fn = "{}.1.c".format(pn)

    print("Looking for {} in {}".format(lookfor, fn))

    LM = {}

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
                        LM[ol] = nl
                        print(ol, nl)
                        break

    return LM

def run_prog(pn, swrites):
    print("running {}".format(pn))
    p = Popen(pn, shell=False, stderr=PIPE)
    out = p.stderr.read().strip().decode('utf-8')
    for line in out.split("\n")[::-1]:
        m = re.search("(L_[0-9]+:)", line)
        if m:
            label = m.group(0)
            print(line, end='  ')
            if label in swrites:
                print("<-- {}".format(swrites[label]))
            else:
                print("")



def main():
    pn = sys.argv[1]
    swrites = find_stack_writes(pn)
    run_prog(pn, swrites)

if __name__ == "__main__":
    main()
