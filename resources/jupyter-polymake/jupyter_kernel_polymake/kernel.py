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
        sig = signal.signal(signal.SIGINT, signal.SIG_DFL)
        try:
            polymake_run_command = pexpect.which( "polymake" )
            #___replace_polymake_run_command___
            self.polymakewrapper = pexpect.spawnu( polymake_run_command + " -" )
            # set jupyter enviroment in polymake
            try:
                self._run_polymake_command( 'prefer "threejs";' )
                self._run_polymake_command( 'include "common::jupyter.rules";' )
                self._run_polymake_command( '$common::is_used_in_jupyter = 1;' )
            except PolymakeRunException:
                return False
        finally:
            signal.signal(signal.SIGINT, sig)
    
    def _run_polymake_command( self, code ):
        self.polymakewrapper.sendline( code.rstrip() + '; ' + 'print "===endofoutput===";' )
        self.polymakewrapper.expect( 'print "===endofoutput===";' )
        error_number = self.polymakewrapper.expect( [ "ERROR", "===endofoutput===" ] )
        output = self.polymakewrapper.before.strip().rstrip()
        if error_number == 0:
            self.polymakewrapper.sendline( 'print "===endofoutput===";' )
            self.polymakewrapper.expect( 'print "===endofoutput===";' )
            output = 'Error' + self.polymakewrapper.before
            self.polymakewrapper.expect( "===endofoutput===" )
            raise PolymakeRunException( output )
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
        
        #stream_content = {'execution_count': self.execution_count, 'data': { 'text/plain': "Code:\n" + code_to_execute } }
        #self.send_response( self.iopub_socket, 'execute_result', stream_content )
        
        try:
            output = self._run_polymake_command( code )
        except KeyboardInterrupt:
            self.polymakewrapper.child.sendintr()
            self._run_polymake_command( '' )
            interrupted = True
        except pexpect.EOF:
            output = self.polymakewrapper.before + 'Restarting polymake'
            self._start_polymake()
        except PolymakeRunException as exception:
            output = exception.args[0]
            return {'status': 'error', 'execution_count': self.execution_count,
                    'ename': 'PolymakeRunException', 'evalue': output, 'traceback': []}
        if not silent:
            while output.find( '.@@HTML@@' ) != -1:
                html_position = output.find( '.@@HTML@@' )
                html_end_position = output.find( '.@@ENDHTML@@' )
                if html_position > 0:
                    before_html = output[:html_position-1].rstrip()
                else:
                    before_html = ''
                output_html = output[html_position+9:html_end_position-1].strip().rstrip()
                output = output[html_end_position+12:].strip()
                if before_html != '':
                    stream_content = {'execution_count': self.execution_count, 'data': { 'text/plain': before_html } }
                    self.send_response( self.iopub_socket, 'execute_result', stream_content )
                stream_content = {'execution_count': self.execution_count,
                                  'source' : "polymake",
                                  'data': { 'text/plain': "Sorry, threejs visualization is currently not available"},
                                  'metadata': dict() }
                self.send_response( self.iopub_socket, 'display_data', stream_content )
            if len(output) != 0:
                stream_content = {'execution_count': self.execution_count, 'data': { 'text/plain': output } }
                self.send_response( self.iopub_socket, 'execute_result', stream_content )
        
        if interrupted:
            return {'status': 'abort', 'execution_count': self.execution_count}

        return {'status': 'ok', 'execution_count': self.execution_count,
                'payload': [], 'user_expressions': {}}

    def do_shutdown(self, restart):
        
        self.polymakewrapper.terminate(force=True)
        if restart:
            self._start_polymake()


### basic code completion for polymake
### currently known shortcomings: intermediate completion, in particular for files, completion of variable names

    def code_completion (self,code):
        completion = []
        code = re.sub( "\)$", "", code)
        code = repr(code)
        code_line = 'print Jupyter::tab_completion(' + code + ');'
        try:
            output = self._run_polymake_command( code_line )
        except PolymakeRunException:
            return (0,[])
        completion = output.split("###")
        if ( len(completion) > 1 ) :
            completion_length = completion.pop(0)
        else :
            completion_length = 0
        return (completion_length,completion)

    
    def do_complete(self, code, cursor_pos):
        
        completion_length, completion = self.code_completion(code[0:cursor_pos])
        cur_start = cursor_pos - int(completion_length)
        
        return {'matches':  completion, 'cursor_start': cur_start,
                'cursor_end': cursor_pos, 'metadata': dict(),
                'status': 'ok'}

    def do_is_complete( self, code ):
        new_code = 'if(0){ ' + code + ' }'
        try:
            self._run_polymake_command( new_code )
        except PolymakeRunException:
            return {'status' : 'incomplete', 'indent': '' }
        return {'status' : 'complete' }

    def do_inspect( self, code, cursor_pos, detail_level=0 ):
        new_code = 'print Jupyter::context_help( q#' + code + '#, 1, "text" );'
        try:
            output = self._run_polymake_command( new_code )
        except PolymakeRunException:
            return {'status': 'ok', 'data': {}, 'metadata': {}, 'found': False}
        if output == '':
            return {'status': 'ok', 'data': {}, 'metadata': {}, 'found': False}
        else:
            return {'status': 'ok', 'data': { 'text/plain': output }, 'metadata': {}, 'found': True}
