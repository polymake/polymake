#  Copyright (c) 1997-2015
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

require Polymake::Core::InteractiveCommands;
require Polymake::Core::InteractiveHelp;
require Polymake::Core::ShellHelpers;

package Polymake::Core::Shell;

use Polymake::Ext;
use POSIX qw( :signal_h );

use Polymake::Struct (
   [ '@ISA' => 'Selector::Member' ],
   [ '$term' => 'Term::ReadLine::Gnu->new("polymake")' ],
   [ '$state' => '0' ],
   '$input_line',               # for callback interface
   '$line_cnt',                 # in the current input group
# for history management
   '$first_hist_index',         # where in the history list the first line of the current input portion will go
   [ '$within_history' => 'undef' ],    # when defined: position in the history list to be copied to the pre-put
   '$within_history_begin',     # the index of the history line preceding the injected portion (for display in prompt)
   '$last_error_was_in_history',
   [ '$histfile' => '"$PrivateDir/history"' ],
   '@remove_hist_lines',        # [ start..end ], ...
# for TAB completion
   '&try_completion',           # method choosing the completion list
   '&completion',               # method iterating over the completion list
   '@completion_words',         # proposals for TAB completion
   '$partial_input',            # input received so far: continued lines concatenated with the input buffer
# for F1 context help
   '$help_point',               # cursor position when F1 was pressed last time
   '$help_input_line',          # input buffer when F1 was pressed last time
   '@help_topics',              # help topics prepared for the next F1 event
   '$help_repeat_cnt',          # how many times F1 has been pressed without moving the cursor
);

declare @custom_tab_completion;

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

   }
   add AtEnd("Shell", sub { undef $Shell });
}

sub interactive { 1 }

my $interactive_preamble="$warn_options; use application undef, \$namespaces::auto_declare;\n";

