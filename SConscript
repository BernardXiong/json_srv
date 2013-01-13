from building import *

cwd     = GetCurrentDir()
src	= Glob('*.c')
CPPPATH = [cwd, str(Dir('#'))]

group = DefineGroup('NetApps', src, depend = ['RT_USING_LWIP'], CPPPATH = CPPPATH)

Return('group')
