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

use XML::LibXML;
use XML::SAX::Base;
use XML::Writer;

require Cwd;

use strict;
use namespaces;

my $pmns="http://www.math.tu-berlin.de/polymake/#3";
my $xml_id_attr="{$XML::NamespaceSupport::NS_XML}id";

my ($rngschema, $DOMparser, $XSLT, $XPath, $suppress_auto_save);

#############################################################################################
#
#  XML Parser
#

package Polymake::Core::XMLhandler;

use Polymake::Ext;

use Polymake::Struct (
   [ new => '$$;$' ],
   [ '$filename' => '#1' ],
   [ '$source' => '#3' ],
   [ '$nsprefix' => 'undef' ],
   '$trusted',
   [ '@cur_object' => '( #2 )' ],
   '@cur_proto',
   '@cur_property',
   '@cur_value',
   [ '$cols' => 'undef' ],
   '$text',
   '@instance_by_id',
   [ '$last_id' => 'undef' ],
   [ '$transforms' => 'undef' ],
   [ '$doc_tree' => 'undef' ],
   [ '$chk' => 'undef' ],
   [ '$SAX' => 'undef' ],
   [ '$skipping' => 'undef' ],
   '@ext',
);

declare $force_verification;

#############################################################################################
sub gather_transforms {
   my $ext_dir=shift;
   [ sort { $a->[0] cmp $b->[0] }
     map { m|/upgrade-(.*)\.xslt$|; [ eval("v$1"), $_, undef, defined($ext_dir) ] }
     glob(($ext_dir || $InstallTop) . "/xml/upgrade-*.xslt") ];
}

my ($core_transforms, %ext_transforms);

# Take care of aggregated released upgrade scripts repeating the single steps from development code line:
# the last pre-release upgrade script is a symlink pointing to the released script.
sub filter_release_transforms {
   my $self=shift;
   my $i=0;
   while ($i<$#{$self->transforms}) {
      my $upgrade_file=$self->transforms->[$i]->[1];
      if (-l $upgrade_file && readlink($upgrade_file) eq ($self->transforms->[$i+1]->[1] =~ $filename_re)[0]) {
         splice @{$self->transforms}, $i, 2;
      } else {
         ++$i;
      }
   }
   return $i>0;  # true if still transforms to do
}

sub apply_transforms {
   my ($self)=@_;
   foreach my $tr (@{$self->transforms}) {
      $tr->[2] ||= do {
         $tr->[3] && set_search_path([ "$InstallTop/xml" ]);
         $XSLT->parse_stylesheet_file($tr->[1]);
      };
      dbg_print( "transforming ", $self->filename || "<input string>", " with ",
                 $tr->[3] ? $tr->[1] : ($tr->[1] =~ $filename_re)[0] ) if $Verbose::files;
      $self->doc_tree=$tr->[2]->transform($self->doc_tree);
   }
   @{$self->transforms}=();
}

sub parse_file {
   my ($self, $fh)=@_;
   my $SAXhandler=new XML::SAX::Base( ContentHandler => $self );
   weak($self->SAX=$SAXhandler);
   my $parser=new XML::LibXML( Handler => $SAXhandler );
   eval { $parser->parse_fh($fh) };
   if ($@) {
      if (index($@,"\eTRANSFORM")==0) {
         seek($fh,0,0);
         $self->doc_tree=$DOMparser->parse_fh($fh);
         # check for further tranformations in extensions
         provide_ext($self, $_->getAttribute("ext"), 0)
           for xpath_ctx()->findnodes('p:property[@ext] | p:object[@ext] | p:attachment[@ext]', $self->doc_tree->documentElement);
         apply_transforms($self);
         $parser=XML::LibXML::SAX::Parser->new( ContentHandler => $self );
         weak($self->SAX=$parser);
         $parser->generate($self->doc_tree);
         undef $self->doc_tree;
         $@="";
      } else {
         $@ =~ s{ at $InstallTop/\S+/XMLfile\.pm line \d+\.?$}{}o;
         die $@;
      }
   }
}

