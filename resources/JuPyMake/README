# JuPyMake
## A basic interface to call polymake from python

This python module is meant to be used in polymakes Jupyter interface.

## Install

Install via
```
python setup.py install
```
Currently, the `polymake-config` command must be executable.

## Example

```
>>> import JuPyMake
>>> JuPyMake.InitializePolymake()
True
>>> JuPyMake.ExecuteCommand("$p=cube(3);")
(True, '', '')
>>> JuPyMake.ExecuteCommand("print $p->VERTICES;")
(True, '1 -1 -1 -1\n1 1 -1 -1\n1 -1 1 -1\n1 1 1 -1\n1 -1 -1 1\n1 1 -1 1\n1 -1 1 1\n1 1 1 1\n', '', '')
>>> JuPyMake.GetCompletion("cube");
(4, '<', ['cube'])
>>> JuPyMake.GetCompletion("cu");
(2, '<', ['cube', 'cuboctahedron'])
>>> JuPyMake.GetContextHelp(input="cube")
['cube<Scalar>(d; x_up, x_low, Options) -> Polytope<Scalar>\n\nOptions: group character_table\n\n']
>>> JuPyMake.GetContextHelp(input="cube",full=True);
... output omitted ...
```

## Notes

Please note that this is alpha work at the moment.
