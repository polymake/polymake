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

package Polymake::Core::PlainFile;

#################################################################################
#
#  Constructor: new PlainFile('filename');
#
use Polymake::Struct (
   [ new => '$' ],
   [ '$filename' => '#1' ],
   '$line',
   '@errors',
   '$unknown_seen',
);

#################################################################################
sub load {
   my ($self, $fh, $persistent)=@_;
   my $app = $User::application;
   my ($app_name, $proto, $file_version, $object, $subobject);

   # determine the application
   # first, try at the file begin (since v2.0)
   seek($fh, 0, 0);
   while (<$fh>) {
      s/^\s+//;
      if (/^ _application \s+ ($id_re) \s*$/xo) {
         $app_name = $1;
         last;
      }
      if (/^ _version \s+ (\S+) \s*$/x) {
         # it is probably v1.4
         $file_version = eval "v$1";
         last;
      }
      if (substr($_, 0, 1) eq '#') {
         next;
      }
      if (/\S/) {
         # try at the file end (v1.5)
         seek($fh, -40, 2);
         local $/;
         $_ = <$fh>;
         if (/^\s* _application \s+ ($id_re) \s*$/xmo) {
            $app_name = $1;
         }
         if (/^\s* _version \s+ (\S+) \s*$/xm) {
            $file_version = eval "v$1";
         }
         seek($fh, 0, 0);
         $. = 1;
         last;
      }
   }

   if (defined($file_version) && $file_version lt v1.5) {
      $app_name = "polytope";
   }
   if (defined($app_name) and !defined($app) || $app->name ne $app_name) {
      $app = add Application($app_name);
   }
   $app->include_rules("*/upgrade-plain.rules");

   local ref *PropertyType::parse_error = \&value_parse_error;

 INPUT:
   while (<$fh>) {
      s/^\s+//;
      next if $_ eq "";
      if (substr($_, 0, 1) eq '#') {
         next;
      }

      if (/^(_?dependenci?es)\s*$/i) {
         while (<$fh>) {        # skip the DEPENDENCES from old versions
            last unless /\S/;
         }
      } elsif (/^ _version \s+ (\S+) \s*$/x) {
         $file_version = eval "v$1";
      } elsif (/^ _application \s+ ($id_re) \s*$/xo) {
         next;
      } elsif (/^ _type \s+ ($type_re) \s*$/xo) {
         my $type = $1;
         $self->line = $.;
         ($proto, my $default_type) = $app->pkg->translate_plain_file_type($type);
         $proto //= do {
            if ($@) {
               chomp $@;
               report_warning($self, "$@, default type '", $default_type->name, "' assumed");
               $@ = "";
            }
            $default_type;
         };
         $object = $proto->construct->();
         next;

      } elsif (my ($negated, $prop_name, $attr)=/^(!)? ($id_re) (?(1) | (\([\w+]*\))?) \s*$/xo) {
         $self->line = $.;

         unless ($proto) {
            $proto = $app->pkg->translate_plain_file_type();
            $object = $proto->construct->();
         }

         my $value = "";
         my $boolean_value;
         while (<$fh>) {
            next if /^\s*\#/;
            if (/\S/) {
               $value .= $_;
            } else {
               last;
            }
         }

         if ($prop_name eq "DESCRIPTION") {
            $object->description = $value;
            next;
         }

         if ($negated) {
            if ($file_version le v2.0) {
               if (length($value)) {
                  report_warning($self, "ignoring data in a negated property '!$prop_name'");
               }
               undef $value;
               $boolean_value = 0;
            } else {
               report_error($self, "obsolete syntax for an undefined property: '!$prop_name'");
               undef $value;
            }
         } elsif (length($value)) {
            if ($value =~ $PropertyValue::UNDEF_re) {
               undef $value;
            } else {
               enforce_nl($value);
            }
            $boolean_value = $value;
         } elsif ($file_version le v2.0) {
            # In v1.3 booleans were encoded as attributes (true) and (false),
            # while in v1.4 .. v2.0 as empty or negated sections.
            $boolean_value = $attr eq "(false)" ? 0 : 1;
         }

         if (defined (my $cast_sub=UNIVERSAL::can($object, "cast_if_seen_$prop_name"))) {
            if (defined (my $ret = $cast_sub->($object))) {
               if (is_array($ret)) {
                  # created a new parent object, the current object must become its property
                  ($subobject, my $sub_prop) = @$ret;
                  # this is necessary since we can't simply exchange the object references
                  # all the stack down
                  swap_deref($object, $subobject);
                  $object->name = $subobject->name;
                  undef $subobject->name;
                  $object->description = $subobject->description;
                  undef $subobject->description;
                  $object->take($sub_prop, $subobject);
                  $proto = $object->type;
               } else {
                  $proto = $app->eval_type($ret);
                  if ($@) {
                     report_parse_error($self, "error reading property '$prop_name'");
                     next;
                  }
                  bless $object, $proto->pkg;
               }
            }
         }
         if (defined (my $upgrade_sub = UNIVERSAL::can($object, "upgrade_plain_$prop_name"))) {
            eval { $upgrade_sub->($object, $value, $boolean_value) };
            if ($@) {
               report_parse_error($self, "error reading property '$prop_name'");
            }
         } elsif (defined (my $prop = $proto->lookup_property($prop_name))) {
            eval { $object->_add($prop, $prop->type->name eq "Bool" ? $boolean_value : $value) };
            if ($@) {
               report_parse_error($self, "error reading property '$prop_name'");
            }
         } elsif (defined($subobject) && defined ($prop = $subobject->type->lookup_property($prop_name))) {
            eval { $subobject->_add($prop, $prop->type->name eq "Bool" ? $boolean_value : $value) };
            if ($@) {
               report_parse_error($self, "error reading property '$prop_name'");
            }
         } elsif ($Shell->interactive) {
            consume_unknown_property($prop_name, $value, $object);
         } else {
            report_error($self, "unknown property '$prop_name'");
            $self->unknown_seen = true;
         }

      } else {
         $self->line = $.;  chomp;
         report_error($self, "ill-formed section header '$_'");
         while (<$fh>) {        # skip the rest of the malformed sektion
            last unless /\S/;
         }
      }
   }

   if (@{$self->errors}) {
      if ($self->unknown_seen) {
         my $name = $self->filename;
         push @{$self->errors}, <<".";

***************************************************************************
You can convert the unknown sections to attachments and/or valid properties
if you load the file $name in the interactive shell or by executing
polymake --touch $name
***************************************************************************
.
      }
      die @{$self->errors};
   }

   if (!defined $object->name) {
      my ($object_name) = $self->filename =~ $filename_re;
      if (defined (my $suffix = $object->default_file_suffix)) {
         $object_name =~ s/\.$suffix(?:\..+)?$//;
      }
      $object->name = $object_name;
   }
   $object->persistent = $persistent;
   $object->transaction->changed = true;
   $object->commit;
   $object
}
#################################################################################
sub consume_unknown_property {
   my ($prop_name, $value, $object)=@_;
   (my $show_value=$value) =~ s/\A((?:^.*\n){3})^(?s:.+)\Z/$1...\n/m;

   local ref $Shell->try_completion = sub : method {
      my ($self) = @_;
      my $text = substr($self->term->Attribs->{line_buffer}, 0, $self->term->Attribs->{point});
      if ($text =~ m{^\s* (?: $qual_id_re \s*[<>,]\s* )* (?'prefix' $qual_id_re (?: ::)?)? $}xo) {
         Shell::Completion::try_type_completion($self, $+{prefix});
      }
      $self->completion_proposals //= [ ];
      if ($text =~ m{^\s* (?: (?'type' $type_qual_re) ::)? (?'prefix' $hier_id_re\.?)? $}xo) {
         my ($object_type, $prefix)=@+{qw(type prefix)};
         if (defined $object_type) {
            if (defined (my $proto=$User::application->eval_type($object_type, 1))) {
               push @{$self->completion_proposals},
                    Shell::Completion::try_property_completion($proto, $prefix);
            }
         } else {
            push @{$self->completion_proposals},
                 Shell::Completion::try_property_completion($object->type, $prefix);
         }
         $self->completion_offset = length($prefix);
      }
   };
   local $Shell->term->{MinLength} = 1;
   my $obj_type=$object->type->full_name;
   print <<".";
Encountered an unknown section '$prop_name':
$show_value

If you want to treat it as a property with a different name, please specify
the new property name, using the dot notation for a property in a subobject.
If the property belongs to a derived object type and you want to cast the
current object ($obj_type) to this derived type, please specify
the new type and property name as a qualified pair: BigObjectType::PROPERTY

If you want to keep the data as an attachment,
please specify a valid property type.

To discard the data completely, just leave the input field empty.
.

   for (;;) {
      my $choice = $Shell->read_input(" > ");
      $choice =~ s/^\s+//;
      $choice =~ s/\s+$//;
      length($choice) or last;
      if ($choice =~ m{^ (?: (?'type' $type_qual_re) ::)? (?'prop_name' $hier_id_re) $}xo) {
         my ($object_type, $prop_name) = @+{qw(type prop_name)};
         my $proto;
         if (defined($object_type)
               and
             defined ($proto = $User::application->eval_type($object_type))
               and
             instanceof BigObjectType($proto)) {
            # don't check auto-cast rules here, let's allow the user to do whatever she wants
            if ($proto->isa($object->type)) {
               bless $object, $proto->pkg;
            } else {
               print "Cast not allowed: ", $proto->full_name, " is not derived from ", $object->type->full_name,
                     "\nPlease choose another object type and/or property or discard the section giving empty input:\n";
               next;
            }
         }
         if ($prop_name =~ /\./ || $object->type->lookup_property($prop_name)) {
            eval { $object->take($prop_name, $value) };
            if ($@) {
               print <<".";
Conversion to a property $prop_name failed: $@
Please choose another property or discard the section giving empty input:
.
               next;
            } else {
               last;
            }
         }
      }

      my $x=$User::application->eval_type($choice);
      $x &&= eval { $x->construct->($value) };
      if ($@) {
         print <<".";
Conversion to an attachment of type $choice failed: $@
Please choose another type or discard the section giving empty input:
.
      } elsif (instanceof BigObject($x)) {
         print "An attachment can only have a property (atomic) data type, not a full object like ", $x->type->name, "\n";
      } else {
         $object->attach($prop_name, $x);
         last;
      }
   }
}
#################################################################################
sub report_warning {
   my $self=shift;
   warn_print( "\"", $self->filename, "\", line ", $self->line, ": ", @_ );
}
sub report_error {
   my ($self, $text)=@_;
   push @{$self->errors}, "\"".$self->filename."\", line ".$self->line.": $text\n";
}
sub value_parse_error {
   my $nl=(substr($_,0,pos)=~tr/\n/\n/)+1;
   my $piece=min(10, index($_,"\n",pos)-pos);
   die "$nl\tinvalid ", $_[0]->full_name, " value ",
       $piece>0 ? ("starting at '", substr($_,pos,$piece), " ...'\n") : ("at the end of line\n");
}
sub report_parse_error {
   my ($self, $my_text)=@_;
   my ($where, $text)= $@ =~ /^(?: (.*?) \t)? (.*)/x;
   $where ||= 0;
   $where+=$self->line;
   push @{$self->errors}, "\"".$self->filename."\", line $where: $my_text: $text\n";
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
