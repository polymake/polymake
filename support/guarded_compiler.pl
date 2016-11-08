use strict;
use IPC::Open3;
use Symbol 'gensym';

my @cmd=@ARGV;
my $err=gensym;
my $rc=0;
my $retry=1;

unless (grep { /^-f(?:no-)?diagnostics-color/ } @cmd) {
   splice @cmd, 1, 0, (-t STDERR ? "-fdiagnostics-color" : "-fno-diagnostics-color");
}

while ($retry) {
   open DupSTDIN, "<&", \*STDIN;
   open DupSTDOUT, ">&", \*STDOUT;
   my $pid=open3("<&DupSTDIN", ">&DupSTDOUT", $err, @cmd);
   my @err=<$err>;
   waitpid($pid,0);
   $rc=$?;
   if ($rc & 127) {
      print STDERR @err, "\n$cmd[0] died with signal ", $rc & 127, ":\n";
      exit(1);
   }
   $rc >>= 8;
   if (!$rc) {
      print STDERR @err if @err;
      exit(0);
   }
   $retry=0;
   eval {
      require Cwd;
      my $wrapper_file=Cwd::abs_path($cmd[-1]);
      for (@err) {
	 if (my ($first_error_file, $line) = m{^\s*([^\s:]+):\s*(\d+)\s*:.*?(?i-:error):}) {
	    if (Cwd::abs_path($first_error_file) eq $wrapper_file) {
	       open IN, $wrapper_file
	         or die "can't read $wrapper_file: $!\n";
	       my @contents=<IN>;
	       close IN;
	       if ($contents[$line-1] =~ s/\bWrapperReturn((?:Lvalue|Anch|New)*)\w*/ObsoleteWrapper$1/) {
		  open OUT, ">", "$wrapper_file.new"
		    or die "can't create $wrapper_file.new: $!\n";
		  print OUT @contents;
		  close OUT;
		  rename "$wrapper_file.new", $wrapper_file
		    or die "can't rename $wrapper_file.new into $wrapper_file: $!\n";
		  $retry=1;
	       }
	    }
	    last;
	 }
      }
   };
   
   print STDERR @err unless $retry;
   print STDERR "\nAttempt to repair the wrapper code failed: $@" if $@;
}

exit($rc);
