# Copyright (c) 2010 Heinrich Fink <hf (at) hfink (dot) eu>, 
#                    Thomas Weber <weber (dot) t (at) gmx (dot) at>
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

import sys
import glob
import os
import subprocess
import datetime

import os.path
from optparse import OptionParser

# This script currently assume that it resides in the
# root folder and that COLLADA files are in assets_collada
# It also assume that ColladaBakery.exe is in the root folder

# Call ColladaBakery for all files in assets_collada dir
input_files = glob.glob('assets_collada/*.dae')

cwd = os.getcwd()

log_file = open('bakery_log.txt','w')

log_file.write('Bakery Log file\n')
log_file.write('===============\n')
log_file.write('\n')
now = datetime.datetime.now()
log_file.write(now.strftime('%Y-%m-%d %H:%M')+'\n')
log_file.write('\n')
log_file.write('Batch baking:')
log_file.write('\n')

try:
    for f in input_files:

        log_file.write('\n++++++++++++ ')
        log_file.write('Baking ' + f)
        log_file.write(' ++++++++++++\n\n')

        log_file.write('++ COLLADA Baking ++\n')
        output = subprocess.check_output(["ColladaBakery.exe",
                                          "--input="+f]);
        print(output)

        log_file.write('\n' + output + '\n')
        
        basename = os.path.basename(f)
        rtrfile = basename[0:-3] + "rtr"
        # Call the material bakery
        log_file.write('++ Material Baking ++\n')
        output = subprocess.check_output(["python",
                                     "material_bakery.py",
                                      "-o",
                                      "assets_baked/" + rtrfile,
                                      "materials/material_lib.xml"])

        print(output)
        log_file.write('\n' + output + '\n')
        
except Exception, e:
    log_file.write('! Exception was encountered: ' + str(e))
    
finally:
    log_file.write('\nExiting batch mode.')
    log_file.close()
                    
raw_input("Hit enter to exit.")

