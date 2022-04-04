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
warpsPerTB = 10

def getSizes(count : int) -> list :
    sizes = numpy.random.randint(10, 50, count)
    return(sizes)

def encodeDummyTrace(sizes : list) -> str:
    traceList = []
    for tbIdx in range(0, int(len(sizes) / warpsPerTB)):
        traceList.append('#BEGIN_TB {} \n'.format(tbIdx, sizes[tbIdx]))
        traceList.append('thread block = 2,3,4 \n')
        for warpIdx in range(0, warpsPerTB) :
            traceIdx = tbIdx * warpsPerTB + warpIdx
            traceSize = sizes[traceIdx]
            traceList.append('warp {}\n'.format(warpIdx))
            traceList.append('insts = {}\n'.format(traceSize))
            for idx in range(0, traceSize) :
                traceList.append(' Line {},{} {} \n'.format(tbIdx, warpIdx, idx))
        traceList.append('#END_TB {}\n'.format(tbIdx))

    traceString = ''.join(traceList)
    return traceString

if __name__ == '__main__' :
    sizes = getSizes(100)
    retString = encodeDummyTrace(sizes)
    with open(filePath, 'wt') as traceFile :
        traceFile.write(retString)
        print('Wrote {} bytes to {}'.format(len(retString), filePath))

    print('Done')