my $canceled=sub {
   if ($Shell->state==3 || $Shell->term->Attribs->{line_buffer} =~ /\S/) {
      $Shell->term->delete_text;
      $Shell->term->redisplay;
      print STDERR "Canceled\n";
   } else {
      print STDERR "Type 'exit;' to leave polymake\n";
   }
   $Shell->state=0;
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
      (\&get_line, $Shell)
   } elsif ($_[1] =~ /^history:/) {
      open my $hf, $';
      new ScriptFilter($hf, [ $interactive_preamble, "#line 1 \"history\"\n" ],
                       sub { $_[0] =~ s{ ($statement_start_re) (application (?: (\s*\(\s*) | \s+) (['"]) $id_re \4) ((?(3) \s*\)\s* | \s*);) }{$1 use $2,1$5}xo; });
   } else {
      $User::application->INC($_[1]);
   }
}

sub pipe_INC {
   if ($_[1] eq "input:") {
      (\&OverPipe::get_line, $Shell)
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
   do {
      $Shell->state=0;
      # the funny trailing '1;' provides all temporaries created in the eval'd expression be destroyed prior to the Scope object.
      {  local $Scope=new Scope(); package Polymake::User; do "input:"; 1; }
      STDOUT->flush();
      $Shell->term->crlf();
      if ($@) {
         if ($Shell->state>1) {
            if ($Shell->line_cnt>1) {
               $@ =~ s{(input line \d+), <.*?> line \d+}{$1}g;
            } else {
               $@ =~ s{ at input line 1, <.*?> line \d+}{}g;
            }
            err_print($@);
         }
         if ($Shell->line_cnt>0) {
            push @{$Shell->remove_hist_lines}, [$Shell->first_hist_index, $Shell->term->Attribs->{history_length}-1];
         }
         $Shell->last_error_was_in_history=0;
      }
   } while ($Shell->state >= 0);
   sigaction SIGINT, $sa_INT_save;
}

sub run_pipe {
   local_unshift(\@INC, \&pipe_INC);
   do {
      $Shell->state=0;
      {  local $Scope=new Scope(); package Polymake::User; do "input:"; 1; }
      STDOUT->flush();
      if ($@) {
         if ($Shell->state>1) {
            $@ =~ s{ at input line \d+, <.*?> line \d+}{}g;
            err_print($@);
         }
      }
   } while ($Shell->state >= 0);
}

sub save_history {
   my ($self, $filename)=@_;
   my $hi=$self->term->Attribs->{history_length}-1;
   if (!defined($filename) && $self->term->history_get($hi) =~ /^\s*exit\s*;\s*$/) {
      push @{$self->remove_hist_lines}, [$hi,$hi];
   }
   foreach (reverse @{$self->remove_hist_lines}) {
      for ($hi=$_->[1]; $hi>=$_->[0]; --$hi) {
         $self->term->remove_history($hi);
      }
   }
   $#{$self->remove_hist_lines}=-1;
   unless (defined $filename) {
      $self->term->StifleHistory($User::history_size);
      $filename=$self->histfile;
   }
   $self->term->WriteHistory($filename);
}

sub DESTROY {
   my $self=shift;
   save_history($self);
   $self->SUPER::DESTROY;
}

sub get_line {
   my ($l, $self)=@_;
   if ($self->state==0) {
      $self->state=1;
      $_ .= $interactive_preamble;
   } elsif ($self->state==1) {
      $self->state=2;
      $self->first_hist_index=$self->term->Attribs->{history_length};
      $self->line_cnt=0;
      $self->help_point=-1;
      $#{$self->help_topics}=-1;
      $_ .= "#line 1 \"input\"\n";
   } elsif ($self->state!=3 || ($l=line_continued())) {
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
      if ($l>0) {
         $self->line_cnt=$l;
      }
      if ($self->line_cnt>1) {
         $prompt.="(".$self->line_cnt.")";
      }
      my $line;
      do {
         $line=$self->readline("$prompt> ");
      } until ($line =~ /\S/);
      $self->state=3;
      if (defined($self->within_history)) {
         $self->within_history=$self->term->where_history;
         $self->term->replace_history_entry($self->within_history, $line);
         if (++$self->within_history >= $self->first_hist_index) {
            undef $self->within_history;
            $self->term->Attribs->{startup_hook}=undef;
         }
      } elsif ($self->line_cnt>1 && $line =~ /^\s*;\s*$/) {
         my $hi=$self->term->Attribs->{history_length}-1;
         $self->term->replace_history_entry($hi, $self->term->history_get($hi).";");
      } else {
         $self->term->add_history($line);
      }
      $_ .= $line."\n";
   } else {
      $self->state=2;
      $self->line_cnt++;
   }
   return length;
}

# the hooks and callbacks here should not copy $self during closure cloning as it defers the destruction of the Shell object!

sub readline {
   my ($self, $prompt)=@_;
   if (keys %active==1) {
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
      undef $Shell->completion;
      $Shell->term->Attribs->{completion_append_character}="";
      $Shell->term->Attribs->{filename_quoting_desired}=0;
      $Shell->completion_words=[];
      $Shell->try_completion->($word);
      if (substr($Shell->term->Attribs->{line_buffer}, $Shell->term->Attribs->{point}, 1) eq $Shell->term->Attribs->{completion_append_character}) {
         $Shell->term->Attribs->{completion_append_character}="";
      }
   }
   $Shell->completion ? $Shell->completion->(@_) : $Shell->completion_words->[$state];
}

sub postprocess_file_list {
   my ($self, $word, $quote, $prefix, $dir_cmd, $list)=@_;
   my $trim=length($prefix)-length($word);
   my $dir_seen;
   if ($dir_cmd) {
      unshift @$list, ".." if length($prefix)==0 || $prefix =~ /^\.\.?$/;
      $self->completion_words=[ map { substr($_, $trim) } grep { m|^~/| ? -d "$ENV{HOME}/$'" : -d $_ } @$list ];
      $self->term->Attribs->{completion_append_character}="/";
   } else {
      $self->completion_words=[ map { substr($_, $trim).( m|^~/| ? -d "$ENV{HOME}/$'" : -d $_ and $dir_seen="/") } @$list ];
      $self->term->Attribs->{completion_append_character}=$quote unless $dir_seen;
   }
   $self->term->Attribs->{filename_completion_desired}=0;
}

sub try_filename_completion {
   my ($self, $word, $quote, $prefix, $dir_cmd)=@_;
   if (length($prefix) > length($word)) {
      if (my @list=$self->term->completion_matches($prefix,$self->term->Attribs->{filename_completion_function})) {
         shift @list if $#list;
         postprocess_file_list(@_, \@list);
      }
   } else {
      $self->completion=$self->term->Attribs->{filename_completion_function};
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
   my ($self, $word)=@_;
   my $pos=&prepare_partial_input;
   return if $pos<0;
   pos($self->partial_input)=$pos;
   foreach my $h (@Helper::list) {
      if (substr($self->partial_input, 0, $pos) =~ /${$h->pattern}/) {
         return if $h->completion->($self, $word);
      }
   }

   # try tab completion rules defined in extensions
   foreach (@custom_tab_completion) {
      return if $_->($self, substr($self->partial_input, 0, $pos), $word);
   }

   if (substr($self->partial_input, 0, $pos) =~
       m{^ $quote_balanced_re $quote_re (?'filename' $non_quote_space_re*) $}xo) {
      # if nothing else worked out, assume a file name
      try_filename_completion($self, $word, $+{quote}, $+{filename}, 0);
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
         print $full_text && substr($_->full_path,1).":\n",
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
      $Shell->term->remove_history($last);
   }
   my ($err, $last_err)=(0, $#{$Shell->remove_hist_lines}-$Shell->last_error_was_in_history);
   for (my $i=0; $i<$last; ++$i) {
      if ($err > $last_err || $i < $Shell->remove_hist_lines->[$err]->[0]) {
         print $hf $Shell->term->history_get($i), "\n";
      } else {
         $i=$Shell->remove_hist_lines->[$err++]->[1];
      }
   }
   close $hf;
   my $created_at=(stat $scriptfile)[9];
   my $ed_cmd=$User::history_editor;
   $ed_cmd =~ s/%f/$scriptfile/g or $ed_cmd.=" $scriptfile";
   system($ed_cmd);
   if (-f $scriptfile && (stat _)[9]>$created_at) {
      my $with_error;
      { package Polymake::User; do "history:$scriptfile"; }
      if ($@) {
         $@ =~ s{(history line \d+), <.*?> line \d+}{$1}g;
         STDOUT->flush();
         $Shell->term->crlf();
         err_print($@);
         $with_error=1;
      }
      open $hf, $scriptfile;
      while (<$hf>) {
         chomp;
         last if $_ eq "__END__";
         next if /^\s*(?:\#|$)/;
         $Shell->term->add_history($_);
      }
      close $hf;
      if ($with_error) {
         push @{$Shell->remove_hist_lines}, [$Shell->first_hist_index, $Shell->term->Attribs->{history_length}-1];
         $Shell->last_error_was_in_history=1;
      }
   } else {
      warn_print("no new commands found");
   }
}
###############################################################################################
my $replay_mode_startup=sub {
   $Shell->term->history_set_pos($Shell->within_history);
   $Shell->term->insert_text($Shell->term->current_history);
};

sub fill_history {
   my ($self, $temporary)=splice @_, 0, 2;
   if (@_) {
      $self->within_history=$self->term->Attribs->{history_length};
      $self->within_history_begin=$self->within_history-1;
      foreach (@_) {
         chomp;
         $self->term->add_history($_);
      }
      $self->term->Attribs->{startup_hook}=$replay_mode_startup;
      if ($temporary) {
         push @{$self->remove_hist_lines}, [$self->first_hist_index, $self->term->Attribs->{history_length}-1];
      }
      1
   } else {
      undef
   }
}

sub Polymake::User::load_commands {
   my $file=shift;
   replace_special_paths($file);
   open my $in, $file or die "can't read from $file: $!\n";
   fill_history($Shell, 0, grep { ! /$nonsignificant_line_re/o } <$in>)
      or warn_print( "no commands found" );
}
###############################################################################################
sub Polymake::User::replay_history {
   my $last=$Shell->term->Attribs->{history_length}-1;
   if ($Shell->term->history_get($last) =~ /^\s*replay_history\s*;\s*/) {
      $Shell->term->remove_history($last--);
   }
   $Shell->within_history=$last;
   $Shell->within_history_begin=0;
   $Shell->term->Attribs->{startup_hook}=$replay_mode_startup;
}
###############################################################################################
sub Polymake::User::save_history {
   $Shell->save_history(shift);
}
###############################################################################################
sub Polymake::User::help {
   my ($subject)=@_;
   my $help_delim=$User::help_delimit ? "-------------------\n" : '';
   my $app=$User::application;
   if ($subject =~ /^($id_re)::/o && $app->used->{$1}) {
      $subject=$';
      $app=$app->used->{$1};
   }
   if (my @topics=uniq( $app->help->get_topics($subject || "top") )) {
      my (@subcats, @subtopics, $need_delim);
      foreach my $topic (@topics) {
         my $text=$topic->display_text;
         if (length($text)) {
             print $help_delim if $need_delim++;
            print $text, "\n";
         }
         foreach (@{$topic->toc}) {
            if ($topic->topics->{$_}->category & 1) {
               push @subcats, $_;
            } else {
               push @subtopics, $_;
            }
         }
      }
      if (@subcats || @subtopics) {
         print $help_delim if $need_delim;
      }
      my $fp=substr($topics[0]->full_path,1);
      $fp &&= " of $fp";
      if (@subcats) {
         print "Categories$fp:\n", (join ", ", sort @subcats), "\n";
      }
      if (@subtopics) {
         print "Subtopics$fp:\n", (join ", ", sort @subtopics), "\n";
      }
   } elsif (length($subject) && index($subject,"/")<0
            and
            @topics=$app->help->find($subject)) {

      if (@topics==1) {
         print substr($topics[0]->full_path,1), ":\n", $topics[0]->display_text;
      } else {
         print "There are ", scalar(@topics), " help topics matching '$subject':\n";
         my $n=0;
         if (@topics<5) {
            foreach (@topics) {
               print $help_delim, ++$n, ": ", substr($_->full_path,1), ":\n",
                     $_->display_text, "\n";
            }
         } else {
            fill_history($Shell, 1,
               map {
                  $subject=substr($_->full_path,1);
                  print ++$n, ": $subject\n";
                  "help '$subject';"
               } @topics);
            print $help_delim, "Please choose those interesting you via history navigation (ArrowUp/ArrowDown):\n";
         }
      }
   } elsif ($subject eq "credits") {
      print "Application ", $User::application->name, " does not use any third-party software directly\n";
   } else {
      err_print( "unknown help topic '$subject'" );
   }
}
###############################################################################################
sub enter_filename {
   my ($self, $text, $opts)=@_;  $opts ||= { };
   my $prompt=$opts->{prompt} || "filename";
   my $check_code=$opts->{check};

   local @{$self->term->Attribs}{qw( completion_entry_function
                                     completer_word_break_characters
                                     startup_hook )}
         =( $self->term->Attribs->{filename_completion_function},
            $self->term->Attribs->{basic_word_break_characters},
            sub { $self->term->insert_text($text) } );
   local $self->term->{MinLength}=1;
   my $response;
   for (;;) {
      $response=$self->read_input("[$prompt] > ");
      last unless length($response);
      replace_special_paths($response);
      if ($check_code and my $error=$check_code->($response)) {
         print "Invalid input: $error\n",
               "Please enter an alternative location or an empty string to abort.\n";
      } else {
         last
      }
   }
   $response;
}
###############################################################################################
# utilities for auto-configuration

sub program_completion : method {
   my ($self, $word)=@_;
   if ($word =~ m{^[/~]}) {
      $self->completion=$self->term->Attribs->{filename_completion_function};
   } elsif ($word =~ /\S/) {
      $self->completion_words=[ sorted_uniq(sort( map { /$filename_re/ } map { glob "$_/$word*" } split /:/, $ENV{PATH} )) ];
   }
}

sub enter_program {
   my ($self, $text, $opts)=@_;
   my $check_code= ref($opts) eq "HASH" && $opts->{check};
   local_sub($self->try_completion, \&program_completion);
   local @{$self->term->Attribs}{qw( completer_word_break_characters
                                     startup_hook )}
         =( $self->term->Attribs->{basic_word_break_characters},
            sub { $self->term->insert_text($text) } );
   local $self->term->{MinLength}=1;
   my $error;
   for (;;) {
      my $response=$self->read_input("[program path] > ");
      $response =~ s/^\s+//;  $response =~ s/\s+$//;
      length($response) or return;
      return $text if $response eq $text;
      if ($response =~ m{^[/~]}) {
         replace_special_paths($response);
         my ($executable, $args)=split /\s+/, $response, 2;
         if (-x $executable && -f _) {
            if ($check_code and $error=$check_code->($executable, $args)) {
               print "Program $executable did not met the requirements: $error\n",
                     "Please enter an alternative location or an empty string to abort.\n";
            } else {
               return $response;
            }
         } elsif (-e _) {
            print "$executable is either not a regular file or not executable\n",
                  "Please enter a correct location or an empty string to abort.\n";
         } else {
            print "File $executable does not exist\n",
                  "Please enter a correct location or an empty string to abort.\n";
         }
      } else {
         my ($first_shot, $full_path, $error)=Configure::find_program_in_path($response, $check_code ? $check_code : ());
         if (defined $first_shot) {
            return $first_shot ? $response : $full_path;
         }
         my ($executable)= $response =~ m/^(\S+)/;
         if (defined $error) {
            my ($location)= $full_path =~ $directory_of_cmd_re;
            print "Program $executable found at location $location, but did not met the requirements: $error\n",
                  "Please enter an alternative location or an empty string to abort.\n";
         } else {
            print "Program $executable not found anywhere along your PATH.\n",
                  "Please enter the full path or an empty string to abort.\n";
         }
      }
      $text="";
   }
}
###############################################################################################
#
#  A reduced version of the Shell, reading commands from an input stream (pipe, socket, etc.)
#  No interactive capabilities like TAB completion or context-sensitive help are provided.

package _::OverPipe;

use Polymake::Struct (
   [ new => '$' ],
   [ '$pipe' => 'new Pipe(#1)' ],
   [ '$state' => '0' ],
);

sub interactive { 0 }

sub get_line {
   my ($l, $self)=@_;
   if ($self->state==0) {
      $self->state=1;
      $_ .= $interactive_preamble;
   } elsif ($self->state==1) {
      $self->state=2;
      $_ .= "#line 1 \"input\"\n";
   } elsif ($self->state!=3 || ($l=line_continued())) {
      namespaces::temp_disable();
      my $line;
      do {
         $line=readline($self->pipe);
      } until ($line =~ /\S/);
      $self->state=3;
      $_ .= $line."\n";
   } else {
      $self->state=2;
   }
   return length;
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
