#!/usr/bin/env python3
import sys

def stddev(l):
    l = [i for i in map(float, l)]
    mean = sum(l) / len(l) 
    variance = sum([((x - mean) ** 2) for x in l]) / len(l) 
    res = variance ** 0.5
    return mean, res, variance

def s2float(s):
    if '~' in s:
        s = s.replace('~', '-')
    return float(s)

def v(x):
    if x < 0.01:
        return f"{x:0.2e}"
    return f"{x:0.2f}"

mlton = {}
rtmlton = {}

with open(sys.argv[1], "r") as f:
    for l in f:
        l = l.strip()
        f = l.split(',')
        print(l,f)
        if f[1] not in mlton:
           mlton[f[1]] = [s2float(f[2])]
           rtmlton[f[1]] = [s2float(f[3])]
        else:
           mlton[f[1]].append(s2float(f[2]))
           rtmlton[f[1]].append(s2float(f[3]))
 

print("          & MLton           & RTMLton \\\\")
print("Test Name & Median & Stddev & Median & Stddev & Slow down \\\\")
for i in mlton.keys():
    m0,d0,v0 = stddev(mlton[i])
    m1,d1,v1 = stddev(rtmlton[i])
    factor = m1/m0
    #print(f"{i} & {v(m0)} & {v(d0)} & {v(m1)} & {v(d1)} & {v(factor)}")
    print(f"{i} & {v(m0)} &  {v(m1)} &  {v(factor)} \\\\")

