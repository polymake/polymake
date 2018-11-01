#  Copyright (c) 1997-2018
#  Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
#  http://www.polymake.org
#
#  This program is free software; you can redistribute it and/or modify it
#  under the terms of the GNU General Public License as published by the
#  Free Software Foundation; either version 2, or (at your option) any
#  later version: http://www.gnu.org/licenses/gpl.txt.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#-------------------------------------------------------------------------------

use strict;
use namespaces;
use warnings qw(FATAL void syntax misc);

require Polymake::Core::InteractiveCommands;
require Polymake::Core::InteractiveHelp;
require Polymake::Core::ShellHelpers;

package Polymake::Core::Shell;

use Polymake::Ext;
use POSIX qw( :signal_h );

use Polymake::Struct (
   [ '@ISA' => 'Selector::Member' ],
   [ '$term' => 'Term::ReadLine::Gnu->new("polymake")' ],
   '$input_line',                       # for callback interface
   '$line_cnt',                         # in the current input group
   [ '$read_lines_from' => 'undef' ],   # temporary alternative source of lines, e.g. a handle of an edited history file
# for history management
   '$first_hist_index',                 # where in the history list the first line of the current input portion will go
   [ '$within_history' => 'undef' ],    # when defined: position in the history list to be copied to the pre-put
   '$within_history_begin',             # the index of the history line preceding the injected portion (for display in prompt)
   [ '$histfile' => '"$PrivateDir/history"' ],
   '@no_hist_lines',                    # [start, end]... blocks of history lines not to be saved in the history file
# for TAB completion
   '&try_completion',           # method filling the completion list
   '@completion_proposals',     # proposals for TAB completion
   '$partial_input',            # input received so far: continued lines concatenated with the input buffer
   '$completion_offset',        # offset from the end of partial input to the point where proposals should be appended
# for F1 context help
   '$help_point',               # cursor position when F1 was pressed last time
   '$help_input_line',          # input buffer when F1 was pressed last time
   '@help_topics',              # help topics prepared for the next F1 event
   '$help_repeat_cnt',          # how many times F1 has been pressed without moving the cursor
);

# All arguments are optional.
# Without arguments, starts an interactive shell.
# Otherwise reads from a pipe: FILEHANDLE, mask(1 | 2) = (redirect STDOUT, redirect STDERR)
sub start {
   if (my ($pipe, $redirects)=@_) {
      $Shell=new OverPipe($pipe);
      if ($redirects & 1) {
         close STDOUT;
         open STDOUT, ">&=", $Shell->pipe
           or die "can't redirect STDOUT: $!\n";
      }
      if ($redirects & 2) {
         close STDERR;
         open STDERR, ">&=", $Shell->pipe
         or die "can't redirect STDERR: $!\n";
      }
   } else {
      require Term::ReadLine;
      *Term::ReadLine::Gnu::AUTOLOAD=\&Term::ReadLine::Gnu::AU::AUTOLOAD;
      $Shell=new Shell(\*STDIN);
      defuse_environ_bug();
      $Shell->term->Attribs->{completion_append_character}="";
      $Shell->term->Attribs->{completer_word_break_characters}.="+-*/.%})[]?!,";
      $Shell->term->Attribs->{special_prefixes}="";
      $Shell->term->Attribs->{basic_quote_characters}="";
      $Shell->term->Attribs->{completion_entry_function}=\&completion_entry;
      $Shell->term->Attribs->{history_base}=0;
      $Shell->term->variable_bind("show-all-if-unmodified","on");
      $Shell->term->ornaments('md,me,,');  # bold prompt
      $Shell->try_completion=\&all_completions;

      my $F1=InteractiveCommands::Tgetent()->{$User::help_key};
      $Shell->term->bind_keyseq($F1, $Shell->term->add_defun("context_help", \&context_help));

      add AtEnd("Shell", sub { save_history($Shell); undef $Shell; });
   }
}

sub interactive { 1 }

my $input_preamble=<<".";
$warn_options;  use application;  declare +auto;
#line 1 "input"
.

my $input_preamble_utf8="use utf8; $input_preamble";