sub parse_string {
   my $self=shift;
   require XML::LibXML::SAX::Parser;
   $DOMparser ||= new XML::LibXML;
   $self->doc_tree=$DOMparser->parse_string(@_);
   my $parser=XML::LibXML::SAX::Parser->new( ContentHandler => $self );
   weak($self->SAX=$parser);
   eval { $parser->generate($self->doc_tree) };
   if ($@) {
      if (index($@,"\eTRANSFORM")==0) {
         apply_transforms($self);
         $parser->generate($self->doc_tree);
         $@="";
      } else {
         $@ =~ s{ at $InstallTop/\S+/XMLfile\.pm line \d+$}{}o;
         die $@;
      }
   }
   undef $self->doc_tree;
}
#############################################################################################
sub initiate_transform {
   require XML::LibXSLT;
   require XML::LibXML::SAX::Parser;
   $DOMparser ||= new XML::LibXML;
   $XSLT ||= XML::LibXSLT->new();
   die "\eTRANSFORM";
}
#############################################################################################
sub xpath_ctx {
   $XPath || do {
      $XPath=new XML::LibXML::XPathContext;
      $XPath->registerNs('p', $pmns);
      $XPath
   }
}
#############################################################################################
# actions= 1: obligatory (top-level object)  2: throw if transforms needed
sub provide_ext {
   my ($self, $ext_attr, $actions)=@_;
   $ext_attr =~ /^\s*/g;
   while ($ext_attr =~ /\G (\d+)? (?(1) (?: = ([^\s\#]+) (?: \#([\d.]+))? )? | (\S+)) (?:\s+|$)/xgc) {
      my ($i, $URI, $version)=($1, downgradeUTF8($2 || $4), $3);
      if (defined $URI) {
         unless (defined $self->ext->[$i]) {   # extra check for the case of repeated visit after a transformation
            if (defined (my $ext=$self->ext->[$i]=provide Extension($URI, $actions & 1))) {
               if ($ext) {
                  my $version_num=defined($version) ? eval("v$version") : "";
                  if ($version_num ne $ext->version_num) {
                     if ($version_num lt $ext->version_num) {
                        push @{$self->transforms},
                             grep { $version_num lt $_->[0] } @{ $ext_transforms{$ext} ||= gather_transforms($ext->dir) };
                     } else {
                        warn_print( defined($self->filename) ? ("file ", $self->filename) : ("<input string>"),
                                    " has been created with a newer version $version of extension ", $ext->URI,
                                    " while yours is ", $ext->version || "UNDEFINED", ";\n",
                                    "Proceeding at your own risk!\n" );
                        undef $self->chk;
                     }
                  }
               }
            } else {
               # skip the data
               return 0;
            }
         }
      } elsif (!defined($self->ext->[$i])) {
         # skip the data
         return 0;
      }
   }
   if ($ext_attr =~ /\G\s*\S/) {
      die "attribute `ext' has an invalid value starting with '", substr($ext_attr, $+[0]-1, 30), "'\n"
   }
   if ($actions & 2 && @{$self->transforms}) {
      if (@{$self->transforms}==1 || filter_release_transforms($self)) {
         splice @{$self->cur_object}, 1;
         $self->cur_object->[0]->reset_to_empty;
         @{$self->cur_proto}=();
         @{$self->cur_property}=();
         @{$self->cur_value}=();
         @{$self->instance_by_id}=();
         undef $self->nsprefix;
         undef $self->cols;
         undef $self->last_id;
         undef $self->chk;
         initiate_transform;
      } elsif (!$self->cur_object->[0]->changed) {
         $self->cur_object->[0]->changed=1;
         $self->cur_object->[0]->ensure_save_changes;
      }
   }
   1
}
#############################################################################################
sub processing_instruction : method {
   my ($self, $pi)=@_;
   if ($pi->{Target} eq "pm") {
      if (!$force_verification && !defined($self->transforms) && $pi->{Data} =~ /\bchk="([[:xdigit:]]+)"/) {
         $self->chk=hex($1);
      }
   }
}
#############################################################################################
sub verify_integrity : method {
   my $self=shift;
   if (defined $self->filename) {
      my ($fsize, $mtime)=(stat $self->filename)[7,9];
      $self->trusted= $self->chk==($mtime^$fsize) || $self->chk==($mtime-1^$fsize);
   } else {
      $self->trusted= $self->chk==length(${$self->source});
   }
}
#############################################################################################
sub start_element : method {
   my ($self, $elem)=@_;
   my $tag="start_".$elem->{LocalName};
   if (!defined($self->nsprefix)) {
      if ($elem->{NamespaceURI} ne $pmns) {
         die "file ", $self->filename, " is not encoded in the current polymake XML namespace\n";
      }

      my $attrs=$elem->{Attributes};
      unless (defined $self->transforms) {
         $self->transforms=[ ];
         if (exists $attrs->{"{}version"}) {
            # check this as the very first thing:
            # if we need to transform the input file, any state changes should be avoided
            my $version_num=eval("v".$attrs->{"{}version"}->{Value});
            if ($version_num ne $VersionNumber) {
               if ($version_num lt $VersionNumber) {
                  @{$self->transforms}=grep { $version_num lt $_->[0] } @{ $core_transforms ||= gather_transforms() };
               } else {
                  warn_print( defined($self->filename) ? ("file ", $self->filename) : ("<input string>"),
                              " has been created with a newer polymake version ",
                              $attrs->{"{}version"}->{Value}, " while yours is $Version;\n",
                              "Proceeding at your own risk!\n" );
                  undef $self->chk;
               }
            }
         }

         if (exists $attrs->{"{}ext"}) {
            provide_ext($self, $attrs->{"{}ext"}->{Value}, 1);
         }

         if (@{$self->transforms}) {
            if (@{$self->transforms}==1 || filter_release_transforms($self)) {
               undef $self->chk;
               initiate_transform;
            } else {
               $self->cur_object->[0]->changed=1;
               $self->cur_object->[0]->ensure_save_changes;
            }
         }
      }

      $self->nsprefix=$elem->{Prefix};
      my $object=$self->cur_object->[0];
      my $is_object= $tag eq "start_object";
      my $proto=eval_qualified_type($self, $attrs->{"{}type"}->{Value}, $is_object)
                  // die "invalid top-level element type: $@\n";

      if (instanceof Object($object)) {
         $is_object or die "top-level element must be `object'\n";
         bless $object, $proto->pkg;
         if (exists $attrs->{"{}name"}) {
            $object->set_name_trusted($attrs->{"{}name"}->{Value});
         }
         push @{$self->cur_proto}, $proto;
      } else {
         !$is_object or die "top-level element must be `data'\n";
         push @{$self->cur_value},
              [ exists $attrs->{"{}value"} ? downgradeUTF8($attrs->{"{}value"}->{Value}) : () ];
         push @{$self->cur_property}, [ $object->mock_property($proto) ];
      }

      if (defined $self->chk) {
         $self->verify_integrity;
      }
      if (!$self->trusted) {
         $rngschema ||= new XML::LibXML::RelaxNG(location => $InstallTop."/xml/datafile.rng");
         if (defined($self->doc_tree)) {
            $rngschema->validate($self->doc_tree);
         } else {
            dbg_print( "validating XML file ", $self->filename ) if $Verbose::files;
            $DOMparser ||= new XML::LibXML;
            $rngschema->validate($DOMparser->parse_file($self->filename));
            dbg_print( "XML validation succeeded" ) if $Verbose::files;
         }
         $PropertyType::trusted_value=0;
         $object->begin_init_transaction;
      }
   } elsif ($self->trusted || $self->nsprefix eq $elem->{Prefix}) {
      $self->$tag($elem);
   } else {
      die "element from an unknown XML namespace";
   }
}

sub end_element : method {
   my ($self, $elem)=@_;
   my $etag="end_".$elem->{LocalName};
   $self->$etag();
}

sub eval_qualified_type {
   my ($self, $type, $is_object, $in_tree)=@_;
   if (my ($app_name, $type_expr)=map { downgradeUTF8($_) } $type =~ /^(?:($id_re)::)? ($type_re)$/xo) {
      my $app= defined($app_name)
               ? add Application($app_name) :
               $in_tree
               ? $self->cur_proto->[-1]->application
               : die("application name missing in the top-level element type");
      while ($type_expr =~ /(?<!::)\b(\w+)::/g) {
         add Application($1);
      }
      $app->eval_type(($is_object ? "objects::" : "props::").$type_expr);
   } else {
      die "malformed type attribute '$type'";
   }
}

sub extract_type {
   my ($self, $attrs, $is_object)=@_;
   if (defined (my $type=$attrs->{"{}type"}->{Value})) {
      if ($type eq "text") {
         $type
      } else {
         eval_qualified_type($self, $type, $is_object, 1)
           // die "invalid ", ($is_object ? "object" : "property"), " type '$type': $@\n";
      }
   } else {
      undef;
   }
}

sub start_object : method {
   my ($self, $elem)=@_;
   my $prop=$self->cur_property->[-1]->[0];
   my $expected=$prop->subobject_type;
   my $attrs=$elem->{Attributes};
   if (exists $attrs->{"{}ext"} and !provide_ext($self, $attrs->{"{}ext"}->{Value}, 2)) {
      skip_subtree_mode($self, $prop->flags & $Property::is_multiple ? 1 : 2);
      return;
   }
   my $type=extract_type($self, $attrs, 1);
   if (defined($type)) {
      $type=$expected->local_type($type) if $prop->flags & $Property::is_locally_derived;
   } else {
      $type=$expected;
   }
   my $sub_obj=Object::__new($type->pkg, exists $attrs->{"{}name"} ? $attrs->{"{}name"}->{Value} : undef);
   $sub_obj->parent=$self->cur_object->[-1]->value;    # during the subobject construction
   push @{$self->cur_object}, $sub_obj;
   push @{$self->cur_proto}, $type;
}

sub end_object : method {
   my ($self)=@_;
   pop @{$self->cur_proto};
   my $obj=pop @{$self->cur_object};
   if (@{$self->cur_value}) {
      push @{$self->cur_value->[-1]}, $obj;
      undef $obj->parent;
   }
}

my $warn_about_unknown_props;

sub start_property : method {
   my ($self, $elem)=@_;
   my $attrs=$elem->{Attributes};
   if (exists $attrs->{"{}ext"} and !provide_ext($self, $attrs->{"{}ext"}->{Value}, 2)) {
      skip_subtree_mode($self, 1);
      return;
   }
   my $prop_name=downgradeUTF8($attrs->{"{}name"}->{Value});
   if ($prop_name =~ s/^($id_re):://o) {
      add Application($1);
   }
   my $prop=$self->cur_proto->[-1]->lookup_property($prop_name);
   unless (defined $prop) {
      warn_print( "encountered unknown property '$prop_name' in object ", $self->cur_proto->[-1]->full_name );
      $warn_about_unknown_props ||= print <<'.';

Please be aware that all unknown properties are automatically removed
from the data files.
If you want to supply additional information with the objects, please use
attachments or declare new properties in extension rules.

.
      skip_subtree_mode($self, 1);
      return;
   }
   my $type=extract_type($self, $attrs, 0);
   push @{$self->cur_property}, [ $prop->concrete($self->cur_object->[-1]), $type ];
   push @{$self->cur_value},
        [ exists $attrs->{"{}value"} ? downgradeUTF8($attrs->{"{}value"}->{Value}) :
          exists $attrs->{"{}undef"} ? undef : () ];
   $self->text="";
}

sub end_property : method {
   my ($self)=@_;
   my ($prop, $type)=@{ pop @{$self->cur_property} };
   my $value_ptr=pop @{$self->cur_value};
   if (defined $type) {
      if (ref($type)) {
         $self->cur_object->[-1]->_add($prop, $type->construct->($value_ptr->[0]), $self->trusted);
      } else {
         # $type eq "text"
         # this value may contain any funny Unicode characters, don't downgrade!
         $self->cur_object->[-1]->_add($prop, $self->text, $self->trusted);
      }
   } elsif ($prop->flags & $Property::is_multiple) {
      $self->cur_object->[-1]->_add_multis($prop, $value_ptr, $self->trusted);
   } else {
      $self->cur_object->[-1]->_add($prop, $value_ptr->[0], $self->trusted);
   }
}

sub start_attachment {
   my ($self, $elem)=@_;
   my $attrs=$elem->{Attributes};
   if (exists $attrs->{"{}ext"} and !provide_ext($self, $attrs->{"{}ext"}->{Value}, 2)) {
      skip_subtree_mode($self, 1);
      return;
   }
   my $name=downgradeUTF8($attrs->{"{}name"}->{Value});
   my $type=extract_type($self, $attrs, 0);
   my $construct= exists $attrs->{"{}construct"} && downgradeUTF8($attrs->{"{}construct"}->{Value});
   push @{$self->cur_property}, [ $name, $type, $construct ];
   push @{$self->cur_value},
        [ exists $attrs->{"{}value"} ? downgradeUTF8($attrs->{"{}value"}->{Value}) : () ];
   $self->text="";
}

sub end_attachment : method {
   my ($self)=@_;
   my ($name, $type, $construct)=@{ pop @{$self->cur_property} };
   my ($value)=@{ pop @{$self->cur_value} };
   if (defined($type)) {
      if (ref($type)) {
         if ($construct) {
            $value=$type->construct->($self->cur_object->[-1]->give($construct), $value);
         } else {
            $value=$type->construct->($value);
         }
      } elsif ($type eq "text") {
         $value=$self->text;
      }
   }
   $self->cur_object->[-1]->attachments->{$name}=[ $value, $construct ];
}

sub end_data : method {
   my ($self)=@_;
   my $type=(pop @{$self->cur_property})->[0]->type;
   my ($value)=@{ pop @{$self->cur_value} };
   $self->cur_value=$type->construct->($value);
}

sub start_description : method {
   my ($self)=@_;
   $self->text="";
}

sub end_description : method {
   my ($self)=@_;
   $self->cur_object->[-1]->description=$self->text;
}

sub start_credit : method {
   my ($self, $elem)=@_;
   push @{$self->cur_property}, $elem->{Attributes}->{"{}product"}->{Value};
   $self->text="";
}

sub end_credit : method {
   my ($self)=@_;
   my $product=pop @{$self->cur_property};
   $self->cur_object->[-1]->credits->{$product}= $self->cur_proto->[-1]->application->credits->{$product} || $self->text;
}

sub start_v : method {
   my ($self, $elem)=@_;
   my $attrs=$elem->{Attributes};
   my $value=[ ];
   if (exists $attrs->{"{}dim"}) {
      set_array_flags($value, $attrs->{"{}dim"}->{Value});
   } elsif (defined($self->cols)) {
      set_array_flags($value, $self->cols);
   }
   if (exists $attrs->{"{}i"}) {
      push @{$self->cur_value->[-1]}, $attrs->{"{}i"}->{Value}+0;
   }
   push @{$self->cur_value}, $value;
   $self->text="";
}

sub end_v : method {
   my ($self)=@_;
   my $list=pop @{$self->cur_value};
   if (!@$list && $self->text =~ /\S/) {
      if ($self->text =~ /['"]/) {
         $self->text=downgradeUTF8($self->text);
         while ($self->text =~ /\G\s* (?: (['"]) (.*?) \1 | (\S+))/xg) {
            push @$list, defined($1) ? $2 : $3;
         }
      } else {
         $self->text =~ s/^\s+//;
         @$list=split /\s+/, downgradeUTF8($self->text);
      }
   }
   push @{$self->cur_value->[-1]}, $list;
   $self->text="";
}

sub start_m : method {
   my ($self, $elem)=@_;
   my $attrs=$elem->{Attributes};
   my $value=[ ];
   if (exists $attrs->{"{}cols"}) {
      $self->cols=$attrs->{"{}cols"}->{Value}+0;
   } elsif (exists $attrs->{"{}dim"}) {
      set_array_flags($value, $self->cols=$attrs->{"{}dim"}->{Value});
   }
   push @{$self->cur_value}, $value;
}

sub end_m : method {
   my ($self)=@_;
   my $list=pop @{$self->cur_value};
   push @{$self->cur_value->[-1]}, $list;
   undef $self->cols;
}

sub start_e : method {
   my ($self, $elem)=@_;
   my $attrs=$elem->{Attributes};
   if (exists $attrs->{"{}i"}) {
      push @{$self->cur_value->[-1]}, $attrs->{"{}i"}->{Value}+0;
   }
   $self->text="";
}

sub end_e : method {
   my ($self)=@_;
   push @{$self->cur_value->[-1]}, downgradeUTF8($self->text);
}

sub start_t : method {
   my ($self, $elem)=@_;
   my $value=[ ];
   my $attrs=$elem->{Attributes};
   if (exists $attrs->{"{}id"}) {
      my $id=$attrs->{"{}id"}->{Value}-1;
      set_array_flags($value, -1, $self->instance_by_id->[$id]=undef);
      $self->last_id=$id;
   } else {
      set_array_flags($value, -1);
   }
   if (exists $attrs->{"{}i"}) {
      push @{$self->cur_value->[-1]}, $attrs->{"{}i"}->{Value}+0;
   }
   $self->text="";
   push @{$self->cur_value}, $value;
}

*end_t=\&end_v;

sub start_r : method {
   my ($self, $elem)=@_;
   my $attrs=$elem->{Attributes};
   if (exists $attrs->{"{}id"}) {
      $self->last_id=$attrs->{"{}id"}->{Value}-1;
   } elsif (!defined($self->last_id)) {
      die "default instance reference without preceding named instances\n";
   }
   push_scalar($self->cur_value->[-1], $self->instance_by_id->[$self->last_id]);
}

sub end_r {}

sub characters : method {
   my ($self, $elem)=@_;
   $self->text .= $elem->{Data};
}

sub skip_subtree_mode {
   my ($self, $depth)=@_;
   ($self->skipping=new Scope())->begin_locals;
   local $self->SAX->{Methods}->{start_element}=sub { ++$depth };
   local $self->SAX->{Methods}->{characters}=sub { };
   local $self->SAX->{Methods}->{end_element}=sub { --$depth or undef $self->skipping };
   $self->skipping->end_locals;
   if (defined($self->filename)) {
      unless (-f $self->filename.".bak") {
         warn_print( "Creating backup file ", $self->filename.".bak" );
         require File::Copy;
         File::Copy::copy($self->filename, $self->filename.".bak");
      }
      ($self->cur_object->[0]->transaction || $self->cur_object->[0])->changed=1;
   }
}

#############################################################################################
package Polymake::Core::XMLfile;

use Polymake::Struct (
   [ new => '$' ],
   [ '$filename' => 'Cwd::abs_path( #1 )' ],
   '$is_compressed',
);

sub load {
   my ($self, $object)=@_;
   my $mode="<".layer_for_compression($self);
   open my $fh, $mode, $self->filename or die "can't open file ".$self->filename.": $!\n";
   unless ($self->is_compressed) {
      local $_;
      while (<$fh>) {
         if (/^\s*<\?xml/) {
            seek($fh,0,0);
            last;
         }
         next if /^\s*$/;

         require Polymake::Core::PlainFile;
         my $plf=new PlainFile($self->filename);
         seek($fh,0,0);
         $object->begin_init_transaction;
         $plf->load($object, $fh);
         $object->transaction->changed=1;
         $object->transaction->commit($object);
         return;
      }
   }
   my $handler=new XMLhandler($self->filename, $object);
   local $PropertyType::trusted_value=1;
   $handler->trusted=$self->is_compressed;
   $handler->parse_file($fh);
   unless ($PropertyType::trusted_value) {
      $object->transaction->changed=1;
      $object->transaction->commit($object);
      $object->dont_save if $suppress_auto_save;
   }
}
#############################################################################################
sub load_data {
   my ($self)=@_;
   my $mode="<".layer_for_compression($self);
   open my $fh, $mode, $self->filename or die "can't open file ".$self->filename.": $!\n";
   my $handler=new XMLhandler($self->filename, new LooseData());
   local $PropertyType::trusted_value=1;
   $handler->trusted=$self->is_compressed;
   $handler->parse_file($fh);
   if (!$handler->trusted && !$suppress_auto_save && -w $self->filename) {
      save_data($self, $handler->cur_value, $handler->cur_object->[0]->description);
   }
   wantarray ? ($handler->cur_value, $handler->cur_object->[0]->description) : $handler->cur_value;
}
#############################################################################################
sub layer_for_compression {
   my ($self)=@_;
   if ($self->filename =~ /\.gz$/) {
      require PerlIO::via::gzip;
      $self->is_compressed=1;
      ":via(gzip)";
   } else {
      ""
   }
}
#############################################################################################
# to be used in the test suite driver and other exotic circumstances
sub suppress_validation {
   my $scope=shift;
   $scope->begin_locals;
   local_sub(\&XMLhandler::verify_integrity, sub : method { $_[0]->trusted=1 });
   $scope->end_locals;
}

sub enforce_validation {
   my $scope=shift;
   $scope->begin_locals;
   local_sub(\&XMLhandler::verify_integrity, sub : method { $_[0]->trusted=0 });
   local_incr($suppress_auto_save);
   $scope->end_locals;
}
#############################################################################################
# mocking a top-level object in a loose data file
package Polymake::Core::XMLfile::LooseData;
use Polymake::Struct (
   '$description',
);

my $dummy_changed;
sub begin_init_transaction {}
sub changed : lvalue { $dummy_changed }
sub ensure_save_changes {}
sub reset_to_empty { (shift)->description="" }
sub value { undef }
sub mock_property { new Property(pop) }

package _::Property;
use Polymake::Struct (
   [ new => '$' ],
   [ '$type' => '#1' ],
);

sub flags { $Property::is_multiple | $Property::is_subobject_array }
sub subobject_type { $_[0]->type->params->[0] }

#############################################################################################
package Polymake::Core::XMLwriter::PITemplate;
use Polymake::Struct (
   [ new => '$$' ],
   [ '$name' => '#1' ],
   [ '$body' => '#2' ],
   '@field',
   '@offset',
   '@length',
);

sub new {
   my $self=&_new;
   while ($self->body =~ s/%\{(\d+)\}/' ' x $1/e) {
      push @{$self->field}, "%0$1x";
      push @{$self->offset}, $-[0];
      push @{$self->length}, $1;
   }
   $self
}

package Polymake::Core::XMLwriter::PIbody;
use Polymake::Struct (
   [ new => '$$' ],
   [ '$template' => '#1' ],
   [ '$handle' => 'undef' ],
   [ '$string' => 'undef' ],
   '$pos',
);

sub new {
   my $self=&_new;
   my $target=pop;
   if (ref($target) eq "GLOB") {
      $self->handle=$target;
      $self->pos=tell($target);
   } else {
      $self->string=$target;
      $self->pos=length($$target);
   }
   $self->pos+=length($self->template->name)+3;   # account for leading <? and a whitespace
   $self
}

use overload '""' => sub { (shift)->template->body };

sub print {
   my ($self, $f)=splice @_, 0, 2;
   if (defined($self->handle)) {
      seek($self->handle, $self->pos+$self->template->offset->[$f], 0);
      $self->handle->printf($self->template->field->[$f], @_);
   } else {
      substr(${$self->string}, $self->pos+$self->template->offset->[$f], $self->template->length->[$f])=
         sprintf($self->template->field->[$f], @_);
   }
}
#############################################################################################
#
#  XML Writer
#

package Polymake::Core::XMLwriter;

my @writer_init=( PREFIX_MAP => { $pmns=>"" }, NAMESPACES => 1,
                  DATA_MODE => 1, DATA_INDENT => 2, ENCODING => 'utf-8', UNSAFE => !$DeveloperMode );

my $data_pi=new PITemplate('pm', 'chk="%{8}"');

sub new {
   new XML::Writer(OUTPUT => $_[1], @writer_init, @_[2..$#_]);
}

sub ext_attr {
   my ($writer, $type)=@_;
   if (defined (my $ext_list=$type->extension)) {
      if (my @ext=map {
                     if ($_->is_bundled || exists $writer->{':suppress_ext'}->{$_}) {
                        ()
                     } else {
                        if (@_==3) {
                           ($_[2] ||= new Scope())->begin_locals;
                           local $writer->{':suppress_ext'}->{$_}=1;
                           $_[2]->end_locals;
                        }
                        my $def;
                        ($writer->{':ext'}->{$_} //= do { $def="=".$_->versioned_URI; keys(%{$writer->{':ext'}}) }) . $def;
                     }
                  }
          is_object($ext_list) ? $ext_list : @{$ext_list}) {
         return (ext => join(" ", @ext));
      }
   }
   ()
}

sub top_ext_attr {
   my ($writer, $type)=@_;
   if (my $ext_list= $type->extension ||
                     ($type->application->installTop ne $InstallTop && $Extension::registered_by_dir{$type->application->installTop})) {
      (ext => join(" ", map {
                           $writer->{':suppress_ext'}->{$_}=1;
                           ($writer->{':ext'}->{$_} = keys(%{$writer->{':ext'}})) . "=". $_->versioned_URI
                        } is_object($ext_list) ? $ext_list : @{$ext_list} ))
   } else {
      ()
   }
}

sub type_attr {
   my ($type, $owner)=@_;
   ( type => $type->qualified_name(defined($owner) ? $owner->type->application : undef) )
}

sub write_subobject {
   my ($writer, $object, $parent, $expected_type)=@_;
   my $type=$object->type;
   $writer->startTag( "object",
                      length($object->name) ? (name => $object->name) : (),
                      $type != $expected_type ? (type_attr($type->pure_type, $parent),
                                                 $type->extension ? ext_attr($writer, $type->extension, $_[4]) : ())
                                              : ()
                    );
   write_object_contents($writer,$object);
   $writer->endTag("object");
}

sub write_object_contents {
   my ($writer, $object)=@_;
   if (length($object->description)) {
      $writer->cdataElement("description", $object->description);
   }
   while (my ($product, $credit)=each %{$object->credits}) {
      $writer->cdataElement("credit", is_object($credit) ? $credit->toFileString : $credit, "product" => $product);
   }
   foreach my $pv (@{$object->contents}) {
      next if !defined($pv) || $pv->property->flags & $Property::is_non_storable;
      my $scope;
      my @ext=ext_attr($writer, $pv->property, $scope);
      if (instanceof Object($pv)) {
         $writer->startTag( "property", name => $pv->property->qual_name, @ext );
         write_subobject($writer, $pv, $object, $pv->property->type, $scope);
         $writer->endTag("property");
      } elsif ($pv->flags & $PropertyValue::is_ref) {
         # TODO: handle explicit references to other objects some day...
      } elsif ($pv->property->flags & $Property::is_multiple) {
         $writer->startTag( "property", name => $pv->property->qual_name, @ext );
         write_subobject($writer, $_, $object, $pv->property->type, $scope) for @{$pv->values};
         $writer->endTag("property");
      } elsif (defined($pv->value)) {
         my $type=$pv->property->type;
         my @show_type;
         if (is_object($pv->value)) {
            if (ref($pv->value) ne $type->pkg) {
               $type=$pv->value->type;
               @show_type=type_attr($type, $object);
               if (my @type_ext=ext_attr($writer, $type)) {
                  if (@ext) {
                     $ext[1].=" $type_ext[1]";
                  } else {
                     @ext=@type_ext;
                  }
               }
            }
         }
         if ($type->toXML) {
            $writer->startTag( "property", name => $pv->property->qual_name, @show_type, @ext );
            $type->toXML->($pv->value, $writer);
            $writer->endTag("property");
         } elsif ($type->name eq "Text") {
            $writer->cdataElement( "property", $pv->value, name => $pv->property->qual_name, type => "text", @ext );
         } else {
            $writer->emptyTag( "property", name => $pv->property->qual_name, @show_type, @ext,
                               value => $type->toString->($pv->value) );
         }
      } else {
         $writer->emptyTag( "property", name => $pv->property->qual_name, @ext, undef => "true" );
      }
   }
   while (my ($name, $at)=each %{$object->attachments}) {
      my ($value, $construct)=@$at;
      if (ref($value)) {
         my $type=$value->type;
         my @construct= $construct ? (construct => $construct) : ();
         if ($type->toXML) {
            $writer->startTag( "attachment", name => $name, type_attr($type, $object), ext_attr($writer,$type), @construct );
            $type->toXML->($value, $writer);
            $writer->endTag("attachment");
         } else {
            $writer->emptyTag( "attachment", name => $name, type_attr($type, $object), ext_attr($writer,$type), @construct, value => $value );
         }
      } elsif (length($value)>100 || $value =~ /\n/) {
         $writer->cdataElement("attachment", $value, name => $name, type => "text");
      } else {
         $writer->emptyTag( "attachment", name => $name, value => $value );
      }
   }
}

sub save {
   my ($output, $object, $is_file)=@_;
   my $writer=new XMLwriter($output);
   $writer->xmlDecl;

   my $pi;
   if ($is_file>=0) {
      $pi=new PIbody($data_pi, $output);
      $writer->pi($pi->template->name, $pi);
   }
   $writer->{':suppress_ext'}={};
   $writer->{':ext'}={};
   $writer->{':ids'}={};
   $writer->{':global_id'}=0;

   my $object_type=$object->type;

   $writer->startTag( [ $pmns, "object" ],
                      defined($object->name) ? (name => $object->name) : (),
                      type_attr($object_type),
                      top_ext_attr($writer, $object_type),
                      version => $Version,
                    );

   write_object_contents($writer, $object);
   $writer->endTag("object");
   $writer->end;

   if (defined $pi) {
      if ($is_file) {
         my $size=tell($output);
         $pi->print(0, $size ^ time);
      } else {
         $pi->print(0, length($$output));
      }
   }
}

sub save_data {
   my ($output, $data, $description, $is_file)=@_;
   my $writer=new XMLwriter($output);
   $writer->xmlDecl;

   my $pi;
   if ($is_file>=0) {
      $pi=new PIbody($data_pi, $output);
      $writer->pi($pi->template->name, $pi);
   }
   $writer->{':ids'}={};
   $writer->{':global_id'}=0;

   my $type=$data->type;
   $writer->startTag( [ $pmns, "data" ],
                      type_attr($type),
                      top_ext_attr($writer, $type),
                      version => $Version,
                      $type->toXML ? () : (value => $type->toString->($data)),
                    );
   if (length($description)) {
      $writer->cdataElement("description", $description);
   }
   if ($type->toXML) {
      $type->toXML->($data, $writer);
   }
   $writer->endTag("data");
   $writer->end;

   if (defined $pi) {
      if ($is_file) {
         my $size=tell($output);
         $pi->print(0, $size ^ time);
      } else {
         $pi->print(0, length($$output));
      }
   }
}

package Polymake::Core::XMLfile;

sub save {
   my ($self, $object)=@_;
   my $compress=layer_for_compression($self);
   my ($of, $of_k)=new OverwriteFile($self->filename, ":utf8".$compress);
   XMLwriter::save($of, $object, $compress ? -1 : 1);
   close $of;
}

sub save_data {
   my ($self, $data, $description)=@_;
   my $compress=layer_for_compression($self);
   my ($of, $of_k)=new OverwriteFile($self->filename, ":utf8".$compress);
   XMLwriter::save_data($of, $data, $description, $compress ? -1 : 1);
   close $of;
}

#############################################################################################
package Polymake::Core::XMLstring;
use Encode;

sub load {
   shift;
   my $object=shift;
   my $handler=new XMLhandler(undef, $object, \($_[0]));
   local $PropertyType::trusted_value=0;
   $object->begin_init_transaction;
   $handler->parse_string(@_);
   $object->transaction->changed=1;
   $object->transaction->commit($object);
}

sub save {
   shift;
   my $string="";
   XMLwriter::save(\$string, @_, 0);
   encode("utf8", $string);
}

sub save_data {
   shift;
   my $string="";
   XMLwriter::save_data(\$string, @_, 0);
   encode("utf8", $string);
}

1


# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
