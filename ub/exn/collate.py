#!/usr/bin/env python3
import sys

def stddev(l):
    l = [i for i in map(float, l)]
    mean = sum(l) / len(l) 
    variance = sum([((x - mean) ** 2) for x in l]) / len(l) 
    res = variance ** 0.5
    return mean, res, variance

exn = []
noexn = []
with open(sys.argv[1], "r") as f:
    for l in f:
        l = l.strip()
        if l.startswith('exn'):
            exn.append(l.split(',')[-1])
        elif l.startswith('noexn'):
            noexn.append(l.split(',')[-1])


m, d, v = stddev(exn)
print("exn   m={} d={} v={}".format(m,d,v))

m, d, v = stddev(noexn)
print("noexn m={} d={} v={}".format(m,d,v))