# utf8 flag =>
sub input_preamble { $_[0] ? \$input_preamble_utf8 : \$input_preamble }

if ($] < 5.020) {
   *input_preamble=sub { \(my $copy=$_[0] ? $input_preamble_utf8 : $input_preamble) }
}

my $canceled=sub {
   if ($Shell->line_cnt>0 || $Shell->term->Attribs->{line_buffer} =~ /\S/) {
      $Shell->term->delete_text;
      $Shell->term->redisplay;
      print STDERR "Canceled\n";
   } else {
      print STDERR "Type 'exit;' to leave polymake\n";
   }
   $Shell->line_cnt=0;
   $Shell->term->callback_handler_remove unless defined($Shell->input_line);
   $Shell->term->cleanup_after_signal;
   if (defined $Shell->within_history) {
      undef $Shell->within_history;
      $Shell->term->history_set_pos($Shell->term->Attribs->{history_length}-1);
      $Shell->term->Attribs->{startup_hook}=undef;
   }
   die "1\n";
};

sub interactive_INC {
   if ($_[1] eq "input:") {
      $Shell->first_hist_index=$Shell->term->Attribs->{history_length};
      $Shell->line_cnt=0;
      $Shell->help_point=-1;
      $#{$Shell->help_topics}=-1;
      # TODO: investigate how to query readline for the current character encoding, or just resort to $ENV{LANG}?
      (input_preamble(1), \&get_line, $Shell)
   } else {
      $User::application->INC($_[1]);
   }
}

sub pipe_INC {
   if ($_[1] eq "input:") {
      $Shell->line_cnt=0;
      (input_preamble(1), \&OverPipe::get_line, $Shell)
   } else {
      $User::application->INC($_[1]);
   }
}

sub run {
   local_unshift(\@INC, \&interactive_INC);

   local $SIG{__WARN__}=sub {
      my $msg=shift;
      if ($msg =~ /input line \d+/) {
         die $msg;
      } else {
         warn_print($msg);
      }
   };
   my $sa_INT_save=new POSIX::SigAction('DEFAULT');
   my $sa_INT=new POSIX::SigAction($canceled, new POSIX::SigSet(SIGQUIT, SIGALRM, SIGPIPE), SA_NODEFER);
   sigaction SIGINT, $sa_INT, $sa_INT_save;

   $Shell->term->ReadHistory($Shell->histfile) if -f $Shell->histfile;
   $Shell->term->{MinLength}=0;     # don't automatically add to history

   print "\rPress F1 or enter 'help;' for basic instructions.\n";
   STDOUT->flush();
   for (;;) {
      # the funny trailing '1;' ensures that all temporaries created in the input lines
      # are destroyed prior to the Scope object.
      {  local $Scope=new Scope(); package Polymake::User; do "input:"; 1; }
      STDOUT->flush();
      $Shell->term->crlf();
      if ($@) {
         if ($Shell->line_cnt>0) {
            if ($Shell->line_cnt>1) {
               $@ =~ s{(input line \d+), <.*?> line \d+}{$1}g;
            } else {
               $@ =~ s{ at input line 1(?:\.|, <.*?> line \d+)}{}g;
            }
            err_print($@);
            add_no_hist_lines($Shell, $Shell->first_hist_index, $Shell->term->Attribs->{history_length}-1);
         }
      }
   }
   sigaction SIGINT, $sa_INT_save;
}

sub run_pipe {
   local_unshift(\@INC, \&pipe_INC);
   do {
      {  local $Scope=new Scope(); package Polymake::User; do "input:"; 1; }
      STDOUT->flush();
      if ($@) {
         if ($Shell->line_cnt > 0) {
            $@ =~ s{ at input line \d+, <.*?> line \d+}{}g;
            err_print($@);
         }
      }
   } while ($Shell->line_cnt >= 0);
}

sub add_no_hist_lines {
   my ($self, $low, $high)=@_;
   if (!@{$self->no_hist_lines} || $self->no_hist_lines->[-1]->[1]+1 < $low) {
      push @{$self->no_hist_lines}, [ $low, $high ];
   } else {
      $self->no_hist_lines->[-1]->[1]=$high;
   }
}

