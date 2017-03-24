from ipykernel.kernelapp import IPKernelApp
from .kernel import polymakeKernel
IPKernelApp.launch_instance(kernel_class=polymakeKernel)
