#  Copyright (c) 1997-2014
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

require Term::ReadLine;
require Polymake::Core::InteractiveCommands;
require Polymake::Core::InteractiveHelp;

package Polymake::Core::Shell;

use Polymake::Ext;
use POSIX qw( :signal_h );

use Polymake::Struct (
   [ '@ISA' => 'Selector::Member' ],
   [ '$term' => 'Term::ReadLine::Gnu->new("polymake")' ],
   [ '$state' => '0' ],
   '$input_line',               # for callback interface
   '$line_cnt',                 # in the current input group
   '$first_hist_index',         # where in the history list the first line of the current input portion will go
   [ '$within_history' => 'undef' ],    # when defined: position in the history list to be copied to the pre-put
   '$within_history_begin',     # the index of the history line preceding the injected portion (for display in prompt)
   '$last_error_was_in_history',
   '&try_completion',           # method choosing the completion list
   '&completion',               # method iterating over the completion list
   '@completion_words',
   [ '$histfile' => '"$PrivateDir/history"' ],
   '@remove_hist_lines',        # [ start..end ], ...
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

# stuff for TAB completion and context-sensitive help

my $statement_start_re=qr{(?: ^ | [;\}] )\s*}x;

my $args_start_re=qr{(?: \s+ | \s*\(\s* )}x;

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
   undef $Shell->term->{MinLength};     # don't automatically add to history

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
      $_ .= "#line 1 \"input\"\n";
   } elsif ($self->state!=3 || ($l=line_continued())) {
      namespaces::temp_disable();
      unless ($User::application->declared & 4) {
         User::show_credits(1);
         if ($User::application->declared & 8) {
            print <<'.';

Warning: some rulefiles could not be configured automatically
due to lacking third-party software and/or other issues.
To see the complete list: show_unconfigured;
.
         }
         $User::application->declared |= 4;
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
   my $vor=length($prefix)-length($word);
   my $dir_seen;
   if ($dir_cmd) {
      unshift @$list, ".." if length($prefix)==0 || $prefix =~ /^\.\.?$/;
      $self->completion_words=[ map { substr($_,$vor) } grep { m|^~/| ? -d "$ENV{HOME}/$'" : -d $_ } @$list ];
      $self->term->Attribs->{completion_append_character}="/";
   } else {
      $self->completion_words=[ map { substr($_,$vor).( m|^~/| ? -d "$ENV{HOME}/$'" : -d $_ and $dir_seen="/") } @$list ];
      $self->term->Attribs->{completion_append_character}=$quote unless $dir_seen;
   }
   $self->term->Attribs->{filename_completion_desired}=0;
}

sub try_filename_completion {
   my ($self, $word, $quote, $prefix, $dir_cmd)=@_;
   if (length($prefix) > length($word)) {
      if (my @list=$self->term->completion_matches($prefix,$self->term->Attribs->{filename_completion_function})) {
         shift @list if $#list;
         postprocess_file_list(@_,\@list);
      }
   } else {
      $self->completion=$self->term->Attribs->{filename_completion_function};
   }
}

sub matching_rulefiles {
   my ($app, $cmd, $prefix)=@_;
   if ($cmd eq "include") {
      map { $_ =~ $filename_re } map { glob("$_/$prefix*") } @{$app->rulepath}
   } else {
      grep { /^\Q$prefix\E/ } $app->list_configured($cmd eq "unconfigure")
   }
}

sub try_property_completion {
   my ($type, $prefix, $allow_auto_cast)=@_;
   my @path=split /\./, $prefix;
   $prefix= substr($prefix,-1,1) eq "." ? "" : pop @path;
   foreach (@path) {
      my $prop=$type->lookup_property($_) || ($allow_auto_cast && $type->lookup_auto_cast($_)) or return;
      instanceof ObjectType($type=$prop->specialization($type)->type) or return;
   }
   sorted_uniq(sort( grep { !$type->is_overridden($_) } grep { /^$prefix/ } map { keys %{$_->properties} }
                     map { ($_, $allow_auto_cast ? keys %{$_->auto_casts} : ()) } $type, @{$type->super} ));
}

sub try_type_completion {
   my ($self, $word, $prefix, $before, $is_object_type)=@_;
   my @list;
   my $char="";
   my $maybe_object_type= !( $prefix =~ s/\bprops::// ) && (!defined($is_object_type) || $is_object_type);
   my $maybe_prop_type= !( $prefix =~ s/\bobjects::// ) && !$is_object_type;
   my $has_app_prefix= $prefix =~ s/^($id_re):://;
   my $app= $has_app_prefix ? eval { User::application($1) } || return ($User::application, $char) : $User::application;
   if ($maybe_object_type) {
      push @list, grep { /^$prefix/ } map { $_->name } map { @{$_->object_types} }
                  ($app, $has_app_prefix ? () : values %{$app->used});
   }
   if ($maybe_prop_type) {
      push @list, map { /^(.*)::$/ } map { complete_variable_name_in_pkg(get_pkg($_->pkg."::props"), " ", $prefix) }
                  ($app, $has_app_prefix ? () : values %{$app->used});
   }
   @list=sorted_uniq(sort(@list));

   my $vor=length($word)-length($prefix);
   if ($vor>0) {
      substr($_,0,0)=substr($word,0,$vor) for @list;
   }

   if ($#list==0) {
      my $pkg=namespaces::lookup_class($app->pkg, $list[0]);
      if (do { no strict 'refs'; exists &{"$pkg\::_min_params"} }) {
         $pkg->_min_params and $char="<";
      } else {
         $char="(";
         if (length($before)) {
            my $cnt=1;
            ++$cnt while $before =~ s/$type_re \s*,\s* $//xo;
            if (my ($outer_type)= $before =~ /($qual_id_re) \s*<\s* $/xo) {
               $app= $outer_type =~ s/^(?! props:: | objects::) ($id_re)::// ? (add Application($1)) : $User::application;
               $pkg=namespaces::lookup_class($app->pkg, $outer_type);
               if (do { no strict 'refs'; exists &{"$pkg\::_min_params"} }) {
                  $char= $pkg->_min_params <= $cnt ? ">" : ",";
               }
            }
         }
      }
   } elsif (!$vor) {
      push @list, map { "$_\::" } grep { /^$prefix/ } known Application;
   }

   $self->completion_words=\@list;
   $self->term->Attribs->{completion_append_character}=$char;

   ($app, $char);
}

sub try_label_completion {
   my ($app, $expr)=@_;
   sorted_uniq(sort( map { $_->list_completions($expr) } $app->prefs, @{$app->prefs->imported} ))
}

# the term is NOT a method call, if preceded by:
my $op_re=qr{(?:[-+*/.,<=;({}\[]| (?<!-)> | ^ )\s* | \w\s+ }x;

# a variable or array element
my $var_re=qr{ \$ $id_re (?:(?=\s*\[) $confined_re)? }xo;

# start of a method call chain: a variable or a function call
my $var_or_func_re=qr{ (?'var' $var_re) | (?:(?'app_name' $id_re)::)?(?'func' $id_re) (?(?=\s*\() $confined_re)? }xo;

# intermediate expression in a method call chain
my $intermed_re=qr{ \s*->\s* (?: (?'method_name' $id_re) (?(?=\s*\() $confined_re)? | (?(?=\s*\[) $confined_re ) ) }xo;

# chain of intermediate expressions
my $intermed_chain_re=qr{ (?'intermed' (?:$intermed_re)*?) \s*->\s* }xo;

my (%popular_perl_keywords, %object_methods);
@popular_perl_keywords{qw( local print require chdir rename unlink mkdir rmdir declare exit )}=();
@object_methods{qw( name description give take add remove type provide lookup list_properties properties schedule list_names
                    get_attachment remove_attachment list_attachments attach )}=();

sub all_completions : method {
   my ($self, $word)=@_;
   my $line=$self->term->Attribs->{line_buffer};
   pos($line)=$self->term->Attribs->{point};

   # commands expecting a filename argument
   if ($line =~
       m{(?: $statement_start_re (?: (?'simple_cmd' require | do | rename | unlink | mkdir | load_commands | save_history | export_configured) |
                                     (?'dir_cmd' chdir | rmdir | found_extension | import_extension)) $args_start_re
           | $op_re (?: load | load_data | glob ) (?'paren' $args_start_re)
           | $statement_start_re (?: save | save_data ) (?'paren' $args_start_re) $expression_re ,\s*
           | \b(?'File' File) \s*=>\s* )
         (?:(?'quote' ['"]) (?'prefix' [^"']*)? )? \G}xogc) {
      my ($simple_cmd, $dir_cmd, $paren, $File, $quote, $prefix)=@+{qw(simple_cmd dir_cmd paren File quote prefix)};
       
      if (defined $quote) {
         try_filename_completion($self, $word, $quote, $prefix, $dir_cmd);
      } else {
         $self->completion_words=[ $simple_cmd || $dir_cmd || $paren =~ /\(/ || $File ? '"' : '(' ];
      }
      return;
   }

   # command expecting an application name
   if ($line =~
       m{ $statement_start_re (extend_)?application $args_start_re (?(1) (?'ext_dir' $expression_re) ,\s*)
          (?:(?'quote' ['"]) (?'prefix' [^"']*)? )? \G}xogc) {

      my ($ext_dir, $quote, $prefix)=@+{qw(ext_dir quote prefix)};

      if (defined $quote) {
         $self->term->Attribs->{completion_append_character}=$quote;
         if ($ext_dir) {
            $ext_dir=eval $ext_dir or return;
            replace_special_paths($ext_dir);
            if ($ext_dir !~ m{^/} && defined (my $ext=$Extension::registered_by_URI{$ext_dir})) {
               $ext_dir=$ext->dir;
            }
         }
         $self->completion_words=[ sorted_uniq(sort( grep { not $ext_dir && -d "$ext_dir/apps/$_" }
                                                     map { $_ =~ $filename_re }
                                                     map { glob "$_/apps/$prefix*" }
                                                     $InstallTop, map { $_->dir } @Extension::active[$Extension::num_bundled .. $#Extension::active] )) ];
      } else {
         $self->completion_words=[ '"' ];
      }
      return;
   }

   # commands expecting a label
   if ($line =~
       m{ $statement_start_re (?'cmd' prefer(?:_now)? | (?:re)?set_preference ) $args_start_re
          (?:(?'quote' ['"]) (?: (?'app_name' $id_re) ::)? (?'expr' \*\.(?: $hier_id_re (?: \s+ (?: $ids_re (?:\s*,)? )? | \.? )? )? | $hier_id_re\.?)? )? \G}xogc) {

      my ($cmd, $quote, $app_name, $expr)=@+{qw(cmd quote app_name expr)};

      if (defined $quote) {
         if (defined $app_name) {
            $self->completion_words=[ map { "$app_name\::$_" } try_label_completion((eval { User::application($app_name) } or return), $expr) ];
         } else {
            $self->completion_words=[ try_label_completion($User::application, $expr) ];
            if (length($expr)==0 || $expr =~ /^$id_re$/o) {
               push @{$self->completion_words}, map { "$_\::" } grep { /^$expr/ && !$User::application->imported->{$_} } keys %{$User::application->used};
            }
         }
         if (@{$self->completion_words}==1 && $self->completion_words->[0] !~ /:$/) {
            $self->term->Attribs->{completion_append_character}=
              $cmd ne "reset_preference" && $expr =~ /^\*\.$hier_id_re\s*$/o
              ? " " : $quote;
         }
      } else {
         $self->completion_words=[ '"' ];
      }
      return;
   }

   # command expecting a configuration option for an extension
   if ($line =~
       m{ $statement_start_re (?:import|reconfigure)_extension $args_start_re (?'ext_dir' $expression_re) ,\s*
          (?: $expression_re ,\s* )* (?'quote' ['"])? (?'prefix' [^'"\s]*) \G}xogc) {

      my ($ext_dir, $quote, $prefix)=@+{qw(ext_dir quote prefix)};

      $ext_dir=eval $ext_dir or return;
      replace_special_paths($ext_dir);
      if ($ext_dir !~ m{^/} && defined (my $ext=$Extension::registered_by_URI{$ext_dir})) {
         return if $ext->is_bundled;
         $ext_dir=$ext->dir;
      } elsif (defined ($ext=$Extension::registered_by_dir{$ext_dir})) {
         return if $ext->is_bundled;
      }
      if (-f "$ext_dir/configure.pl") {
         if (defined $quote) {
            require Symbol;
            my (%allowed_options, %allowed_with, $err);
            {  package Polymake::StandaloneExt;
               do "$ext_dir/configure.pl";
               $err=$@;
            }
            unless ($err) {
               eval "Polymake::StandaloneExt::allowed_options(\\%allowed_options, \\%allowed_with)";
               $err=$@;
            }
            Symbol::delete_package("Polymake::StandaloneExt");
            return if $err;
            my $vor=length($prefix)-length($word);
            $self->completion_words=[ map { substr($_, $vor) }
                                      sort( grep { /^\Q$prefix\E/ }
                                            (map { "--$_" } keys %allowed_options),
                                            (map { ("--with-$_", "--without-$_") } keys %allowed_with) ) ];
            $self->term->Attribs->{completion_append_character}=$quote;
         } else {
            $self->completion_words=[ '"' ];
         }
      }
      return;
   }

   # command expecting a directory or URI of an extension
   if ($line =~
       m{ $statement_start_re (?:(?:obliterate|reconfigure)_extension | (?'app_cmd' found|extend)_application)
          $args_start_re (?: (?'quote' ['"]) (?'prefix' [^'"]*) )? \G}xogc) {

      my ($app_cmd, $quote, $prefix)=@+{qw(app_cmd quote prefix)};

      if (defined $quote) {
         my @list;
         my $vor=length($prefix)-length($word);
         my $match= $prefix =~ m{^/} ? "dir" : "URI";
         my @list= map { my $item=$_->$match;
                         ($app_cmd eq "extend" || !$_->is_bundled) && $item =~ /^\Q$prefix\E/ ? ($item) : ()
                       } values %Extension::registered_by_dir;

         if ($app_cmd eq "found" && index("core", $prefix)==0) {
            push @list, "core";
         }
         $self->completion_words=[ map { substr($_,$vor) } sort @list ];
         $self->term->Attribs->{completion_append_character}=$quote;
      } else {
         $self->completion_words=[ '"' ];
      }
      return;
   }

   # command expecting a rulefile
   if ($line =~
       m{ $statement_start_re (?'cmd' reconfigure|unconfigure|include)
          $args_start_re (?: (?'quote' ['"]) (?: (?'app_name' $id_re) ::)? (?'prefix' .*) )? \G}xogc) {

      my ($cmd, $quote, $app_name, $prefix)=@+{qw(cmd quote app_name prefix)};

      if (defined $quote) {
         my @list;
         if (defined $app_name) {
            if (defined (my $app=lookup Application($app_name))) {
               @list=map { "$app_name\::$_" } matching_rulefiles($app, $cmd, $prefix);
            }
         } else {
            @list=matching_rulefiles($User::application, $cmd, $prefix);
            while (my ($app_name, $app)=each %{$User::application->used}) {
               if ($app_name =~ /^\Q$prefix\E/ and
                   $cmd eq "include" ? glob($app->rulepath->[0]."/*") : $app->list_configured($cmd eq "unconfigure")) {
                  push @list, "$app_name\::";
               }
            }
         }

         my $vor=length($prefix)-length($word);
         $vor += length($app_name)+2 if defined($app_name);
         $self->completion_words=[ map { substr($_,$vor) } sort @list ];
         if ($#{$self->completion_words}==0 && $self->completion_words->[0] !~ /::$/) {
            $self->term->Attribs->{completion_append_character}=$quote;
         }
      } else {
         $self->completion_words=[ '"' ];
      }
      return;
   }

   # command expecting a script file
   if ($line =~
       m{ $op_re script (?'paren' $args_start_re) (?:(?'quote' ['"]) (?'prefix' [^"']*)? )? \G}xogc) {
       
      my ($paren, $quote, $prefix)=@+{qw(paren quote prefix)};
      
      if (defined $quote) {
         if ($prefix =~ m|^[./~]|) {
            try_filename_completion($self, $word, $quote, $prefix, 0);
         } else {
            my @list=$self->term->completion_matches($prefix, $self->term->Attribs->{filename_completion_function});
            foreach my $dir ((map { @{$_->scriptpath} } $User::application, values %{$User::application->imported}),
                             @User::lookup_scripts, "$InstallTop/scripts") {
               if (my @sublist=$self->term->completion_matches("$dir/$prefix", $self->term->Attribs->{filename_completion_function})) {
                  shift @sublist if $#sublist;
                  my $tail=length($dir)+1;
                  push @list, map { substr($_,$tail) } @sublist;
               }
            }
            if (@list) {
               postprocess_file_list($self, $word, $quote, $prefix, 0, \@list);
            }
         }
      } else {
         $self->completion_words=[ $paren =~ /\(/ ? '"' : '(' ];
      }
      return;
   }

   # methods expecting a property name
   if ($line =~
       m{ $op_re $var_or_func_re $intermed_chain_re
          (?: (?'mult_word' give | lookup | (?'mult_arg' provide | get_schedule)) | take | add | remove ) \s*\(\s*
          (?('mult_arg') (?: (?: '$hier_id_alt_re'|"$hier_id_alt_re" )\s*,\s*)* )
          (?:(?'quote' ['"]) (?('mult_word') (?: $hier_id_re \s*\|\s*)* ) (?'prefix' $hier_id_re\.?)?)? \G}xogc) {
      
      if (defined $+{quote}) {
         my ($var, $app_name, $func, $intermed, $mult_word, $mult_arg, $quote, $prefix)=@+{qw(var app_name func intermed mult_word mult_arg quote prefix)};

         if (defined( my $type=retrieve_method_owner_type($var, $app_name, $func, $intermed) )) {
            if (instanceof ObjectType($type)) {
               push @{$self->completion_words}, try_property_completion($type, $prefix, 1);
               $self->term->Attribs->{completion_append_character}=$quote;
            }
         }
      } else {
         $self->completion_words=[ '"' ];
      }
      return;
   }

   # additional properties of a subobject
   if ($line =~
       m{ $op_re $var_or_func_re $intermed_chain_re
          add \s*\(\s* (['"])(?'prop_name' $id_re)\g{-2} \s*,\s*  (?: $expression_re ,\s* )?
          (?: (?: $id_re|'$hier_id_re'|"$hier_id_re" ) \s*=>\s* $expression_re ,\s* )*
          (?'quote' ['"])? (?'prefix' $hier_id_re\.?)? \G}xogc) {

      my ($var, $app_name, $func, $intermed, $prop_name, $quote, $prefix)=@+{qw(var app_name func intermed prop_name quote prefix)};

      if (defined( my $type=retrieve_method_owner_type($var, $app_name, $func, $intermed) )) {
         if (instanceof ObjectType($type) && defined(my $prop=$type->lookup_property($prop_name))) {
            push @{$self->completion_words}, try_property_completion($prop->specialization($type)->type, $prefix);
            if ($quote) {
               $self->term->Attribs->{completion_append_character}=$quote;
            } else {
               $_ .= "=>" for @{$self->completion_words};
            }
         }
      }
      return;
   }

   # methods expecting an attachment name
   if ($line =~
       m{ $op_re $var_or_func_re $intermed_chain_re (?: get_attachment | remove_attachment )
          \s*\(\s* (?:(?'quote' ['"]) (?'prefix' $id_re)?)? \G}xogc) {

      my ($var, $app_name, $func, $intermed, $quote, $prefix)=@+{qw(var app_name func intermed quote prefix)};

      if (defined $quote) {
         if (($var)=retrieve_method_owner_type($var, $app_name, $func, $intermed)) {
            if (is_object($var) && instanceof Core::Object($var)) {
               $self->completion_words=[ sort( grep { /^$prefix/ } keys %{$var->attachments} ) ];
               $self->term->Attribs->{completion_append_character}=$quote;
            }
         }
      } else {
         $self->completion_words=[ '"' ];
      }
      return;
   }

   # methods expecting a label or rule header
   if ($line =~
       m{ $op_re $var_or_func_re $intermed_chain_re (?: disable_rules | apply_rule )
          \s*\(\s* (?:(?'quote' ['"]) (?'prefix' [^'"]*))? \G}xogc) {

      my ($var, $app_name, $func, $intermed, $quote, $prefix)=@+{qw(var app_name func intermed quote prefix)};

      if (defined $quote) {
         if (defined( my $type=retrieve_method_owner_type($var, $app_name, $func, $intermed) )) {
            if (instanceof ObjectType($type)) {
               my @list;
               if ($prefix =~ /^ (?: $hier_id_re (?:\.)? )? $/xo) {
                  # maybe a label
                  @list=sorted_uniq(sort( grep { /^\Q$prefix\E/ } map { $_->full_name } map { @{$_->labels} } map { @{$_->production_rules} } $type, @{$type->super} ));
               }
               my $vor=length($prefix)-length($word);
               Rule::prepare_header_search_pattern($prefix);
               push @list, sorted_uniq(sort( grep { /^$prefix/ } map { $_->header } map { @{$_->production_rules} } $type, @{$type->super} ));
               $self->completion_words=[ map { substr($_,$vor) } @list ];
               $self->term->Attribs->{completion_append_character}=$quote;
            }
         }
      } else {
         $self->completion_words=[ '"' ];
      }
      return;
   }

   # function expecting a label or rule header
   if ($line =~
       m{ $statement_start_re disable_rules  \s*\(\s* (?:(?'quote' ['"]) (?'prefix' [^'"]*))? \G}xogc) {

      my ($quote, $prefix)=@+{qw(quote prefix)};

      if (defined $quote) {
         my @list;
         if ($prefix =~ /^ (?: $hier_id_re (?:\.)? )? $/xo) {
            # maybe a label
            @list=sorted_uniq(sort( grep { /^\Q$prefix\E/ } map { $_->full_name } map { @{$_->labels} } @{$User::application->rules} ));
         }
         my $vor=length($prefix)-length($word);
         Rule::prepare_header_search_pattern($prefix);
         push @list, sorted_uniq(sort( grep { /^$prefix/ } map { $_->header } @{$User::application->rules} ));
         $self->completion_words=[ map { substr($_,$vor) } @list ];
         $self->term->Attribs->{completion_append_character}=$quote;
      } else {
         $self->completion_words=[ '"' ];
      }
      return;
   }

   # help topic
   if ($line =~
       m{ $statement_start_re help $args_start_re (?:(?'quote' ['"]) (?'path' .*/)? (?'prefix' [^/]*))? \G}xogc) {

      my ($quote, $path, $prefix)=@+{qw(quote path prefix)};

      if (defined $quote) {
         if (defined $path) {
            $self->completion_words=[ sorted_uniq(sort( map { $_->list_toc_completions($prefix) } $User::application->help->get_topics($path) )) ];
         } else {
            $self->completion_words=[ sorted_uniq(sort( $User::application->help->list_completions($prefix) )) ];
         }
         if (@{$self->completion_words}==1 &&
             grep { @{$_->toc} } $User::application->help->get_topics($path.$self->completion_words->[0])) {
            $quote="/";
         }
         if ((my $vor=length($prefix)-length($word))>0) {
            substr($_,0,$vor)="" for @{$self->completion_words};
         }
         $self->term->Attribs->{completion_append_character}=$quote;
      } else {
         $self->completion_words=[ "'" ];
      }
      return;
   }

   # a qualified variable name
   if ($line =~
       m{ (?: $statement_start_re (?'cmd' local | (?:re)?set_custom) $args_start_re)?
          (?'type' (?'scalar'\$)|[\@%]) (?: (?'varname' $qual_id_re(?'qual'::)?)
                                        (?('scalar') (?('qual') | \s*\{\s* (?'keyquote' ['"])?(?'key' [\w-]*) )?) )? \G}xogc) {

      my ($cmd, $type, $varname, $keyquote, $key)=@+{qw(cmd type varname keyquote key)};

      if (defined $key) {
         # completing a hash key
         my $i=0;
         my @list=grep { defined } map { $_->custom->find("%$varname", ++$i >= 3) }
                                       $User::application, $Prefs, values %{$User::application->imported};
         if (@list) {
            $self->completion_words=[ sorted_uniq(sort( grep { index($_,$key)==0 } map { keys %{$_->default_value} } @list)) ];
         } elsif (defined (my $user_hash=find_user_variable('%',$varname))) {
            $self->completion_words=[ sort( grep { index($_,$key)==0 } keys %$user_hash ) ];
         }

         if ($#{$self->completion_words}==0) {
            $self->term->Attribs->{completion_append_character}=$keyquote || '}';
         }

      } else {
         # completing variable name
         my $i=0;
         my @list=map { $_->custom->list_completions($type, $varname, ++$i >= 3) }
                      $User::application, $Prefs, values %{$User::application->imported};
         if ($cmd eq "" || $cmd eq "local"
               and
             !@list || $varname !~ /::/) {
            push @list, complete_user_variable_name($type,$varname);
         }
         $self->completion_words=[ sort(@list) ];
         if ($cmd && $#{$self->completion_words}==0 && substr($self->completion_words->[0],-1) ne ":") {
            $self->term->Attribs->{completion_append_character}= $cmd eq "reset_custom" ? ";" : "=";
            if ($type eq '$') {
               foreach $type (qw(% @)) {
                  $i=0;
                  if (grep { defined } map { $_->custom->find($type.$self->completion_words->[0], ++$i >= 3) }
                                           $User::application, $Prefs, values %{$User::application->imported}) {
                     $self->term->Attribs->{completion_append_character}= $type eq '%' ? "{" : "[";
                     return;
                  }
               }
            }
         }
      }
      return;
   }

   # a method call, possible in a longer chain
   if ($line =~
       m{ $op_re $var_or_func_re $intermed_chain_re
          (?: (?'last_method_name' $id_re) (?: (?'paren' \s*\() (?: (?'preceding_args' (?:$expression_re ,\s*)*) | (?'quote' ['"])) (?'prefix' $id_re)? )? )? \G}xogc) {

      my ($var, $app_name, $func, $intermed, $last_method_name, $paren, $preceding_args, $quote, $prefix)=@+{qw(
           var   app_name   func   intermed   last_method_name   paren   preceding_args   quote   prefix )  };

      if (($var, my $type)=retrieve_method_owner_type($var, $app_name, $func, $intermed)) {
         if (defined $paren) {
            if (instanceof ObjectType($type) && defined (my $prop=$type->lookup_property($last_method_name))) {
               if ($prop->flags & $Property::is_multiple) {
                  if (defined $quote) {
                     # name of a subobject
                     if (defined($var)) {
                        $self->completion_words=[ sorted_uniq(sort( grep { defined($_) && index($_, $prefix)==0 } map { $_->name } @{$var->give($last_method_name)} )) ];
                        $self->term->Attribs->{completion_append_character}=$quote;
                     }
                  } else {
                     # property for selecting a subobject
                     $self->completion_words=[ map { "$_=>" } try_property_completion($prop->specialization($type)->type, $prefix) ];
                  }
               }
               if (!defined($quote) && index("temporary", $prefix)==0) {
                  push @{$self->completion_words}, "temporary";
               }

            } else {
               # completing the keyword argument
               my $n_preceding_args=0;
               ++$n_preceding_args while $preceding_args =~ /\G $expression_re ,\s* /xog;
               my @list=map { $_->keyword_completions($n_preceding_args, $type, $prefix) }
                            retrieve_method_topics($type, $last_method_name, "find");
               $self->completion_words=[ map { "$_=>" } sorted_uniq(sort(@list)) ];
            }

         } else {
            # completing the method name
            $prefix=$last_method_name;
            my @list=retrieve_method_topics($type, $prefix, "list_completions");
            if (ref($type)) {
               if (instanceof ObjectType($type)) {
                  @list=( (grep { !$type->is_overridden($_) } @list),
                          grep { index($_, $prefix)==0 } keys %object_methods );
               } elsif ($type->cppoptions && $type->cppoptions->fields) {
                  push @list, grep { index($_, $prefix)==0 } @{$type->cppoptions->fields};
               }
            }

            $self->completion_words=[ sorted_uniq(sort(@list)) ];
            if ($#{$self->completion_words}==0 && ref($type) &&
                $self->completion_words->[0] !~ /^name|description|list_attachments$/ &&
                exists $object_methods{$self->completion_words->[0]}) {
               $self->term->Attribs->{completion_append_character}="(";
            }
         }
      }
      return;
   }

   # a constructor call with property names?
   if ($line =~
       m{ \bnew \s+ (?'type' $type_re) \s*\(\s* (?: $expression_re ,\s* )* (?: (?'prop_name' $id_re) | (?'quote' ['"]) (?'chain' $hier_id_re \.?)? )? \G}xogc) {

      my ($type, $prop_name, $quote, $chain)=@+{qw(type prop_name quote chain)};

      if (defined ($type=eval { $User::application->eval_type($type) })
            and
          instanceof ObjectType($type)) {
         $self->completion_words=[ try_property_completion($type, $prop_name || $chain) ];
         if (defined $prop_name) {
            $_ .= "=>" for @{$self->completion_words};
         } else {
            $self->term->Attribs->{completion_append_character}=$quote;
         }
         return;
      }
   }

   # a type expression
   if ($line =~
       m{ $op_re (?'cmd' new|typeof|instanceof) \s+ (?'before' (?: $qual_id_re \s*[<>,]\s* )*) (?'prefix' $qual_id_re (?: ::)?)? \G}xogc) {

      my ($cmd, $before, $prefix)=@+{qw(cmd before prefix)};

      if ($before =~ /[<>]/ && $before !~ /,\s*$/ && $before =~ /^ $balanced_re $/xo) {
         if (length($prefix)==0 && $cmd ne "typeof") {
            $self->completion_words=[ "(" ];
         }
      } elsif (try_type_completion($self, $word, $prefix, $before) eq ">" &&
               $cmd ne "typeof" &&
               $line !~ m{\G \s*>}xgc) {
         $self->completion_words->[0].=">";
         $self->term->Attribs->{completion_append_character}="(";
      }
      return;
   }

   # a function with explicit template arguments
   if ($line =~
       m{ $op_re (?'cmd' $qual_id_re) \s*<\s* (?'before' (?: $qual_id_re \s*[<>,]\s* )*) (?'prefix' $qual_id_re (?: ::)?)? \G}xogc) {

      my ($cmd, $before, $prefix)=@+{qw(cmd before prefix)};

      if ($before =~ /[^\s,]\s*$/ && $before =~ /^ $balanced_re \s*>\s* $/xo) {
         if (length($prefix)==0) {
            $self->completion_words=[ "(" ];
         }
      } else {
         # FIXME: don't list the functions expecting ObjectType literally (how to detect ???)
         my $only_objects= $cmd eq "cast" && $before !~ /\S/ || undef;
         my ($app, $char)=try_type_completion($self, $word, $prefix, $before, $only_objects);
         if ($char eq ">" || $char eq "(") {
            # type complete: check for the number of parameters
            my $h=$app->help->find("functions", $cmd);
            my $cnt=defined($h) && $h->expects_template_params;
            $self->completion_words->[0].=">" if $char eq ">";
            my $given=$before.$self->completion_words->[0];
            --$cnt while $cnt>0 && $given =~ /\G $type_re \s* (?: ,\s* | $)/gox;
            if ($cnt>0) {
               $self->term->Attribs->{completion_append_character}=",";
            } elsif ($line !~ m{\G \s*>}xgc) {
               $self->completion_words->[0].=">";
               $self->term->Attribs->{completion_append_character}="(";
            }
         }
      }
      return;
   }

   # symbolic color name
   if ($line =~
       m{(?: \$Visual::Color::$id_re \s*=\s* | Color \s*=>\s* ) (?:(?'quote' ['"])(?!\d)(?'prefix' [^"']*))? \G}xogc) {

      my ($quote, $prefix)=@+{qw(quote prefix)};

      if (defined $quote) {
         $self->completion_words=[ sort( Visual::list_color_completions($prefix) ) ];
         if ((my $vor=length($prefix)-length($word))>0) {
            substr($_,0,$vor)="" for @{$self->completion_words};
         }
         $self->term->Attribs->{completion_append_character}=$quote;
      } else {
         $self->completion_words=[ '"' ];
      }
      return;
   }

   # a user function or "popular" perl function
   if ($line =~
       m{ $op_re (?:(?'app_name' $id_re)::)?(?'func' $id_re) (?:(?=\s*<) $confined_re)? (?: (?'paren' \s*\()
                 (?'preceding_args' (?:$expression_re ,\s*)*) (?'prefix' $id_re)? )? \G}xogc) {

      my ($app_name, $func, $paren, $preceding_args, $prefix)=@+{qw(app_name func paren preceding_args prefix)};

      my $app= defined($app_name) ? (eval { User::application($app_name) } || return) : $User::application;
      my @list;
      if (defined $paren) {
         # completing the keyword argument
         my $n_preceding_args=0;
         ++$n_preceding_args while $preceding_args =~ /\G $expression_re ,\s* /xog;
         if (@list=map { $_->keyword_completions($n_preceding_args, $app, $prefix) }
                       $app->help->find((defined($app_name) ? "!rel" : ()), "functions", $func)) {
            $self->completion_words=[ map { "$_=>" } sorted_uniq(sort(@list)) ];
            return;
         } elsif (length($prefix)) {
            # maybe a nested function call?
            undef $app_name;
            $app=$User::application;
            $func=$prefix;
         } else {
            return;
         }
      }

      # completing the function name
      if (defined $app_name) {
         push @list, map { "$app_name\::$_" } $app->help->list_completions("!rel", "functions", $func);
      } else {
         push @list, $app->help->list_completions("functions", $func);
         if ($line =~ m{ $statement_start_re $id_re \G}xogc) {
            push @list, grep { index($_, $func)==0 } keys %popular_perl_keywords;
         }
      }
      $self->completion_words=[ sorted_uniq(sort(@list)) ];

      if ($#{$self->completion_words}==0) {
         my $cmd=$self->completion_words->[0];
         my $h=$app->help->find("functions", $cmd);

         $self->term->Attribs->{completion_append_character}=
           $cmd =~ /^(?:exit|history|replay_history|show_(?:preferences|unconfigured|credits|extensions))$/
           ? ";" :
           defined($h) && $h->expects_template_params
           ? "<" :
           exists $popular_perl_keywords{$cmd} ||
           $cmd =~ /^(?:application|help|apropos|include|prefer(?:_now)?|(?:re)?set_(?:custom|preference)|(?:re|un)configure)$/
           ? " " : "(";
      }
      return;
   }

	# try tab completion rules defined in extensions	
	foreach (@Core::Shell::custom_tab_completion) {
		if ($_->($self, $line, $word)) {
			return;
		}
	}


   if ($line =~
       m{((?<!\w)['"]) ([^"']*) \G}xogc) {
      # if nothing else worked out, assume a file name
      try_filename_completion($self, $word, $1, $2, 0);
   }
}

sub complete_variable_name_in_pkg {
   my ($pkg, $type, $prefix)=@_;
   my @answer;
   while (my ($name, $glob)=each %$pkg) {
      if (length($prefix) ? index($name,$prefix)==0 : $name !~ /^\./) {
         if (defined_scalar($glob) && $type eq '$' or
             defined(*{$glob}{ARRAY}) && ($type eq '@' || $type eq '$') or
             defined(*{$glob}{HASH}) && ($type eq '%' || $type eq '$' || $name =~ /::$/)) {
            push @answer, $name;
         }
      }
   }
   @answer;
}

sub complete_user_variable_name {
   my ($type, $prefix)=@_;
   if ((my $pkg_end=rindex($prefix,"::"))>0) {
      my $pkg_part=substr($prefix,0,$pkg_end);
      my $pkg=get_pkg("Polymake::User::$pkg_part") or return ();
      map { "$pkg_part\::$_" } complete_variable_name_in_pkg($pkg,$type,substr($prefix,$pkg_end+2));
   } else {
      complete_variable_name_in_pkg(\%Polymake::User::,@_);
   }
}

sub find_user_variable {
   my ($type, $name)=@_;
   my $glob=do {
      if ((my $pkg_end=rindex($name,"::"))>0) {
         my $pkg=get_pkg("Polymake::User::" . substr($name,0,$pkg_end)) or return;
         $pkg->{substr($name,$pkg_end+2)};
      } else {
         $Polymake::User::{$name};
      }
   } or return;

   $type eq '%'
   ? *{$glob}{HASH} :
   $type eq '@'
   ? *{$glob}{ARRAY} :
   $type eq '$'
   ? *{$glob}{SCALAR}
   : $glob
}

sub retrieve_return_type {
   my $name=(shift)->return_type;
   if (defined($name) and $name !~ /::/) {
      eval { $User::application->eval_type($name) }
   } else {
      $name
   }
}

sub retrieve_method_owner_type {
   my ($var, $app_name, $func, $intermed)=@_;
   my ($app, $type, $topic, @how);

   if (defined($var)) {
      package Polymake::User;
      $type=eval "$var->type"
        and
      $var= length($intermed) ? undef : eval $var;
   } else {
      if (defined $app_name) {
         $app=eval { User::application($app_name) } or return;
         @how=qw(!rel);
      } else {
         $app=$User::application;
      }
      if (defined ($func=$app->help->find(@how, "functions", $func))) {
         if (!defined ($type=retrieve_return_type($func)) &&
             $func->parent->category &&
             $func->parent->name =~ /^\s* (?: producing | transforming ) /xi) {
            $type=$app->default_type;
         }
      }
   }

   if (defined($type)) {
      my $prop;
      while ($intermed =~ m{\G $intermed_re }gxo) {
         if (defined (my $method_name=$+{method_name})) {
            undef $prop;
            if (instanceof ObjectType($type) and defined ($prop=$type->lookup_property($method_name, 1))) {
               $type=$prop->specialization($type)->type;
            } elsif (defined ($func= ref($type) ? ($topic=$type->help_topic and $topic->find("methods", $method_name))
                                                : ($topic=$User::application->help->find("objects", $type)) and $topic->find("methods", $method_name))) {
               if (defined (my $ret_type=retrieve_return_type($func))) {
                  $type=$ret_type;
               }
               # otherwise let's just suppose the method returns the same object
            } else {
               ref($type) or defined ($type=eval { $User::application->eval_type($type) }) or return;
               if (instanceof PropertyType($type)) {
                  $type=$type->get_field_type($method_name) or return;
               } else {
                  return;
               }
            }
         } elsif (not defined($prop) && $prop->flags & $Property::is_multiple) {
            # a bracketed index expression
            $type=$type->get_element_type or return;
         }
      }
   }

   ($var, $type)
}

sub retrieve_method_topics {
   my ($type, $name, $help_method)=@_;
   if (ref($type)) {
      if (instanceof ObjectType($type)) {
         return map { $_->$help_method("!rel", "properties", "methods", $name) }
                    ( grep { defined } map { $_->help_topic }
                      ($type, @{$type->super}, keys %{$type->auto_casts}) ),
                    $type->application->help->find(qw(objects Core::Object));
      }

      my @list;
      do {
         if (defined (my $topic=$type->help_topic)) {
            push @list, $topic->$help_method("!rel", "methods", $name);
         }
         $type=$type->super;
      } while (defined $type);
      return @list;
   }

   no strict 'refs';
   map {
      map { $_->$help_method("!rel", "methods", $name) } $User::application->help->find("objects", $_)
   } $type, @{"$type\::ISA"};
}

sub guess_topics_from_context {
   my $line=shift;
   my ($var, $app_name, $func, $intermed, $app, $name, @how);
   my $match=0;

   if ($line =~
       m{ (?: (?: $op_re (?:new|typeof|instanceof)) \s+ | [<,] \s* ) (?:(?'app_name' $id_re)::)?(?'name' $id_re) \s* $ }xo) {

      ($app_name, $name)=@+{qw(app_name name)};

      if (defined($app_name)) {
         $app=eval { User::application($app_name) } or return;
         @how=qw(!rel);
      } else {
         $app=$User::application;
      }
      if (my @topics=$app->help->find(@how, "objects", "property_types", $name)) {
         return @topics;
      }
   }

   if ($line =~
       m{ (?:re)?set_custom \s+ (?'var' [\$\@%] $qual_id_re) \s* (?(?=[\{\[]) (?'elem' $confined_re) \s* )? (?: = (?:$balanced_re)?)? $}xo) {

      my ($var, $elem)=@+{qw(var elem)};

      if (substr($elem,0,1) eq '[') {
         substr($var,0,1)='@';
      } elsif (substr($elem,0,1) eq '{') {
         substr($var,0,1)='%';
      }
      if (my @topics=$User::application->help->find($var)) {
         return @topics;
      }
   }

   # Find the rightmost match.  This operation can't be expressed directly in one regexp, hence the loop.
   while ($line =~ m{ $op_re $var_or_func_re $intermed_chain_re (?'name' $id_re) \s*
                      (?: \( (?'tail' $open_balanced_re) )? $ }xogc) {
      $match=1;
      ($var, $app_name, $func, $intermed, $name, $line)=@+{qw(var app_name func intermed name tail)};
      defined($line) or last;
   }
   if ($match) {
      if (defined (my $type=retrieve_method_owner_type($var, $app_name, $func, $intermed))) {
         return retrieve_method_topics($type, $name, "find");
      } else {
         return;
      }
   }

   while ($line =~
          m{ $op_re (?:(?'app_name' $id_re)::)?(?'name' $id_re) (?:(?=\s*<) $confined_re)? (?: \s*\( (?'tail' $open_balanced_re) )? $ }xogc) {
      $match=1;
      ($app_name, $name, $line)=@+{qw(app_name name tail)};
      defined($line) or last;
   }
   if ($match) {
      if (defined($app_name)) {
         $app=eval { User::application($app_name) } or return;
         @how=qw(!rel);
      } else {
         $app=$User::application;
      }
      return $app->help->find(@how, "functions", $name);
   }

   return;
}

my ($last_F1_line, $last_F1_point);

sub context_help {
   print "\n";
   my $help_delim=$User::help_delimit? "-------------------\n" : '';
   my $line=$Shell->term->Attribs->{line_buffer};
   if ($line =~ /\S/) {
      my $pt=pos($line)=$Shell->term->Attribs->{point};
      my $full_text= $last_F1_point == $pt && $last_F1_line eq $line;

      # move before the end of the statement if nothing follows
      $line =~ m{(?: \)\s* (?: ;\s*)? | \s*;\s* ) \G \s*$ }xgc and pos($line)=$-[0];

      # move before the trailing spaces
      $line =~ m{\s+ \G}xgc and pos($line)=$-[0];

      # move behind the name if currently pointing in its middle
      $line =~ m{($qual_id_re)? \G (?(1) [\w:]* | $qual_id_re) (?: \s*\( )?}xgc;

      my @topics;
      until (@topics=guess_topics_from_context(substr($line, 0, pos($line)))) {
         # try to move behind the next name
         $line =~ m{\G .*? (?<! [\$\@%]) $id_re (?: \s*\( )?}xgc or last;
      }

      if (@topics) {
         my $n=0;
         my $tell_about_full;
         foreach (@topics) {
            print $n++>0 && $help_delim,
                  $full_text && substr($_->full_path,1).":\n",
                  $_->display_text($full_text, $tell_about_full);
         }
         if ($tell_about_full) {
            print "Press help key again for more details\n";
         } elsif ($full_text && $topics[0]->name =~ /^(?:import|reconfigure)_extension$/) {
            if ($line =~ /$& $args_start_re (?| ' ($single_quoted_re) ' | " ($double_quoted_re) " ) [^;]* \G/xgc) {
               display_config_options($1);
            }
         }
         $last_F1_line=$line;
         $last_F1_point=$pt;
      } else {
         print "Sorry, no suitable context help found; try moving cursor towards a name of interest.\n";
      }

   } else {
      User::help();
   }

   STDOUT->flush;
   $Shell->term->on_new_line;
   $Shell->term->redisplay;
}
###############################################################################################
sub display_config_options {
   my $ext_dir=shift;
   replace_special_paths($ext_dir);
   if ($ext_dir !~ m{^/} && defined (my $ext=$Extension::registered_by_URI{$ext_dir})) {
      return if $ext->is_bundled;
      $ext_dir=$ext->dir;
   } elsif (defined ($ext=$Extension::registered_by_dir{$ext_dir})) {
      return if $ext->is_bundled;
   }
   if (-f "$ext_dir/configure.pl") {
      require Symbol;
      package Polymake::StandaloneExt;
      do "$ext_dir/configure.pl";
      unless ($@) {
         print STDERR "\nFollowing configuration options are possible:\n";
         eval { usage() };
      }
      Symbol::delete_package("Polymake::StandaloneExt");
   }
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
   my $help_delim=$User::help_delimit? "-------------------\n" : '';
   if (my @topics=uniq( $User::application->help->get_topics($subject || "top") )) {
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
            @topics=$User::application->help->find($subject)) {

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
         my ($first_shot, $full_path, $error)=find_program_in_path($response, $check_code ? $check_code : ());
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
   [ '$pipe' => 'new Pipe( #1 )' ],
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
