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

import os
import platform

env = Environment(ENV = {'PATH' : os.environ['PATH']})

# Get configuration from command line. Can be either 'debug' or 'release'
# 'debug' is the default
config = ARGUMENTS.get('config', 'Release')

# Common settings for all build products

env['CPPPATH'] = ['#/build/src_generated',
                  '#/external/glm/include',
                  '#/common']

protoc_binary = 'protoc'

python_exe = 'python'

if platform.system() == 'Linux':
    env['CCFLAGS'] = '-Wall -Wextra -Wno-unused-parameter -Wno-unused-function '
    if config == 'Debug':
        env.Append(CCFLAGS = '-ggdb -O0')
    else:
        env.Append(CCFLAGS = '-O2')

    env['LIBS'] = ['boost_regex', 
                   'protobuf',
                   'kyotocabinet',
                   'z']

    python_exe = 'python2'

elif platform.system() == 'Windows':
    env['ENV']['TMP'] = os.environ['TMP']
    env['LIBPATH'] = ['#/external/GLFW/lib/%s' % config]
    env.Append(LIBPATH = '#/external/protocol_buffers/lib/%s' % config)
    env.Append(LIBPATH = '#/external/kyoto_cabinet/lib/%s' % config)
    env.Append(LIBPATH = '#/external/zlib/lib/%s' % config)
    env.Append(LIBPATH = '#/external/boost/lib/static/%s' % config)

    env.Append(CPPPATH = '#/external/protocol_buffers/include')
    env.Append(CPPPATH = '#/external/boost/include')
    env.Append(CPPPATH = '#/external/kyoto_cabinet/include')
    
    env['LIBS'] = ['libprotobuf-lite.lib',
                   'kernel32.lib',
                   'user32.lib',
                   'gdi32.lib',
                   'winspool.lib',
                   'comdlg32.lib',
                   'advapi32.lib',
                   'shell32.lib',
                   'ole32.lib',
                   'oleaut32.lib',
                   'uuid.lib',
                   'odbc32.lib',
                   'odbccp32.lib',
                   'kyotocabinet.lib',
                   'zlibstat.lib']

    protoc_binary = '..\\external\\protocol_buffers\\bin\\protoc.exe'

    if config == 'Debug':

        env.Append(CCFLAGS = '/ZI /nologo /W3 /WX- /Od /Oy- /D WIN32 /D ' +
                             '_DEBUG /D _CONSOLE ' +
                             '/D ENABLE_WIN_MEMORY_LEAK_DETECTION ' +
                             '/D _CRT_SECURE_NO_WARNINGS /Gm /EHsc /RTC1 ' + 
                             '/MDd /GS /fp:precise /Zc:wchar_t /Zc:forScope')

        env.Append(LINKFLAGS = '/INCREMENTAL /NOLOGO /ALLOWISOLATION /DEBUG '+
                               '/TLBID:1 /DYNAMICBASE /NXCOMPAT /MACHINE:X86 '+
                               '/ERRORREPORT:QUEUE')
        
    else:

        env.Append(CCFLAGS = '/Zi /nologo /W3 /WX- /O2 /Oi /Oy- /GL /D WIN32 '+
                             '/D NDEBUG /D _CONSOLE ' +
                             '/D _CRT_SECURE_NO_WARNINGS /Gm- /EHsc /MD /GS ' +
                             '/Gy /fp:precise /Zc:wchar_t /Zc:forScope')

        env.Append(LINKFLAGS = '/INCREMENTAL:NO /NOLOGO /SUBSYSTEM:CONSOLE ' + 
                               '/OPT:REF /OPT:ICF /LTCG /TLBID:1 ' +
                               '/DYNAMICBASE /NXCOMPAT /MACHINE:X86')
else:
    raise Scons.Errors.UserError('Your operating system is not supported (yet?)')

env.Command(['#/build/src_generated/rtr_format.pb.cc',
             '#/build/src_generated/rtr_format.pb.h',
             '#/build/src_generated/rtr_format.py'],
             '#/common/rtr_format.proto',
             '%s --cpp_out=../build/src_generated rtr_format.proto\
              --python_out=../build/src_generated' % 
             protoc_binary,
             chdir='common')

env.SideEffect(['#/build/src_gen_lock'], 
               ['#/build/src_generated/rtr_format.pb.cc',
                '#/build/src_generated/rtr_format.pb.h',
                '#/build/src_generated/rtr_format.py'])

Export('env')
Export('config')
Export('python_exe')

SConscript(['player/SConscript',
            'collada_bakery/SConscript'])
