# jupyter kernel polymake
###Jupyter kernel for polymake 

This is the current beta version of a jupyter kernel for polymake. It uses the `ipykernel` and
`pexpect` python module, along with some sugar.

## Requirements

To use this kernel, you need python3 with jupyter installed.
To get further information about installing it, please have a look at [python](https://www.python.org)
and [Jupyter](http://jupyter.org). For most Linux and OS X versions, all the packages needed are
accessible via `pip`. For the usage of interactive widgets you also need the `ipywidgets` python module
installed.

Also, the latest version of [polymake](http://polymake.org) is required.

## polymake-kernel

To install the kernel for your Jupyter, clone this repository and install it via the following commands,
where the first might be need super user rights, depending on your python installation. The second should
be executed as the user who wants to use jupyter, e.g., your normal user.

```shell
    python setup.py install
```

If one of those commands fail, please make sure you have the proper rights. Also, check whether your
python is really python3. If not, python3 might work for you, but if you want to start Jupyter via the commands
below, you need to alias python3 for python

To use it, use one of the following:

```shell
    jupyter notebook
    jupyter qtconsole --kernel polymake
    jupyter console --kernel polymake
```

Note that this kernel assumes that `polymake` is in the `PATH`.

## Examples

Please have a look at the Demo worksheet.
