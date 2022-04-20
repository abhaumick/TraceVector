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
traceSize = 100
warpsPerTB = 10

minWarpSize = 10
maxWarpSize = 100

def getSizes(count : int) -> list :
    sizes = numpy.random.randint(minWarpSize, maxWarpSize, count)
    return(sizes)

def encodeDummyTrace(sizes : list) -> str:
    traceList = []
    numTB = int(len(sizes) / warpsPerTB)
    
    # File Header
    traceList.append('-kernel name = dummy\n')
    traceList.append('-kernel id = 1\n')
    traceList.append('-grid dim = (1,4096,1)\n')
    traceList.append('\n')

    traceList.append("#traces format ="
        " threadblock_x threadblock_y threadblock_z"
        " warpid_tb PC mask dest_num [reg_dests] opcode"
        " src_num [reg_srcs] mem_width [adrrescompress?] [mem_addresses]\n")
    traceList.append('\n')


    for tbIdx in range(0, numTB):
        traceList.append('#BEGIN_TB {} \n'.format(tbIdx, sizes[tbIdx]))
        traceList.append('\n')
        traceList.append('thread block = 0,{},0 \n'.format(tbIdx))
        traceList.append('\n')
        for warpIdx in range(0, warpsPerTB) :
            traceIdx = tbIdx * warpsPerTB + warpIdx
            traceSize = sizes[traceIdx]
            traceList.append('warp = {}\n'.format(warpIdx))
            traceList.append('insts = {}\n'.format(traceSize))
            pc = 0
            for idx in range(0, traceSize) :
                traceList.append('{:04x} ffffffff 1 R0 instruction {}, {}, {} \n' \
                    .format(pc, tbIdx, warpIdx, idx))
                pc = pc + 0x10
            traceList.append('\n')
        traceList.append('#END_TB {}\n'.format(tbIdx))
        traceList.append('\n')

    traceString = ''.join(traceList)
    return traceString

if __name__ == '__main__' :
    sizes = getSizes(traceSize)
    retString = encodeDummyTrace(sizes)
    with open(filePath, 'wt', newline='') as traceFile :
        traceFile.write(retString)
        print('Wrote {} bytes to {}'.format(len(retString), filePath))

    print('Done')
