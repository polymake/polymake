from ipykernel.kernelbase import Kernel
import pexpect

from subprocess import check_output
from os import unlink, path

import base64
import imghdr
import re
import signal
import urllib

import sys

from ipykernel.comm import Comm
from ipykernel.comm import CommManager

import JuPyMake

kernel_object_for_ipython = None

def _mock_get_ipython():
    global kernel_object_for_ipython
    return kernel_object_for_ipython

try:
    import IPython
    ipython_loaded = True
except ImportError:
    ipython_loaded = False

if ipython_loaded:
    ## Rewrite this incredibly stupid get_ipython method
    get_ipython = _mock_get_ipython
    sys.modules['IPython'].get_ipython = _mock_get_ipython
    sys.modules['IPython'].core.getipython.get_ipython = _mock_get_ipython

try:
    from ipywidgets import *
    ipywidgets_extension_loaded = True
except ImportError:
    ipywidgets_extension_loaded = False

class own_ipython:
    kernel = None
    def __init__(self, kernel = None ):
        self.kernel = kernel

__version__ = '0.3'

version_pat = re.compile(r'version (\d+(\.\d+)+)')

class PolymakeRunException(Exception):
    pass

class polymakeKernel(Kernel):
    implementation = 'jupyter_polymake_wrapper'
    implementation_version = __version__

    help_links = [ { 'text': "Polymake website", 'url': "http://polymake.org/" },
                   { 'text': "Polymake documentation", 'url': "https://polymake.org/doku.php/documentation" },
                   { 'text': "Polymake tutorial", 'url': "https://polymake.org/doku.php/tutorial/start" },
                   { 'text': "Polymake reference", 'url': "https://polymake.org/release_docs/3.0/" } ]

    def _replace_get_ipython(self):
        new_kernel = own_ipython(self)
        global kernel_object_for_ipython
        kernel_object_for_ipython = new_kernel

    @property
    def language_version(self):
        m = version_pat.search(self.banner)
        return m.group(1)

    _banner = None

    @property
    def banner(self):
        if self._banner is None:
            self._banner = "Jupyter kernel for polymake"
        return self._banner

    language_info = {'name': 'polymake',
                     'codemirror_mode': 'perl', #
                     'mimetype': 'text/x-polymake', 
                     'file_extension': '.pl'} # FIXME: Is this even real?

    def __init__(self, **kwargs):
        Kernel.__init__(self, **kwargs)
        self._replace_get_ipython()
        self.comm_manager = CommManager(shell=None, parent=self,
                                        kernel=self)
        self.shell_handlers['comm_open'] = self.comm_manager.comm_open
        self.shell_handlers['comm_msg'] = self.comm_manager.comm_msg
        self.shell_handlers['comm_close'] = self.comm_manager.comm_close
        if ipywidgets_extension_loaded:
            self.comm_manager.register_target('ipython.widget', Widget.handle_comm_opened)
        self._start_polymake()

    def _start_polymake(self):
        JuPyMake.InitializePolymake()
        try:
            self._run_polymake_command( 'include "common::jupyter.rules";' )
        except PolymakeRunException:
            return
        return

    def _run_polymake_command( self, code ):
        try:
            output = JuPyMake.ExecuteCommand( code.strip()+"\n" )
        except Exception as exception:
            raise PolymakeRunException(exception.args[0])
        return output
    
    def _process_python( self, code ):
        if code.find( "@python" ) == -1 and code.find( "@widget" ) == -1:
            return False
        exec(code[7:],globals(),locals())
        return True

    def do_execute(self, code, silent, store_history=True,
                   user_expressions=None, allow_stdin=False):
        
        default_return = {'status': 'ok', 'execution_count': self.execution_count,
                          'payload': [], 'user_expressions': {}}
        
        if not code.strip():
            return default_return
        
        if self._process_python( code ):
            return default_return
        
        interrupted = False
        code = code.rstrip()
        
        try:
            output = self._run_polymake_command( code )
        except KeyboardInterrupt:
            self._run_polymake_command( '' )
            interrupted = True
        except PolymakeRunException as exception:
            output = exception.args[0]
            stream_content = {'execution_count': self.execution_count, 'data': { 'text/plain': "Error: Incomplete Statement:\n" + code } }
            self.send_response( self.iopub_socket, 'execute_result', stream_content )
            return {'status': 'error', 'execution_count': self.execution_count,
                    'ename': 'PolymakeRunException', 'evalue': output, 'traceback': []}
        if not silent:
            if output[0] == True:
                if output[1] != "":
                    output_stdout = output[1]
                    while output_stdout.find( '.@@HTML@@' ) != -1:
                        html_position = output_stdout.find( '.@@HTML@@' )
                        html_end_position = output_stdout.find( '.@@ENDHTML@@' )
                        if html_position > 0:
                            before_html = output_stdout[:html_position].rstrip()
                        else:
                            before_html = ''
                        output_html = output_stdout[html_position+9:html_end_position-1].strip().rstrip()
                        output_stdout = output_stdout[html_end_position+12:].strip()
                        if before_html != '':
                            stream_content = {'execution_count': self.execution_count, 'data': { 'text/plain': before_html } }
                            self.send_response( self.iopub_socket, 'execute_result', stream_content )
                        stream_content = {'execution_count': self.execution_count,
                                          'source' : "polymake",
                                          #'data': { 'text/html': "Sorry, threejs visualization is currently not available"},
                                          'data': { 'text/html': output_html},
                                          'metadata': dict() }
                        self.send_response( self.iopub_socket, 'display_data', stream_content )
                    if len(output_stdout) != 0:
                        stream_content = {'execution_count': self.execution_count, 'data': { 'text/plain': output_stdout } }
                        self.send_response( self.iopub_socket, 'execute_result', stream_content )
                if output[2] != "":
                    output_html = "<details><summary><pre style=\"display:inline\"><small>Click here for additional output</small></pre></summary>\n<pre>\n"+output[2]+"</pre>\n</details>\n"
                    stream_content = {'execution_count': self.execution_count,
                                      'source' : "polymake",
                                      'data': { 'text/html': output_html},
                                      'metadata': dict() }
                    self.send_response( self.iopub_socket, 'display_data', stream_content )
                if output[3] != "":
                    stream_content = {'execution_count': self.execution_count, 'data': { 'text/plain': output[3] } }
                    self.send_response( self.iopub_socket, 'execute_result', stream_content )
                    return {'status': 'error', 'execution_count': self.execution_count,
                            'ename': 'PolymakeRunException', 'evalue': output, 'traceback': []}
            elif output[0] == False:
                if output[3] == "":
                    stream_content = {'execution_count': self.execution_count, 'data': { 'text/plain': "Error: Incomplete Statement:\n" + code } }
                    self.send_response( self.iopub_socket, 'execute_result', stream_content )
                    return {'status': 'error', 'execution_count': self.execution_count,
                            'ename': 'IncompleteStatementError', 'evalue': output, 'traceback': []}
                else:
                    stream_content = {'execution_count': self.execution_count, 'data': { 'text/plain': output[3] } }
                    self.send_response( self.iopub_socket, 'execute_result', stream_content )
                    return {'status': 'error', 'execution_count': self.execution_count,
                            'ename': 'PolymakeRunException', 'evalue': output, 'traceback': []}
        if interrupted:
            return {'status': 'abort', 'execution_count': self.execution_count}

        return {'status': 'ok', 'execution_count': self.execution_count,
                'payload': [], 'user_expressions': {}}

    def do_shutdown(self, restart):
        if restart:
            self._start_polymake()
    
    ## Temporary method to determine offset of completion. Will be replaced soon.
    def get_completion_length( self, code, completion ):
      code_length = len(code)
      maximal_length = 0
      for i in range(1,code_length+1):
          if code[code_length-i:code_length] == completion[0:i]:
              maximal_length = i
      return maximal_length
    
    def do_complete(self, code, cursor_pos):
        try:
            completions = JuPyMake.GetCompletion(code[0:cursor_pos])
        except:
            completions = (0,"",[])
        completion_offset = completions[0]
        cur_start = cursor_pos - completion_offset
        return {'matches': completions[2], 'cursor_start': cur_start,
                'cursor_end': cursor_pos, 'metadata': dict(),
                'status': 'ok'}

    def do_is_complete( self, code ):
        new_code = 'if(0){ ' + code + ' }'
        try:
            output = self._run_polymake_command( new_code )
        except PolymakeRunException:
            return {'status' : 'incomplete', 'indent': '' }
        if output[0] == False:
            return {'status' : 'incomplete', 'indent': '' }
        return {'status' : 'complete' }

    def do_inspect( self, code, cursor_pos, detail_level=0 ):
        print(detail_level)
        ## ignore detail_level for now
        full = True
        try:
            output = JuPyMake.GetContextHelp( input=code, position=cursor_pos, full=full )
        except PolymakeRunException:
            output = []
        try:
            output_html = JuPyMake.GetContextHelp( input=code, position=cursor_pos, full=full, html=True )
        except PolymakeRunException:
            output_html = []
        output_data = { }
        if output != []:
            output_data['text/plain'] = "\n".join(output)
        if output_html != []:
            output_data['text/html'] = "\n".join(output_html)
        return {'status': 'ok', 'data': output_data, 'metadata': {}, 'found': True}
