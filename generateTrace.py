#
# @file generateTrace.py
# @author Abhishek Bhaumick (abhaumic@purdue.edu)
# @brief 
# @version 0.1
# @date 2022-03-31
# 
# 

import os
import numpy

filePath = 'trace.log'

def getSizes(count : int) -> list :
    sizes = numpy.random.randint(10, 100, count)
    return(sizes)

def encodeDummyTrace(sizes : list) -> str:
    traceList = []
    for traceIdx in range(0, len(sizes)):
        traceList.append('# Trace {} with {} records \n'.format(traceIdx, sizes[traceIdx]))
        for idx in range(0, sizes[traceIdx]) :
            traceList.append('- Line {}, {} \n'.format(traceIdx, idx))

    traceString = ''.join(traceList)
    return traceString

if __name__ == '__main__' :
    sizes = getSizes(10)
    retString = encodeDummyTrace(sizes)
    with open(filePath, 'wt') as traceFile :
        traceFile.write(retString)
        print('Wrote {} bytes to {}'.format(len(retString), filePath))

    print('Done')