sub save_history {
   my ($self, $filename)=@_;
   my $hi=$self->term->Attribs->{history_length}-1;
   if (!defined($filename) && $self->term->history_get($hi) =~ /^\s*exit\s*;\s*$/) {
      add_no_hist_lines($self, $hi, $hi);
   }
   foreach (reverse @{$self->no_hist_lines}) {
      for ($hi=$_->[1]; $hi>=$_->[0]; --$hi) {
         $self->term->remove_history($hi);
      }
   }
   $#{$self->no_hist_lines}=-1;
   unless (defined $filename) {
      $self->term->StifleHistory($User::history_size);
      $filename=$self->histfile;
   }
   $self->term->WriteHistory($filename);
}

sub get_line {
   my ($l, $self)=@_;
   if ($self->line_cnt==0 || ($l=line_continued())>0) {
      namespaces::temp_disable();
      unless ($User::application->declared & $Application::credits_shown) {
         User::show_credits(1);
         if ($User::application->declared & $Application::has_failed_config) {
            print <<'.';

Warning: some rulefiles could not be configured automatically
due to lacking third-party software and/or other issues.
To see the complete list: show_unconfigured;
.
         }
         $User::application->declared |= $Application::credits_shown;
      }
      my $prompt=$User::application->name." ";
      if (defined($self->within_history)) {
         $prompt.="[".($self->within_history-$self->within_history_begin)."]";
      }
      $prompt.="($l)" if ($l > 1);

      my $line;
      do {
         $line=$self->readline("$prompt> ");
      } while ($line =~ $nonsignificant_line_re);
      if (defined($self->within_history)) {
         $self->line_cnt=$l || 1;
         $self->within_history=$self->term->where_history;
         $self->term->replace_history_entry($self->within_history, $line);
         if (++$self->within_history >= $self->first_hist_index) {
            undef $self->within_history;
            $self->term->Attribs->{startup_hook}=undef;
         }
      } elsif ($self->line_cnt > 0 && $line =~ /^\s*;\s*$/) {
         my $hi=$self->term->Attribs->{history_length}-1;
         $self->term->replace_history_entry($hi, $self->term->history_get($hi).";");
      } elsif ($line !~ /$end_of_source_file_re/) {
         $self->line_cnt=$l || 1;
         $self->term->add_history($line);
      }
      $_ .= $line."\n";
   }
   return length;
}

# The hooks and callbacks here should not copy $self during closure cloning
# because it would defer the destruction of the Shell object!

sub readline {
   my ($self, $prompt)=@_;
   if (defined $self->read_lines_from) {
      my $line=CORE::readline($self->read_lines_from);
      if (length($line)==0 ? ($line="$end_of_source_file\n") : $line =~ $end_of_source_file_re) {
         undef $self->read_lines_from;
      } elsif ($line !~ $nonsignificant_line_re) {
         print STDOUT $prompt, $line;
         chomp $line;
      }
      $line
   } elsif (keys %active==1) {
      $self->term->readline($prompt);
   } else {
      $self->term->CallbackHandlerInstall($prompt, sub { $Shell->input_line=shift; $Shell->term->callback_handler_remove; });
      undef $self->input_line;
      do {
         try_read($self) or return;
         $self->term->callback_read_char;
      } until (defined $self->input_line);
      $self->input_line;
   }
}

*read_input=\&readline;

sub in_avail { (shift)->term->callback_read_char; }

sub completion_entry {
   my ($word, $state)=@_;
   if (!$state) {
      $Shell->term->Attribs->{completion_append_character}="";
      $Shell->term->Attribs->{filename_quoting_desired}=0;
      $Shell->completion_proposals=[];
      $Shell->completion_offset=0;
      $Shell->try_completion->();
      trim_completion_proposals($Shell, $word);
      if (substr($Shell->term->Attribs->{line_buffer}, $Shell->term->Attribs->{point}, 1) eq $Shell->term->Attribs->{completion_append_character}) {
         $Shell->term->Attribs->{completion_append_character}="";
      }
   }
   $Shell->completion_proposals->[$state];
}

