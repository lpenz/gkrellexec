
import os

gtk2include = os.popen('pkg-config gtk+-2.0 --cflags').read().split(' ')[:-1]
gtk2lib = os.popen('pkg-config gtk+-2.0 --libs').read().split(' ')[:-1]

env = Environment()

env.SharedLibrary('gkrellexec.c', CFLAGS = ['-O2', '-Wall', '-Werror', gtk2include], LINKFLAGS=gtk2lib)
env.Command('gkrellexec.so', 'libgkrellexec.so', 'cp $SOURCE $TARGET')

env.Command('README.textile', 'gkrellexec.t2t', 'txt2tags -t textile -H -i $SOURCE -o $TARGET')

