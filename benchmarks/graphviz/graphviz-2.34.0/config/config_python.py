import sys
from distutils import sysconfig

if sys.argv[1] == "archlib":
    print sysconfig.get_python_lib(1,1)
elif sys.argv[1] == "lib":
    print sysconfig.get_python_lib(0,1)
elif sys.argv[1] == "archsitelib":
    print sysconfig.get_python_lib(1,0)
elif sys.argv[1] == "sitelib":
    print sysconfig.get_python_lib(0,0)