# readline expects proposals for the trailing word only, without any separators and interpuctuation;
# if the end of the input line matches a more complex syntactic construction, the proposals must be trimmed
sub trim_completion_proposals {
   my ($self, $word)=@_;
   my $trim=$self->completion_offset-length($word);
   if ($trim>0) {
      substr($_,0,$trim)="" for @{$self->completion_proposals};
   } elsif ($trim<0) {
      $word=substr($word,0,-$trim);
      substr($_,0,0).=$word for @{$self->completion_proposals};
   }
}

sub try_filename_completion {
   my ($self, $quote, $prefix, $dir_cmd)=@_;
   if (my @list=$self->term->completion_matches($prefix, $self->term->Attribs->{filename_completion_function})) {
      shift @list if $#list;
      if ($dir_cmd) {
         unshift @list, ".." if length($prefix)==0 || $prefix =~ /^\.\.?$/;
         $self->completion_proposals=[ grep { m|^~/| ? -d "$ENV{HOME}/$'" : -d $_ } @list ];
         $self->term->Attribs->{completion_append_character}="/";
      } else {
         foreach (@list) {
            if (m|^~/| ? -d "$ENV{HOME}/$'" : -d $_) {
               $_.="/";
               $quote="";
            }
         }
         $self->completion_proposals=\@list;
         $self->term->Attribs->{completion_append_character}=$quote;
      }
      $self->completion_offset=length($prefix);
      $self->term->Attribs->{filename_completion_desired}=0;
   }
}

# private:
sub prepare_partial_input {
   my ($self)=@_;
   my $line=$self->term->Attribs->{line_buffer};
   $self->partial_input="";
   my $pos;
   if ($self->line_cnt > 1) {
      my $last_hist=$self->term->Attribs->{history_length}-1;
      $self->partial_input=join(" ", map { $self->term->history_get($_) } $last_hist-$self->line_cnt+2 .. $last_hist).$line;
      $pos=$self->term->Attribs->{point} + length($self->partial_input) - length($line);
   } else {
      $self->partial_input=$line;
      $pos=$self->term->Attribs->{point};
   }
   $self->partial_input =~ /\S/ ? $pos : -1
}

sub all_completions : method {
   my ($self)=@_;
   my $pos=&prepare_partial_input;
   return if $pos<0;
   pos($self->partial_input)=$pos;
   foreach my $h (@Helper::list) {
      if (substr($self->partial_input, 0, $pos) =~ /${$h->pattern}/) {
         return if $h->completion->($self);
      }
   }

   if (substr($self->partial_input, 0, $pos) =~
       m{^ $quote_balanced_re $quote_re (?'filename' $non_quote_space_re*) $}xo) {
      # if nothing else worked out, assume a file name
      try_filename_completion($self, $+{quote}, $+{filename});
   }
}

# private:
sub fetch_help_topics {
   my ($self, $pos)=@_;
   $#{$self->help_topics}=-1;
   pos($self->partial_input)=$pos;
   $pos=0;

   do {
      # move before the end of the statement if nothing follows
      $self->partial_input =~ m{(?: \)\s* (?: ;\s*)? | \s*;\s* ) \G \s*$ }xgc and pos($self->partial_input)=$-[0];

      # move before the trailing spaces and operator signs unless at the beginning of a name
      $self->partial_input =~ m{[-+*/=&|^\s]+ \G (?![a-zA-Z])}xgc and pos($self->partial_input)=$-[0];

      # move before the closing quote of a string if standing immediately behind it
      $self->partial_input =~ m{^ $quote_balanced_re (?<= $anon_quote_re) \G}xogc and pos($self->partial_input)=pos($self->partial_input)-1;

      # move before the keyword arrow if standing immediately behind it
      $self->partial_input =~ m{ ($id_re) \s* => \G }xogc and pos($self->partial_input)=$+[1];

      # move past the name if currently pointing in its middle
      $self->partial_input =~ m{($qual_id_re)? \G (?(1) [\w:]* | $qual_id_re)}xogc;

      foreach my $h (@Helper::list) {
         if (defined($h->help) &&
             substr($self->partial_input, 0, pos($self->partial_input)) =~ /${$h->pattern}/
               and
             ($pos, @{$self->help_topics})=$h->help->($self)) {
            if ($pos >= pos($self->partial_input)) {
               croak( "internal error:\nhelper ", $h->name, " requested to move position from\n",
                      substr($self->partial_input, 0, pos($self->partial_input)), "<<*>>", substr($self->partial_input, pos($self->partial_input)), "\nto\n",
                      substr($self->partial_input, 0, $pos), "<<*>>", substr($self->partial_input, $pos), "\n");
            }
            pos($self->partial_input)=$pos;
            return 1 if @{$self->help_topics};
            last;
         }
      }
   } while ($pos>0);
   0
}

