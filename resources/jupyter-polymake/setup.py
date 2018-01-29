#!/usr/bin/env python3.5

import json
import os
import sys
from distutils.core import setup
import sys
from jupyter_client.kernelspec import install_kernel_spec
from IPython.utils.tempdir import TemporaryDirectory
from os.path import dirname,abspath
from shutil import copy as file_copy

kernel_json = {"argv":[sys.executable,"-m","jupyter_kernel_polymake", "-f", "{connection_file}"],
 "display_name":"polymake",
 "language":"polymake",
 "codemirror_mode":"perl", # note that this does not exist yet
 "env":{"PS1": "$"}
}

def install_my_kernel_spec(user=True):
    with TemporaryDirectory() as td:
        os.chmod(td, 0o755) # Starts off as 700, not user readable
        with open(os.path.join(td, 'kernel.json'), 'w') as f:
            json.dump(kernel_json, f, sort_keys=True)
        path_of_file = dirname( abspath(__file__) ) + "/jupyter_kernel_polymake/resources/"
        filenames=[ "three.js", "Detector.js", "controls/TrackballControls.js", "renderers/SVGRenderer.js", "renderers/CanvasRenderer.js", "renderers/Projector.js", "menu.svg", "close.svg" ]
        for i in filenames:
            file_copy(path_of_file + i, td )
        file_copy(path_of_file + "kernel.js", td )
        file_copy(path_of_file + "logo-32x32.png", td )
        file_copy(path_of_file + "logo-64x64.png", td )
        print('Installing jupyter kernel spec for polymake')
        install_kernel_spec(td, 'polymake', user=user, replace=True)

def main(argv=None):
    install_my_kernel_spec()

if __name__ == '__main__':
    main()



setup( name="jupyter_kernel_polymake"
     , version="0.16"
     , description="A Jupyter kernel for polymake"
     , author="Sebastian Gutsche"
     , author_email="sebastian.gutsche@gmail.com"
     , url="https://github.com/sebasguts/jupyter-polymake"
     , packages=["jupyter_kernel_polymake"]
     , package_dir={"jupyter_kernel_polymake": "jupyter_kernel_polymake"}
     , package_data={"jupyter_kernel_polymake": ["resources/logo-32x32.png",
                                                 "resources/logo-64x64.png",
                                                 "resources/kernel.js",
                                                 "resources/three.js",
                                                 "resources/Detector.js",
                                                 "resources/controls/TrackballControls.js",
                                                 "resources/renderers/SVGRenderer.js",
                                                 "resources/renderers/CanvasRenderer.js",
                                                 "resources/renderers/Projector.js",
                                                 "resources/renderers/menu.svg",
                                                 "resources/renderers/close.svg" ]}
     ,
     )
