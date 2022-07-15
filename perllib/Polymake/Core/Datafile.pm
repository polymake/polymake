#  Copyright (c) 1997-2022
#  Ewgenij Gawrilow, Michael Joswig, and the polymake team
#  Technische Universität Berlin, Germany
#  https://polymake.org
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
use feature 'state';

package Polymake::Core::Datafile;

use Polymake::Struct (
   [ new => '$%' ],
   [ '$filename' => 'Cwd::abs_path(#1)' ],
   '$is_compressed',
   [ '$canonical' => '#%', default => 'undef' ],
);

sub new {
   my $self = &_new;
   enforce_canonical($self);
   $self
}

sub load {
   my $self = &_new;
   open my $fh, "<".layer_for_compression($self), $self->filename
     or die "can't open file ".$self->filename.": $!\n";

   my $data;
   my %flags = (changed => false, filename => $self->filename);
   local $_;
   while (<$fh>) {
      if (/^\s*\{/) {
	 { local $/; $_ .= <$fh>; }
	 local $PropertyType::trusted_value = 1;
         my $decoded = decode_json($_);
	 $data = Serializer::deserialize($decoded, \%flags);
         $self->canonical = $decoded->{_canonical};
	 last;
      }
      if (/^\s*<\?xml/) {
         seek($fh, 0, 0);
	 require Polymake::Core::XMLtoJSON;
	 local $PropertyType::trusted_value = 0;
	 $data = Serializer::deserialize(XMLtoJSON::from_filehandle($fh), \%flags);
	 last;
      }
      if (/\S/) {
         require Polymake::Core::PlainFile;
         my $plf = new PlainFile($self->filename);
         if ($Verbose::files) {
            dbg_print( "upgrading ", $self->filename, " from old plain file format" );
         }
         return $plf->load($fh, $self);
      }
   }
   if (!$self->canonical && enforce_canonical($self)) {
      $flags{changed} = true;
   }
   if (instanceof BigObject($data)) {
      $data->persistent = $self;
      if ($flags{changed}) {
         $data->set_changed;
      }
   } elsif ($flags{changed}) {
      save($self, $data);
   }
   $data
}
#############################################################################################
sub from_string {
   local ($_) = @_;
   if (/\A\s*\{/s) {
      return Serializer::deserialize(decode_json($_));
   }
   if (/\A\s*<\?xml/s) {
      require Polymake::Core::XMLtoJSON;
      return Serializer::deserialize(XMLtoJSON::from_string($_));
   }
   die "unrecognized input string: JSON or XML expected\n";
}
#############################################################################################
sub layer_for_compression {
   my ($self) = @_;
   if ($self->filename =~ /\.gz$/) {
      require PerlIO::via::gzip;
      $self->is_compressed = true;
      ":via(gzip)";
   } else {
      ""
   }
}
#############################################################################################
sub enforce_canonical {
   my ($self) = @_;
   $self->canonical //= $self->filename =~ m{/(?: testsuite | testscenarios) (?: /[^/]+ ){1,2} /[^/]* $}x;
}
#############################################################################################
sub save {
   my ($self, $data, %options) = @_;
   $options{save} = true;
   $options{pretty} = $self->canonical;
   $options{name_proposal} = sub {
      my ($obj) = @_;
      my ($name) = $self->filename =~ $filename_re;
      if (length(my $suffix = $obj->default_file_suffix)) {
         $name =~ s/\.$suffix$//;
      }
      $name
   };
   my $serialized = Serializer::serialize($data, \%options);
   my $compress = layer_for_compression($self);
   my $encoder = JSON->new->utf8;
   if ($self->canonical) {
      $encoder->canonical->indent->space_after;
      $serialized->{_canonical} = true;
   }
   my ($of, $of_k) = new OverwriteFile($self->filename, $compress);
   $encoder->write($serialized, $of);
   close $of;
}
#############################################################################################
1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