# private:
sub display_help_topics {
   my ($self)=@_;
   my $full_text=$self->help_repeat_cnt>0;
   my ($tell_about_full, $pos);
   my $n=0;

   foreach (@{$self->help_topics}) {
      next if is_object($_) && $_->annex->{display} =~ /^\s*noshow\s*$/;
      print "-------------------\n" if $User::help_delimit && $n++;
      if (is_object($_)) {
         print $full_text ? substr($_->full_path,1).":\n" : "",
               $_->display_text($full_text, $tell_about_full);
      } else {
         print $_->($full_text, $tell_about_full);
      }
   }
   if ($tell_about_full) {
      print "Press help key again for more details\n";
   } elsif (($pos=pos($self->partial_input)-1) >= 0  and
            fetch_help_topics($self, $pos)) {
      print "\nPress help key again for another topic\n";
   } else {
      # rewind to the initial state
      $Shell->help_point=-1;
      $Shell->help_input_line="";
   }
}

sub context_help {
   print "\n";
   my $pt=$Shell->term->Attribs->{point};
   my $line=$Shell->term->Attribs->{line_buffer};
   if ($pt==$Shell->help_point && $line eq $Shell->help_input_line) {
      # repeatedly pressed F1 without moving the cursor
      if (@{$Shell->help_topics}) {
         ++$Shell->help_repeat_cnt;
         display_help_topics($Shell);
      }
   } else {
      my $pos=prepare_partial_input($Shell);
      if ($pos>=0) {
         $Shell->help_point=$pt;
         $Shell->help_input_line=$line;
         $Shell->help_repeat_cnt=0;
         if (fetch_help_topics($Shell, $pos)) {
            display_help_topics($Shell);
         } else {
            print "Sorry, no matching help topics found; type more input, try TAB completion, or move the cursor towards a word of interest\n";
         }
      } else {
         User::help();
      }
   }

   STDOUT->flush;
   $Shell->term->on_new_line;
   $Shell->term->redisplay;
}
###############################################################################################
sub Polymake::User::history {
   my $tempfile=new Tempfile();
   my $scriptfile="$tempfile.pl";
   open my $hf, ">$scriptfile" or die "can't create temporary file $scriptfile: $!\n";
   print $hf <<'.';
#############################################################################
#  Please put the command(s) you want to be executed at the very beginning  #
#  of this file;  everything below this line will be ignored.               #
#############################################################################
__END__
.
   my $last=$Shell->term->Attribs->{history_length}-1;
   if ($Shell->term->history_get($last) =~ /^\s*history\s*;\s*/) {
      $Shell->term->remove_history($last--);
   }
   my ($exclude, $last_exclude)=(0, $#{$Shell->no_hist_lines});
   for (my $i=0; $i<=$last; ++$i) {
      if ($exclude > $last_exclude ||
          $i < $Shell->no_hist_lines->[$exclude]->[0]) {
         print $hf $Shell->term->history_get($i), "\n";
      } else {
         $i=$Shell->no_hist_lines->[$exclude++]->[1];
      }
   }
   close $hf;
   my ($size, $created_at)=(stat $scriptfile)[7,9];
   my $ed_cmd=$User::history_editor;
   $ed_cmd =~ s/%f/$scriptfile/g or $ed_cmd.=" $scriptfile";
   system($ed_cmd);
   my ($new_size, $new_time);
   if (-f $scriptfile and
       ($new_size, $new_time)=(stat _)[7,9] and
       $new_time > $created_at || $new_size != $size) {
      open $hf, $scriptfile
        or die "can't read from $scriptfile: $!\n";
      $Shell->read_lines_from=$hf;
   } else {
      warn_print("no new commands found");
   }
}
###############################################################################################
sub prepare_replay {
   my ($cur_hist, $beg_hist)=@_;
   $cur_hist //= $Shell->term->Attribs->{history_length};
   $beg_hist //= $cur_hist-1;
   $Shell->term->Attribs->{startup_hook}=sub {
      $Shell->term->history_set_pos($Shell->within_history);
      $Shell->term->insert_text($Shell->term->current_history);
   };
   $Shell->within_history=$cur_hist;
   $Shell->within_history_begin=$beg_hist;
}

sub fill_history {
   shift if $_[0]==$Shell;
   my $opts= ref($_[0]) eq "HASH" ? shift : {};
   if (@_) {
      prepare_replay;
      my $filter=$opts->{filter};
      foreach (@_) {
         chomp;
         $Shell->term->add_history($_)
           unless (defined($filter) && /$filter/);
      }
      if ($opts->{temporary}) {
         add_no_hist_lines($Shell, $Shell->first_hist_index, $Shell->term->Attribs->{history_length}-1);
      }
      1
   } else {
      undef
   }
}
###############################################################################################
sub Polymake::User::load_commands {
   my ($filename)=@_;
   replace_special_paths($filename);
   open my $in, $filename or die "can't read from $filename: $!\n";
   fill_history({ filter => $nonsignificant_line_re }, <$in>);
}
###############################################################################################
sub Polymake::User::replay_history {
   my $last=$Shell->term->Attribs->{history_length}-1;
   if ($Shell->term->history_get($last) =~ /^\s*replay_history\s*;\s*/) {
      $Shell->term->remove_history($last--);
   }
   prepare_replay($last, 0);
}
###############################################################################################
sub Polymake::User::save_history {
   my ($filename)=@_;
   $Shell->save_history($filename);
}
###############################################################################################
sub enter_string {
   my ($self, $text, $opts)=@_;
   local @{$self->term->Attribs}{qw( completer_word_break_characters
                                     startup_hook )}
         =( $self->term->Attribs->{basic_word_break_characters},
            sub { $self->term->insert_text($text) } );
   local $self->term->{MinLength}=1;

   my $check=$opts->{check};
   defined($opts->{completion}) and local_sub($self->try_completion, $opts->{completion});

   for (;;) {
      my $response=$self->read_input("[$opts->{prompt}] > ");
      $response =~ s/^\s+//;  $response =~ s/\s+$//;
      length($response) or return;
      if (defined($check) && defined (my $error=$check->($response))) {
         if (substr($error, -1, 1) ne "\n") {
            $error .= " or an empty string to abort.\n";
         }
         print $error;
         $text="";
      } else {
         return $response;
      }
   }
}
###############################################################################################
sub enter_filename {
   my ($self, $text, $opts)=@_;
   $opts->{prompt} //= "filename";
   my $check=$opts->{check};
   $opts->{check}=sub {
      replace_special_paths($_[0]);
      my $error=$check && $check->($_[0]);
      $error && "Invalid input: $error\nPlease enter an alternative location" 
   };
   local ${$self->term->Attribs}{completion_entry_function} = $self->term->Attribs->{filename_completion_function};
   enter_string($self, $text, $opts);
}
###############################################################################################
#
#  A reduced version of the Shell, reading commands from an input stream (pipe, socket, etc.)
#  No interactive capabilities like TAB completion or context-sensitive help are provided.

package Polymake::Core::Shell::OverPipe;

use Polymake::Struct (
   [ new => '$' ],
   [ '$pipe' => 'new Pipe(#1)' ],
   '$line_cnt',
);

sub interactive { 0 }

sub get_line {
   my ($l, $self)=@_;
   if ($self->line_cnt==0 || ($l=line_continued())>0) {
      namespaces::temp_disable();
      my $line;
      do {
         $line=readline($self->pipe);
         if (length($line)==0) {
            # pipe closed
            $self->line_cnt=-1;
            return length;
         }
      } while ($line =~ $nonsignificant_line_re);
      $self->line_cnt=$l || 1;
      $_ .= $line."\n";
   }
   return length;
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
