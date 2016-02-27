#!/usr/bin/python

# ===-- Animate.py --------------------------------------------------------===##
# 
#                      The KLEE Symbolic Virtual Machine
# 
#  This file is distributed under the University of Illinois Open Source
#  License. See LICENSE.TXT for details.
# 
# ===----------------------------------------------------------------------===##

import os
import TreeGraph
import Image

def main():
    from optparse import OptionParser
    op = OptionParser("usage: %prog [options] <tree-stream-path> <output-directory>")
    op.add_option('','--start', dest='startCount', type=int, default=10,
                  help='number of paths to start animation at')
    op.add_option('','--end', dest='endCount', type=int, default=1000,
                  help='number of paths to start animation at')
    op.add_option('','--stride', dest='countStride', type=int, default=10,
                  help='number of paths to step by in each frame')
    op.add_option('','--convert-to-jpg', dest='convertToJPG',
                  action='store_true', default=False)
    op.add_option('','--convert-to-rgb', dest='convertToRGB',
                  action='store_true', default=False)
    opts,args = op.parse_args()

    if len(args) != 2:
        parser.error('invalid number of arguments')

    symPath,outputDir = args
    if not os.path.exists(outputDir):
        os.mkdir(outputDir)
    
    for frame,count in enumerate(range(opts.startCount, opts.endCount,
                                     opts.countStride)):
        print 'generating frame %d with path count %d...' % (frame+1, count)
        pdf_path = os.path.join(outputDir, 'frame_%05d.pdf' % frame)
        TreeGraph.makeTreeGraph(pdf_path, symPath, count)
        if not opts.convertToJPG:
            continue

        jpg_path = os.path.join(outputDir, 'frame_%05d.jpg' % frame)
        if not opts.convertToRGB:
            os.system('convert "%s" "%s"' % (pdf_path, jpg_path))
            continue

        jpg_tmp_path = os.path.join(outputDir, 'frame_%05d_tmp.jpg' % frame)
        os.system('convert "%s" "%s"' % (pdf_path, jpg_tmp_path))

        img = Image.open(jpg_tmp_path)
        img = img.convert('RGB')
        img.save(jpg_path, quality=100)
        
if __name__=='__main__':
    try:
        main()
    except:
        import sys,traceback
        traceback.print_exc(file= sys.__stdout__)
        sys.__stdout__.flush()
