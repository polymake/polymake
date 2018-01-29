from distutils.core import setup, Extension
import sys
import subprocess

import os

if sys.version_info < (3,5):
    macro_list = [ ( "PYTHON_VERSION_OLDER_THREE_FIVE", "1" ) ]
else:
    macro_list = [ ]

if sys.version_info < (3,0):
    FileNotFoundError = OSError

def conditional_decode( string ):
  if sys.version_info < (3,0):
      return string
  return string.decode( 'utf-8' )

polymake_cflags = conditional_decode( subprocess.check_output( [ "polymake-config", "--cflags" ] ).strip() ).split(' ')
polymake_cflags += conditional_decode( subprocess.check_output( [ "polymake-config", "--includes" ] ).strip() ).split(' ')
polymake_ldflags = conditional_decode( subprocess.check_output( [ "polymake-config", "--ldflags" ] ).strip() ).split(' ')
polymake_ldflags += [ "-lpolymake" ]

polymake_cc = conditional_decode( subprocess.check_output( [ "polymake-config", "--cc" ] ).strip() )
os.environ["CC"] = polymake_cc
os.environ["CXX"] = polymake_cc

setup(
    name = 'JuPyMake',
    version = '0.8',
    description = 'A simple interface to Polymake',
    author = 'Sebastian Gutsche',
    author_email = 'sebastian.gutsche@gmail.com',
    url = 'https://github.com/sebasguts/JuPyMake',
    ext_modules= [ Extension( "JuPyMake",
                              [ "JuPyMake.cpp" ],
                              extra_compile_args=polymake_cflags,
                              extra_link_args=polymake_ldflags,
                              define_macros = macro_list ) ],
    package_data = { '': [ "COPYING", "GPLv2" ] },
)
