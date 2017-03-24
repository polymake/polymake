#  Copyright (c) 1997-2017
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

require Cwd;

use strict;
use namespaces;
use feature 'state';

# XML namespaces
my $pmns="http://www.math.tu-berlin.de/polymake/#3";
my $RelaxNGns="http://relaxng.org/ns/structure/1.0";
my $W3CSchemaDatatypes="http://www.w3.org/2001/XMLSchema-datatypes";

my $suppress_auto_save=0;

#############################################################################################
#
#  XML Reader
#

package Polymake::Core::XMLreader;

use Polymake::Ext;

use Polymake::Struct (
   [ new => '$$' ],
   [ '$filename' => '#1' ],
   [ '$source' => 'undef' ],     # IO handle or a string reference
   '$trusted',
   [ '@cur_object' => '(#2)' ],
   '@cur_proto',
   '@cur_property',
   '@cur_value',
   '$text',
   '@cols',
   '@ext',
   '@transforms',
   [ '$doc_tree' => 'undef' ],
   [ '$chk' => 'undef' ],
);

declare $force_verification;
declare $reject_unknown_properties=0;

#############################################################################################
my ($core_transforms, %ext_transforms);

package _::Transform;

use Polymake::Struct (
   [ new => '$$$' ],
   [ '$version' => 'eval("v" . #1)' ],
   [ '$filename' => '#2' ],
   [ '$doc_tree' => 'undef' ],
   [ '$in_ext' => '#3' ],
);

sub init {
   my ($pkg, $ext_dir)=@_;
   [ sort { $a->version cmp $b->version }
     map { m|/upgrade-(.*)\.xslt$|; new($pkg, $1, $_, defined($ext_dir)) }
     glob(($ext_dir // $InstallTop) . "/xml/upgrade-*.xslt") ];
}

#############################################################################################
package Polymake::Core::XMLreader;

use XML::LibXML::Reader;

# Take care of aggregated released upgrade scripts repeating the single steps from development code line:
# the last pre-release upgrade script is a symlink pointing to the released script.
sub has_applicable_transforms {
   my ($self)=@_;
   my $i=0;
   while ($i < $#{$self->transforms}) {
      my $upgrade_file=$self->transforms->[$i]->filename;
      if (-l $upgrade_file && readlink($upgrade_file) eq ($self->transforms->[$i+1]->filename =~ $filename_re)[0]) {
         splice @{$self->transforms}, $i, 2;
      } else {
         ++$i;
      }
   }
   return $i>0 || @{$self->transforms};  # true if still transforms to do
}

sub apply_transforms {
   my ($self)=@_;
   require XML::LibXSLT;
   state $XSLT=new XML::LibXSLT();
   state $XPath=do {
      my $x=new XML::LibXML::XPathContext();
      $x->registerNs('p', $pmns);
      $x
   };

   # check for further transformations in extensions
   provide_ext($self, $_->getAttribute("ext"))
     for $XPath->findnodes('p:property[@ext] | p:object[@ext] | p:attachment[@ext]', $self->doc_tree->documentElement);

   foreach my $tr (@{$self->transforms}) {
      $tr->doc_tree //= do {
         $tr->in_ext && set_search_path([ "$InstallTop/xml" ]);
         $XSLT->parse_stylesheet_file($tr->filename);
      };
      dbg_print( "transforming ", $self->filename || "<input string>", " with ",
                 $tr->in_ext ? $tr->filename : ($tr->filename =~ $filename_re)[0] ) if $Verbose::files;
      $self->doc_tree=$tr->doc_tree->transform($self->doc_tree);
   }

   $#{$self->transforms}=-1;
}
#############################################################################################
sub reset_data {
   my ($self)=@_;
   splice @{$self->cur_object}, 1;
   $self->cur_object->[0]->reset_to_empty;
   @{$self->cur_proto}=();
   @{$self->cur_property}=();
   @{$self->cur_value}=();
}
#############################################################################################
sub create_doc_tree {
   my ($self, $parser)=@_;
   $self->doc_tree //= do {
      if (defined $parser) {
         # still at the root of the document - the most common use case (validation, main version upgrade)
         $parser->preserveNode;
         $parser->finish or die "malformed XML input\n";
         $parser->document
      } elsif (defined $self->filename) {
         seek($self->source, 0, 0);
         $parser=new XML::LibXML();
         $parser->parse_fh($self->source);
      } else {
         $parser=new XML::LibXML();
         $parser->parse_string(${$self->source});
      }
   }
}
#############################################################################################
my %open=( object => \&open_object,  property => \&open_property,
           v => \&open_v, m => \&open_m, e => \&open_e, t => \&open_t,
           attachment => \&open_attachment, description => \&open_description, credit => \&open_credit );

my %close=( object => \&close_object, data => \&close_data, property => \&close_property,
            v => \&close_v, m => \&close_m, e => \&close_e, t => \&close_t,
            attachment => \&close_attachment, description => \&close_description, credit => \&close_credit );

sub parse {
   my ($self, $parser)=@_;
   my $nodeType;

   for (;;) {
      $parser->read > 0 or die "empty input\n";
      last if ($nodeType=$parser->nodeType) == XML_READER_TYPE_ELEMENT;
      if ($nodeType == XML_READER_TYPE_PROCESSING_INSTRUCTION) {
         if ($parser->name eq "pm") {
            if (!$force_verification && $parser->value =~ /\bchk="([[:xdigit:]]+)"/) {
               $self->chk=hex($1);
            }
         }
      }
      # silently ignore comments or other unexpected declarations at the begin
   }

   # leading element
   if ($parser->namespaceURI ne $pmns) {
      die "the root data element does not belong to the current polymake XML namespace\n";
   }
   my $nsprefix=$parser->prefix;

   my $attrs=get_attrs($parser);
   if (defined (my $version=$attrs->{version})) {
      # check this as the very first thing:
      # if we need to transform the input file, any state changes should be avoided
      my $version_num=eval("v".$version);
      if ($version_num ne $VersionNumber) {
         if ($version_num lt $VersionNumber) {
            @{$self->transforms}=grep { $version_num lt $_->version } @{ $core_transforms //= init Transform() };
         } else {
            warn_print( defined($self->filename) ? ("file ", $self->filename) : ("<input string>"),
                        " has been created with a newer polymake version $version while yours is $Version;\n",
                        "Proceeding at your own risk!\n" );
            undef $self->chk;
         }
      }
   } else {
      undef $self->chk;
   }

   if (defined (my $ext=$attrs->{ext})) {
      provide_ext($self, $ext, 1);
   }

   if (@{$self->transforms}) {
      if (has_applicable_transforms($self)) {
         undef $self->chk;
         create_doc_tree($self, $parser);
         apply_transforms($self);
      } else {
         # no need to transform anything, but the version number has to be bumped
         $self->cur_object->[0]->changed=1;
         $self->cur_object->[0]->ensure_save_changes;
      }
   }

   if (defined $self->chk) {
      verify_integrity($self);
   }
   if (!$self->trusted) {
      my $verbose= $Verbose::files && defined $self->filename && !defined($self->doc_tree);
      dbg_print( "validating XML file ", $self->filename ) if $verbose;
      create_doc_tree($self, $parser);
      state $rngschema=new XML::LibXML::RelaxNG(location => $InstallTop."/xml/datafile.rng");
      $rngschema->validate($self->doc_tree);
      dbg_print( "XML validation succeeded" ) if $verbose;
      $PropertyType::trusted_value=0;
   }

 PARSE_TOP: {
      if (defined $self->doc_tree) {
         $parser=new XML::LibXML::Reader(DOM => $self->doc_tree, no_blanks=>1);
         $parser->document;  # without this pointless call, $parser will leak the DOM tree upon own destroying
         for (;;) {
            $parser->read > 0 or die "empty input after transformation\n";
            last if $parser->nodeType == XML_READER_TYPE_ELEMENT;
         }
         $attrs=get_attrs($parser);
      }

      my $object=$self->cur_object->[0];
      my $is_object= $parser->localName eq "object";
      my $proto=eval_qualified_type($self, $attrs->{type}, $is_object)
                // die "invalid top-level element type: $@\n";

      if (instanceof Object($object)) {
         $is_object or die "top-level element must be `object'\n";
         bless $object, $proto->pkg;
         if (defined (my $name=$attrs->{name})) {
            $object->set_name_trusted($name);
         }
         push @{$self->cur_proto}, $proto;
         $self->trusted or $object->begin_init_transaction;

      } else {
         $is_object and die "top-level element must be `data'\n";
         if (defined (my $value=$attrs->{value})) {
            push @{$self->cur_value}, [ downgradeUTF8($value) ];
         } else {
            push @{$self->cur_value}, [ ];
         }
         push @{$self->cur_property}, [ $object->mock_property($proto) ];
      }
      @{$self->cols}=(undef);

      while ($parser->read > 0) {
         $nodeType=$parser->nodeType;
         if ($nodeType == XML_READER_TYPE_ELEMENT) {
            $self->trusted || $nsprefix eq $parser->prefix
              or die "unexpected XML namespace ", $parser->namespaceURI, "\n";
            $attrs=get_attrs($parser);

            if (defined (my $ext=$attrs->{ext})) {
               if (provide_ext($self, $ext)) {
                  if (@{$self->transforms}) {
                     if (has_applicable_transforms($self)) {
                        # restart from the beginning
                        reset_data($self);
                        undef $self->chk;
                        create_doc_tree($self);
                        apply_transforms($self);
                        redo PARSE_TOP;
                     } elsif (!$self->cur_object->[0]->changed) {
                        $self->cur_object->[0]->changed=1;
                        $self->cur_object->[0]->ensure_save_changes;
                     }
                  }
               } else {
                  # skip the element from an unknown extension
                  save_backup($self);
                  skip_node($parser) ? redo : next;
               }
            }

            if ($open{$parser->localName}->($self, $attrs)) {
               if ($parser->isEmptyElement) {
                  if (defined (my $close=$close{$parser->localName})) {
                     $close->($self);
                  }
               }
            } else {
               # skip the unknown property
               save_backup($self);
               skip_node($parser) and redo;
            }

         } elsif ($nodeType == XML_READER_TYPE_END_ELEMENT) {
            $close{$parser->localName}->($self);

         } elsif ($nodeType == XML_READER_TYPE_TEXT || $nodeType == XML_READER_TYPE_CDATA) {
            $self->text .= $parser->value;
         }
      }
   }
   1
}

# private:
sub get_attrs {
   my ($parser)=@_;
   my %attrs;
   if ($parser->moveToFirstAttribute) {
      do {
         $attrs{$parser->name}=$parser->value;
      } while ($parser->moveToNextAttribute);
      $parser->moveToElement;
   }
   \%attrs;
}

# private:
sub skip_node {
   my ($parser)=@_;
   unless ($parser->isEmptyElement) {
      my $d=$parser->depth;
      for (;;) {
         # The behavior of nextSibling seems to depend on the parser source:
         # For text input (file or string) it moves to the next sibling or to the closing parent tag,
         # while for DOM tree it fails when the current node is the last child of its parent.
         # This inconsistency is obviously a bug we have to get around.
         if ($parser->nodeType == XML_READER_TYPE_ELEMENT && $parser->nextSibling) {
            if ($parser->nodeType == XML_READER_TYPE_ELEMENT && $parser->depth==$d
                or
                $parser->nodeType == XML_READER_TYPE_END_ELEMENT && $parser->depth==$d-1) {
               return 1;
            }
         } else {
            $parser->read or last;
            last if $parser->nodeType == XML_READER_TYPE_END_ELEMENT && $parser->depth==$d;
         }
      }
   }
   0   # proceed with the next regular "read"
}
#############################################################################################
sub parse_file {
   my ($self, $fh)=@_;
   $self->source=$fh;
   eval {
      parse($self, new XML::LibXML::Reader(IO => $fh, no_blanks=>1));
   } or die "invalid data file ", $self->filename, ": $@\n";
}

sub parse_string {
   my $self=shift;
   $self->source=\($_[0]);
   eval {
      parse($self, new XML::LibXML::Reader(string => $_[0], no_blanks=>1))
   } or die "invalid XML string: $@\n";
}
#############################################################################################
sub provide_ext {
   my ($self, $ext_attr, $mandatory)=@_;
   $ext_attr =~ /^\s*/g;
   my $ret=1;
   while ($ext_attr =~ /\G (\d+) (?: = ([^\s\#]+) (?: \#([\d.]+))? )? (?:\s+|$)/xgc) {
      $self->ext->[$1] //= do {
         my $URI=downgradeUTF8($2 // die "invalid reference to an extension #$1 before its introduction\n");
         my $version=$3;
         if (my $ext=provide Extension($URI, $mandatory)) {
            my $version_num=defined($version) ? eval("v$version") : "";
            if ($version_num ne $ext->version_num) {
               if ($version_num lt $ext->version_num) {
                  push @{$self->transforms},
                       grep { $version_num lt $_->version } @{ $ext_transforms{$ext} //= init Transform($ext->dir) };
               } else {
                  warn_print( defined($self->filename) ? ("file ", $self->filename) : ("<input string>"),
                              " has been created with a newer version $version of extension ", $ext->URI,
                              " while yours is ", $ext->version // "UNDEFINED", ";\n",
                              "Proceeding at your own risk!\n" );
                  undef $self->chk;
               }
            }
            1
         } else {
            0
         }
      } or $ret=0;     # skip the data
   }
   if ($ext_attr =~ /\G\s*\S/) {
      die "attribute `ext' has an invalid value starting with '", substr($ext_attr, $+[0]-1, 30), "'\n"
   }
   $ret
}
#############################################################################################
sub verify_integrity {
   my ($self)=@_;
   if (defined $self->filename) {
      my ($fsize, $mtime)=(stat $self->filename)[7,9];
      $self->trusted= $self->chk==($mtime^$fsize) || $self->chk==($mtime-1^$fsize);
   } else {
      $self->trusted= $self->chk==length(${$self->source});
   }
}
#############################################################################################
sub eval_qualified_type {
   my ($self, $type, $is_object, $in_tree)=@_;
   if (my ($app_name, $type_expr)=map { downgradeUTF8($_) } $type =~ /^(?:($id_re)::)? ($type_re)$/xo) {
      my $app= defined($app_name)
               ? add Application($app_name) :
               $in_tree
               ? $self->cur_proto->[-1]->application
               : die "application name missing in the top-level element type\n";
      while ($type_expr =~ /(?<!::)\b(\w+)::/g) {
         add Application($1);
      }
      $app->eval_type(($is_object ? "objects::" : "props::").$type_expr);
   } else {
      die "malformed type attribute '$type'\n";
   }
}

sub extract_type {
   my ($self, $attrs, $is_object)=@_;
   if (defined (my $type=$attrs->{type})) {
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

sub open_object {
   my ($self, $attrs)=@_;
   my $prop=$self->cur_property->[-1]->[0];
   my $type=extract_type($self, $attrs, 1);
   if (defined($type)) {
      $type=$prop->type->final_type($type) if $prop->flags & $Property::is_augmented;
   } else {
      $type=$prop->subobject_type;
   }
   my $sub_obj=Object::__new($type->pkg, $attrs->{name});
   $sub_obj->parent=$self->cur_object->[-1]->value;    # during the subobject construction
   push @{$self->cur_object}, $sub_obj;
   push @{$self->cur_proto}, $type;
   1
}

sub close_object {
   my ($self)=@_;
   pop @{$self->cur_proto};
   my $obj=pop @{$self->cur_object};
   if (@{$self->cur_value}) {
      push @{$self->cur_value->[-1]}, $obj;
      undef $obj->parent;
   }
}

sub open_property {
   my ($self, $attrs)=@_;
   my $prop_name=downgradeUTF8($attrs->{name});
   if ($prop_name =~ s/^($id_re):://o) {
      add Application($1);
   }
   if (defined (my $prop=$self->cur_proto->[-1]->lookup_property($prop_name))) {
      push @{$self->cur_property}, [ $prop, extract_type($self, $attrs, 0) ];
      if (defined (my $value=$attrs->{value})) {
         push @{$self->cur_value}, [ downgradeUTF8($value) ];
      } else {
         push @{$self->cur_value}, [ exists $attrs->{undef} ? undef : () ];
      }
      $self->text="";
      1
   } else {
      my @message=("encountered unknown property '$prop_name' in object ", $self->cur_proto->[-1]->full_name);
      if ($reject_unknown_properties) {
         die @message, "\n";
      } else {
         warn_print(@message);
         state $tell_once= print $console <<'.';

Please be aware that all unknown properties are automatically removed
from the data files.
If you want to supply additional information with the objects, please use
attachments or declare new properties in extension rules.

.
      }
      0
   }
}

sub close_property {
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
   } elsif (@$value_ptr) {
      if ($prop->flags & $Property::is_multiple) {
         $self->cur_object->[-1]->_add_multis($prop, $value_ptr, $self->trusted);
      } else {
         $self->cur_object->[-1]->_add($prop, $value_ptr->[0], $self->trusted);
      }
   }
}

sub open_attachment {
   my ($self, $attrs)=@_;
   my $name=downgradeUTF8($attrs->{name});
   my $type=extract_type($self, $attrs, 0);
   my $construct=$attrs->{construct};
   push @{$self->cur_property}, [ $name, $type, defined($construct) && downgradeUTF8($construct) ];
   if (defined (my $value=$attrs->{value})) {
      push @{$self->cur_value}, [ downgradeUTF8($attrs->{value}) ];
   } else {
      push @{$self->cur_value}, [ ];
   }
   $self->text="";
   1
}

sub close_attachment {
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

sub close_data {
   my ($self)=@_;
   my $type=(pop @{$self->cur_property})->[0]->type;
   my ($value)=@{ pop @{$self->cur_value} };
   $self->cur_value=$type->construct->($value);
}

sub open_description {
   my ($self)=@_;
   $self->text="";
   1
}

sub close_description {
   my ($self)=@_;
   $self->cur_object->[-1]->description=$self->text;
}

sub open_credit {
   my ($self, $attrs)=@_;
   push @{$self->cur_property}, $attrs->{product};
   $self->text="";
   1
}

sub close_credit {
   my ($self)=@_;
   my $product=pop @{$self->cur_property};
   $self->cur_object->[-1]->credits->{$product}= $self->cur_proto->[-1]->application->credits->{$product} || $self->text;
}

sub open_v {
   my ($self, $attrs)=@_;
   my $value=[ ];
   if (defined (my $dim=$attrs->{dim})) {
      set_array_flags($value, $dim, "dim");
   }
   if (defined (my $i=$attrs->{i})) {
      push @{$self->cur_value->[-1]}, $i+0;
   }
   push @{$self->cur_value}, $value;
   $self->text="";
   1
}

sub close_t {
   my ($self)=@_;
   pop @{$self->cols};
   my $list=pop @{$self->cur_value};
   if (!@$list && $self->text =~ /\S/) {
      split_list_value($self->text, $list);
   }
   push @{$self->cur_value->[-1]}, $list;
   $self->text="";
}

sub close_v {
   my ($self)=@_;
   my $list=pop @{$self->cur_value};
   if (!@$list) {
      if ($self->text =~ /\S/) {
         split_list_value($self->text, $list);
      } elsif (defined (my $cols=$self->cols->[-1])) {
         set_array_flags($list, $cols, "dim");
      }
   }
   push @{$self->cur_value->[-1]}, $list;
   $self->text="";
}

sub split_list_value {
   my ($text, $list)=@_;
   if ($text =~ /['"]/) {
      $text=downgradeUTF8($text);
      while ($text =~ /\G\s* (?: (['"]) (.*?) \1 | (\S+))/xg) {
         push @$list, defined($1) ? $2 : $3;
      }
   } else {
      $text =~ s/^\s+//;
      @$list=split /\s+/, downgradeUTF8($text);
   }
}

sub open_m {
   my ($self, $attrs)=@_;
   my $value=[ ];
   my $cols=$attrs->{cols};
   if (defined (my $dim=$attrs->{dim})) {
      set_array_flags($value, $dim, "dim");
   } elsif (defined $cols) {
      set_array_flags($value, $cols, "cols");
   }
   push @{$self->cols}, $cols;
   push @{$self->cur_value}, $value;
   1
}

sub close_m {
   my ($self)=@_;
   pop @{$self->cols};
   my $list=pop @{$self->cur_value};
   push @{$self->cur_value->[-1]}, $list;
}

sub open_e {
   my ($self, $attrs)=@_;
   &store_element_index;
   $self->text="";
   1
}

sub close_e {
   my ($self)=@_;
   push @{$self->cur_value->[-1]}, downgradeUTF8($self->text);
}

sub open_t {
   my ($self, $attrs)=@_;
   my $value=[ ];
   set_array_flags($value, -1);
   &store_element_index;
   $self->text="";
   push @{$self->cols}, undef;
   push @{$self->cur_value}, $value;
   1
}

sub store_element_index {
   my ($self, $attrs)=@_;
   if (defined (my $i=$attrs->{i})) {
      my $vec=$self->cur_value->[-1];
      if (!@$vec && defined (my $cols=$self->cols->[-1])) {
         set_array_flags($vec, $cols, "dim");
      }
      push @$vec, $i+0;
   }
}

sub save_backup {
   my ($self)=@_;
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
   [ '$filename' => 'Cwd::abs_path(#1)' ],
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

   my $reader=new XMLreader($self->filename, $object);
   local $PropertyType::trusted_value=1;
   $reader->trusted=$self->is_compressed;
   $reader->parse_file($fh);
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
   my $reader=new XMLreader($self->filename, new LooseData());
   local $PropertyType::trusted_value=1;
   $reader->trusted=$self->is_compressed;
   $reader->parse_file($fh);
   if (!$reader->trusted && !$suppress_auto_save && -w $self->filename) {
      save_data($self, $reader->cur_value, $reader->cur_object->[0]->description);
   }
   wantarray ? ($reader->cur_value, $reader->cur_object->[0]->description) : $reader->cur_value;
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
   my ($scope)=@_;
   $scope->begin_locals;
   local_sub(\&XMLreader::verify_integrity, sub { $_[0]->trusted=1 });
   $scope->end_locals;
}

sub enforce_validation {
   my ($scope)=@_;
   $scope->begin_locals;
   local_sub(\&XMLreader::verify_integrity, sub { $_[0]->trusted=0 });
   local_incr($suppress_auto_save);
   $scope->end_locals;
}

sub reject_unknown_properties {
   my ($scope)=@_;
   $scope->begin_locals;
   local_incr($XMLreader::reject_unknown_properties);
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
sub transaction { undef }
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
#
#  XML Writer
#
package Polymake::Core::XMLwriter;
use XML::Writer;

my $noXML="noXML\n";

sub complain_no_serialization : method {
   die "don't know how to serialize ", $_[0]->full_name, " to XML\n";
}

# setting of the final method pointers is deferred until the first use
# because during the initial load phase not all type descriptors might be available
sub set_methods : method {
   my $proto=shift;
   eval { generate_methods($proto, $proto->cppoptions->descr) };
   if ($@) {
      if ($@ eq $noXML) {
         $proto->toXML=\&complain_no_serialization;
         $proto->toXMLschema=\&complain_no_serialization;
      } else {
         die $@;
      }
   }
   # if called from the writer, do the real job right now
   if (@_) {
      $proto->toXML->(@_);
   }
}

sub generate_methods {
   my ($proto, $type_descr)=@_;
   if ($proto->toXML && $proto->toXML != \&set_methods && $proto->cppoptions->descr == $type_descr) {
      # reuse already generated methods
      return ($proto->toXML, $proto->toXMLschema);
   }
   my ($toXML, $toXMLschema)=do {
      if ($type_descr->is_container) {
         if ($type_descr->is_assoc_container) {
            generate_methods_for_assoc_container($type_descr);
         } elsif ((my $dim=$type_descr->own_dimension)==2) {
            generate_methods_for_matrix($type_descr);
         } elsif ($dim==1) {
            if ($type_descr->is_sparse_container) {
               generate_methods_for_sparse_container($type_descr);
            } else {
               generate_methods_for_dense_container($type_descr);
            }
         } else {
            die $noXML;
         }
      } elsif ($type_descr->is_composite) {
         generate_methods_for_composite($type_descr->member_types, $type_descr->member_descrs);
      } elsif ($type_descr->is_serializable) {
         generate_methods_for_serialized($type_descr);
      } else {
         die $noXML;
      }
   };
   if ($proto->cppoptions->descr == $type_descr) {
      # cache the methods for later use
      $proto->toXML=$toXML;
      $proto->toXMLschema=$toXMLschema;
   }
   ($toXML, $toXMLschema)
}
#############################################################################################
sub write_sparse_sequence {
   my ($toXML, $tag, $arr, $writer, $constraints, @attrs)=@_;
   my $dim=scalar @$arr;
   if (my $size=$arr->size) {
      if ($size == $dim) {
         $writer->startTag($tag, @attrs);
         foreach my $elem (@$arr) {
            $toXML->($elem, $writer);
         }
      } else {
         push @attrs, dim => $dim unless $constraints eq "!dim";
         $writer->startTag($tag, @attrs);
         for (my $it=args::entire($arr); $it; ++$it) {
            $toXML->($it->deref, $writer, undef, i => $it->index);
         }
      }
      $writer->endTag($tag);
   } else {
      push @attrs, dim => $dim unless $constraints eq "!dim";
      $writer->emptyTag($tag, @attrs);
   }
}

sub write_schema_for_sparse_sequence {
   my ($proto, $toXMLschema, $tag, $writer, $constraints, @attrs)=@_;
   $writer->startTag("element", name => $tag);
   $writer->emptyTag("ref", name => $_) for @attrs;
   $writer->startTag("choice");
   $writer->startTag("group");
   $writer->startTag("zeroOrMore");
   produce_schema_for_complex_type($proto, $toXMLschema, $writer);
   $writer->endTag("zeroOrMore");
   $writer->endTag("group");
   $writer->startTag("group");
   $writer->emptyTag("ref", name => "SparseContainerDim") unless $constraints eq "!dim";
   $writer->startTag("zeroOrMore");
   $toXMLschema->($writer, undef, "ElementIndex");
   $writer->endTag("zeroOrMore");
   $writer->endTag("group");
   $writer->endTag("choice");
   $writer->endTag("element");
}
#############################################################################################
sub write_simple_sparse_sequence {
   my ($value_proto, $arr, $writer, $constraints, @attrs)=@_;
   my $dim=scalar @$arr;
   if (my $size=$arr->size) {
      if ($size == $dim) {
         if ($value_proto->toString) {
            $writer->dataElement("v", join(" ", map { $value_proto->toString->($_) } @$arr), @attrs);
         } else {
            $writer->dataElement("v", join(" ", @$arr), @attrs);
         }
      } else {
         push @attrs, dim => $dim unless $constraints eq "!dim";
         $writer->startTag("v", @attrs);
         $writer->setDataMode(0);
         for (my $it=args::entire($arr); $it; ++$it) {
            $writer->characters(" ");
            $writer->dataElement("e", $value_proto->toString->($it->deref), i => $it->index);
         }
         $writer->characters(" ");
         $writer->setDataMode(1);
         $writer->endTag("v");
      }
   } else {
      push @attrs, dim => $dim unless $constraints eq "!dim";
      $writer->emptyTag("v", @attrs);
   }
}

sub write_schema_for_simple_sparse_sequence {
   my ($value_proto, $writer, $constraints, @attrs)=@_;
   $writer->startTag("element", name => "v");
   $writer->emptyTag("ref", name => $_) for @attrs;
   $writer->startTag("choice");
   produce_schema_for_simple_type_list($value_proto, $writer);
   $writer->startTag("group");
   $writer->emptyTag("ref", name => "SparseContainerDim") unless $constraints eq "!dim";
   $writer->startTag("zeroOrMore");
   $writer->startTag("element", name => "e");
   $writer->emptyTag("ref", name => "ElementIndex");
   produce_schema_for_simple_type($value_proto, $writer);
   $writer->endTag("element");
   $writer->endTag("zeroOrMore");
   $writer->endTag("group");
   $writer->endTag("choice");
   $writer->endTag("element");
}
#############################################################################################
sub generate_methods_for_sparse_container {
   my ($type_descr)=@_;
   my $value_proto=$type_descr->value_type;
   my $value_descr=$type_descr->value_descr;
   defined($value_proto) && defined($value_descr) or die $noXML;
   if ($value_proto->toXML) {
      my $tag= $value_proto->dimension==0 ? "v" : "m";
      my ($value_toXML, $value_toXMLschema)=generate_methods($value_proto, $value_descr);
      ( sub {
           write_sparse_sequence($value_toXML, $tag, @_);
        } ,
        sub {
           write_schema_for_sparse_sequence($value_proto, $value_toXMLschema, $tag, @_);
        } )
   } else {
      ( sub {
           write_simple_sparse_sequence($value_proto, @_);
        } ,
        sub {
           write_schema_for_simple_sparse_sequence($value_proto, @_);
        } )
   }
}
#############################################################################################
sub generate_methods_for_dense_container {
   my ($type_descr)=@_;
   my $value_proto=$type_descr->value_type;
   my $value_descr=$type_descr->value_descr;
   defined($value_proto) && defined($value_descr) or die $noXML;
   if ($value_proto->toXML) {
      my $tag= $value_proto->dimension==0 ? "v" : "m";
      my ($value_toXML, $value_toXMLschema)=generate_methods($value_proto, $value_descr);
      ( # XML writer
        sub {
           my ($value, $writer, $constraints, @attrs)=@_;
           if (@$value) {
              $writer->startTag($tag, @attrs);
              foreach my $elem (@$value) {
                 $value_toXML->($elem, $writer);
              }
              $writer->endTag($tag);
           } else {
              $writer->emptyTag($tag, @attrs);
           }
        } ,
        # schema writer
        sub {
           my ($writer, $constraints, @attrs)=@_;
           $writer->startTag("element", name => $tag);
           $writer->emptyTag("ref", name => $_) for @attrs;
           $writer->startTag("zeroOrMore");
           produce_schema_for_complex_type($value_proto, $value_toXMLschema, $writer);
           $writer->endTag("zeroOrMore");
           $writer->endTag("element");
        } )

   } else {
      ( # XML writer
        $value_proto->toString
        ? sub {
             my ($value, $writer, $constraints, @attrs)=@_;
             if (@$value) {
                $writer->dataElement("v", join(" ", map { $value_proto->toString->($_) } @$value), @attrs);
             } else {
                $writer->emptyTag("v", @attrs);
             }
          }
        : sub {
             my ($value, $writer, $constraints, @attrs)=@_;
             if (@$value) {
                $writer->dataElement("v", join(" ", @$value), @attrs);
             } else {
                $writer->emptyTag("v", @attrs);
             }
          } ,
        # schema writer
        sub {
           my ($writer, $constraints, @attrs)=@_;
           $writer->startTag("element", name => "v");
           $writer->emptyTag("ref", name => $_) for @attrs;
           produce_schema_for_simple_type_list($value_proto, $writer);
           $writer->endTag("element");
        } )
   }
}
##############################################################################################
sub generate_methods_for_matrix {
   my ($type_descr)=@_;
   my $row_proto=$type_descr->value_type;
   my $row_descr=$type_descr->value_descr;
   defined($row_proto) && defined($row_descr) or die $noXML;

   my $may_have_gaps=$type_descr->is_sparse_serialized;
   my $write_cols=$row_descr->is_sparse_container || $row_descr->is_set;
   my ($row_toXML, $row_toXMLschema)=generate_methods($row_proto, $row_descr);
   $may_have_gaps
   ? ( # XML writer
       sub {
          my $value=shift;
          write_sparse_sequence($row_toXML, "m", args::rows($value), @_);
       } ,
       # schema writer
       sub {
          write_schema_for_sparse_sequence($row_proto, $row_toXMLschema, "m", @_);
     } )
   : ( # XML writer
       sub {
          my ($value, $writer, $constraints, @attrs)=@_;
          push @attrs, cols => $value->cols if $write_cols || !@$value;
          if (@$value) {
             $writer->startTag("m", @attrs);
             my $constraint= $write_cols ? "!dim" : "";
             foreach my $row (@$value) {
                $row_toXML->($row, $writer, $constraint);
             }
             $writer->endTag("m");
          } else {
             $writer->emptyTag("m", @attrs);
          }
       } ,
       # schema writer
       sub {
          my ($writer, $constraints, @attrs)=@_;
          $writer->startTag("element", name => "m");
          $writer->startTag("optional") unless $write_cols;
          $writer->emptyTag("ref", name => "NumberColumns");
          $writer->endTag("optional") unless $write_cols;
          $writer->emptyTag("ref", name => $_) for @attrs;
          $writer->startTag("zeroOrMore");
          produce_schema_for_complex_type($row_proto, $row_toXMLschema, $writer, "!dim");
          $writer->endTag("zeroOrMore");
          $writer->endTag("element");
       } )
}
##############################################################################################
sub generate_methods_for_assoc_container {
   my ($type_descr)=@_;
   my ($pair_toXML, $pair_toXMLschema)=generate_methods_for_composite([ $type_descr->key_type, $type_descr->value_type ],
                                                                      [ $type_descr->key_descr, $type_descr->value_descr ]);
   ( # XML writer
     sub {
        my ($value, $writer, $constraints, @attrs)=@_;
        if (keys %$value) {
           $writer->startTag("m", @attrs);
           while (my ($k, $v)=each %$value) {
              $pair_toXML->([$k, $v], $writer);
           }
           $writer->endTag("m");
        } else {
           $writer->emptyTag("m");
        }
     },
     # schema writer
     sub {
        my ($writer, $constraints, @attrs)=@_;
        $writer->startTag("element", name => "m");
        $writer->emptyTag("ref", name => $_) for @attrs;
        $writer->startTag("zeroOrMore");
        $pair_toXMLschema->($writer);
        $writer->endTag("zeroOrMore");
        $writer->endTag("element");
     } )
}
############################################################################################
# simple data type within a complex tuple
sub generate_method_for_simple_item {
   my ($proto)=@_;
   sub {
      my ($value, $writer)=@_;
      $writer->dataElement("e", $proto->toString->($value));
   }
}
#############################################################################################
sub generate_methods_for_composite {
   my ($member_types, $member_descrs)=@_;
   defined($member_types) && defined($member_descrs) or die $noXML;
   my ($complex, $formatted)=(0, 0);
   my (@member_to_XML, @member_to_XMLschema);
   my $i=0;
   foreach my $member_proto (@$member_types) {
      defined($member_proto) or die $noXML;
      if ($member_proto->toXML) {
         if (!$complex) {
            # fill the missing methods for preceding simple members
            for (my $j=0; $j<$i; ++$j) {
               push @member_to_XML, generate_method_for_simple_item($member_types->[$j]);
               push @member_to_XMLschema, undef;
            }
         }
         my ($toXML, $toXMLschema)=generate_methods($member_proto, $member_descrs->[$i]);
         push @member_to_XML, $toXML;
         push @member_to_XMLschema, $toXMLschema;
         $complex=1;
      } elsif ($complex) {
         push @member_to_XML, generate_method_for_simple_item($member_proto);
         push @member_to_XMLschema, undef;
      } elsif ($member_proto->toString) {
         $formatted=1;
      }
   } continue { ++$i }

   if ($complex) {
      ( # XML writer
        sub {
           my ($value, $writer, $constraints, @attrs)=@_;
           $writer->startTag("t", @attrs);
           my $i=0;
           foreach my $member (@$value) {
              $member_to_XML[$i]->($member, $writer);
           } continue { ++$i }
           $writer->endTag("t");
        },
        # schema writer
        sub {
           my ($writer, $constraints, @attrs)=@_;
           $writer->startTag("element", name => "t");
           $writer->emptyTag("ref", name => $_) for @attrs;
           my $i=0;
           foreach my $toXMLschema (@member_to_XMLschema) {
              if (defined $toXMLschema) {
                 produce_schema_for_complex_type($member_types->[$i], $toXMLschema, $writer);
              } else {
                 $writer->startTag("element", name => "e");
                 produce_schema_for_simple_type($member_types->[$i], $writer);
                 $writer->endTag("element");
              }
           } continue { ++$i }
           $writer->endTag("element");
        } )

   } else {
      ( # XML writer
        $formatted
        ? sub {
             my ($value, $writer, $constraints, @attrs)=@_;
             my $i=0;
             $writer->dataElement("t", join(" ", map { $member_types->[$i++]->toString->($_) } @$value), @attrs);
          }
        : sub {
             my ($value, $writer, $constraints, @attrs)=@_;
             $writer->dataElement("t", join(" ", @$value), @attrs);
          } ,
        # schema writer
        sub {
           my ($writer, $constraints, @attrs)=@_;
           $writer->startTag("element", name => "t");
           $writer->emptyTag("ref", name => $_) for @attrs;
           $writer->startTag("list");
           foreach my $member_proto (@$member_types) {
              produce_schema_for_simple_type($member_proto, $writer);
           }
           $writer->endTag("list");
           $writer->endTag("element");
        } )
   }
}
#############################################################################################
sub generate_methods_for_serialized {
   my ($type_descr)=@_;
   my $serialized_proto=$type_descr->serialized_type;
   my $serialized_descr=$type_descr->serialized_descr;
   defined($serialized_proto) && defined($serialized_descr) or die $noXML;
   my ($toXML, $toXMLschema)=generate_methods($serialized_proto, $serialized_descr);
   ( sub {
        my $serialized=CPlusPlus::convert_to_serialized(shift);
        $toXML->($serialized, @_);
     },
     $toXMLschema
   )
}
#############################################################################################
# called once per type, stores final method pointers
sub type_toXMLschema : method {
   my $proto=shift;
   if ($proto->toXML) {
      # complex type: first call will set the final methods
      $proto->toXML->();

   } elsif ($proto->XMLdatatype) {
      $proto->toXMLschema=sub {
         my ($writer)=@_;
         my @datatypes=$proto->XMLdatatype->();
         $writer->startTag("choice") if @datatypes>1;
         foreach my $datatype (@datatypes) {
            if (is_object($datatype)) {
               # refers to another simple type
               produce_schema_for_simple_type($datatype, $writer);
            } elsif ($datatype =~ /^$id_re$/o) {
               # standard type from the XSD library
               $writer->emptyTag("data", type => $datatype);
            } elsif ($datatype =~ /^(?:text-)?pattern:\s*/) {
               $writer->startTag("data", type => "string");
               $writer->dataElement("param", $', name => "pattern");
               $writer->endTag("data");
            } else {
               die "unrecognizable XML datatype for simple type ", $proto->name, ": $datatype\n";
            }
         }
         $writer->endTag("choice") if @datatypes>1;
      };
   } else {
      $proto->toXMLschema=sub { $_[0]->emptyTag("text") };
   }

   $proto->toXMLschema->(@_);
}

sub element_name_for_type {
   my ($proto)=@_;
   $proto->application->name."-".$proto->xml_name
}

sub element_name_for_property_type {
   &element_name_for_type.'-prop'
}

sub element_name_for_property {
   my ($prop)=@_;
   element_name_for_type($prop->belongs_to).".".$prop->name
}

sub element_name_for_contents {
   &element_name_for_type.'-contents'
}

#############################################################################################
sub produce_schema_for_complex_type {
   my ($proto, $toXMLschema, $writer, $constraints)=@_;
   $writer->{':types'}->{$proto} //= do {
      push @{$writer->{':type_queue'}}, $proto;
      ""
   };
   if (!$constraints && $toXMLschema == $proto->toXMLschema) {
      $writer->emptyTag("ref", name => element_name_for_type($proto));
   } else {
      $toXMLschema->($writer, $constraints);
   }
}

sub produce_schema_for_simple_type {
   my ($proto, $writer)=@_;
   if ($proto->XMLdatatype) {
      $writer->{':types'}->{$proto} //= do {
         push @{$writer->{':type_queue'}}, $proto;
         ""
      };
      $writer->emptyTag("ref", name => element_name_for_type($proto));
   } else {
      $writer->emptyTag("text");
   }
}

sub produce_schema_for_simple_type_list {
   my ($proto, $writer)=@_;
   if ($proto->XMLdatatype) {
      my @datatypes=$proto->XMLdatatype->();
      if (@datatypes==1 && is_string($datatypes[0]) && $datatypes[0] =~ /^text-pattern:\s+/) {
         my $pattern=$';
         $writer->startTag("data", type => "string");
         $writer->dataElement("param", "($pattern)?(\\s+($pattern))*", name => "pattern");
         $writer->endTag("data");
      } else {
         $writer->startTag("list");
         $writer->startTag("zeroOrMore");
         &produce_schema_for_simple_type;
         $writer->endTag("zeroOrMore");
         $writer->endTag("list");
      }
   } else {
      $writer->emptyTag("text");
   }
}
#############################################################################################
sub produce_schema_for_LooseData {
   my ($writer, @protos)=@_;
   $writer->startTag("element", name => "data");
   $writer->emptyTag("ref", name => "PolymakeVersion");
   $writer->startTag("choice") if @protos>1;
   foreach my $proto (@protos) {
      $writer->startTag("group") if @protos>1;
      $writer->startTag("attribute", name => "type");
      $writer->dataElement("value", $proto->qualified_name);
      $writer->endTag("attribute");
      if ($proto->extension) {
         $writer->emptyTag("ref", name => "Extensions");
      }
      $writer->startTag("optional");
      $writer->startTag("element", name => "description");
      $writer->emptyTag("text");
      $writer->endTag("element");
      $writer->endTag("optional");
      $writer->emptyTag("ref", name => element_name_for_type($proto));
      $writer->endTag("group") if @protos>1;
   }
   $writer->endTag("choice") if @protos>1;
   $writer->endTag("element");
}
#############################################################################################
sub produce_schema_for_Object {
   my ($proto, $writer, $constraints)=@_;
   if ($constraints ne "!top") {
      $writer->startTag("define", name => element_name_for_type($proto));
      $writer->startTag("element", name => "object");
      $writer->emptyTag("ref", name => "PolymakeVersion");
      $writer->startTag("attribute", name => "type");
      $writer->dataElement("value", $proto->qualified_name);
      $writer->endTag("attribute");
      $writer->emptyTag("ref", name => "Extensions");
      $writer->emptyTag("ref", name => element_name_for_contents($proto));
      $writer->endTag("element");
      $writer->endTag("define");
   }

   $writer->startTag("define", name => element_name_for_contents($proto));
   $writer->emptyTag("ref", name => "ObjectTextDescriptions");

   $writer->startTag("oneOrMore");
   $writer->startTag("element", name => "property");
   $writer->startTag("choice");
   my %seen;
   foreach my $obj_proto ($proto, @{$proto->linear_isa}) {
      while (my ($prop_name, $prop)=each %{$obj_proto->properties}) {
         if (!($prop->flags & ($Property::is_permutation | $Property::is_non_storable))) {
            if ($obj_proto != $proto) {
               $prop=$proto->property($prop_name);   # get the correct instantiation
            }
            $writer->{':props'}->{$prop} //= do {
               push @{$writer->{':prop_queue'}}, $prop;
               ""
            };
            $seen{$prop}++ or $writer->emptyTag("ref", name => element_name_for_property($prop));
         }
      }
   }
   $writer->endTag("choice");
   $writer->endTag("element");
   $writer->endTag("oneOrMore");

   $writer->startTag("zeroOrMore");
   $writer->emptyTag("ref", name => "Attachment");
   $writer->endTag("zeroOrMore");
   $writer->endTag("define");
}

sub produce_schema_for_subobject {
   my ($proto, $writer, $parent_proto)=@_;
   my @derived=instanceof ObjectType::Augmented($proto)
               ? values %{$proto->inst_cache}
               : grep { not instanceof ObjectType::Augmented($_) || instanceof ObjectType::Specialization($_) || $_->abstract } $proto->derived;
   $writer->startTag("choice") if @derived;

   foreach my $obj_proto ($proto, @derived) {
      $writer->{':types'}->{$obj_proto} //= do {
         push @{$writer->{':type_queue'}}, $obj_proto;
         "!top"
      };
      $writer->startTag("element", name => "object");
      if ($obj_proto != $proto) {
         $writer->startTag("attribute", name => "type");
         my $qual_type=$obj_proto->pure_type->qualified_name($parent_proto->application);
         if ($qual_type =~ /^${id_re}::/) {
            # always contains application name
            $writer->dataElement("value", $qual_type);
         } else {
            # might miss application name
            $writer->startTag("choice");
            $writer->dataElement("value", $qual_type);
            $writer->dataElement("value", $obj_proto->pure_type->application->name."::".$qual_type);
            $writer->endTag("choice");
         }
         $writer->endTag("attribute");
      }
      $writer->emptyTag("ref", name => element_name_for_contents($obj_proto));
      $writer->endTag("element");
   }
   $writer->endTag("choice") if @derived;
}

sub prepare_schema_for_property_type {
   my ($proto, $writer)=@_;
   $writer->{':types'}->{$proto} //= do {
      push @{$writer->{':type_queue'}}, $proto;
      ""
   };
   # some derived types might be instantiated later
   if (my @derived=grep { !instanceof PropertyTypeInstance($_) } $proto->derived
         or
       defined($proto->generic) && @{$proto->generic->derived_abstract}) {
      my $prop_type_name=element_name_for_property_type($proto);
      $writer->emptyTag("ref", name => $prop_type_name);
      $writer->{':prop_types'}->{$prop_type_name} //= $proto;
      foreach $proto (@derived) {
         $writer->{':types'}->{$proto} //= do {
            push @{$writer->{':type_queue'}}, $proto;
            ""
         };
      }
   } else {
      $writer->emptyTag("ref", name => element_name_for_type($proto));
   }
}

sub produce_schema_for_property {
   my ($prop, $writer)=@_;
   $writer->startTag("define", name => element_name_for_property($prop));
   $writer->startTag("attribute", name => "name");
   $writer->dataElement("value", $prop->qual_name);
   $writer->endTag("attribute");

   if ($prop->flags & $Property::is_subobject) {
      if ($prop->flags & $Property::is_multiple) {
         $writer->startTag("oneOrMore");
      }
      produce_schema_for_subobject($prop->type, $writer, $prop->belongs_to);
      if ($prop->flags & $Property::is_multiple) {
         $writer->endTag("oneOrMore");
      }
   } else {
      $writer->startTag("choice");
      $writer->startTag("attribute", name => "undef");
      $writer->dataElement("value", "true");
      $writer->endTag("attribute");

      if ($prop->flags & $Property::is_subobject_array) {
         $writer->startTag("element", name => "m");
         $writer->startTag("zeroOrMore");
         produce_schema_for_subobject($prop->type->params->[0], $writer, $prop->belongs_to);
         $writer->endTag("zeroOrMore");
         $writer->endTag("element");

      } elsif ($prop->type->toXML) {
         prepare_schema_for_property_type($prop->type, $writer);

      } elsif ($prop->type->XMLdatatype) {
         $writer->startTag("attribute", name => "value");
         produce_schema_for_simple_type($prop->type, $writer);
         $writer->endTag("attribute");

      } else {
         $writer->emptyTag("ref", name => "Text");
      }

      $writer->endTag("choice");
   }
   $writer->endTag("define");
}
#############################################################################################
sub produce_schema_for_property_type {
   my ($name, $proto, $writer)=@_;
   $writer->startTag("define", name => $name);
   $writer->startTag("choice");
   $writer->emptyTag("ref", name => element_name_for_type($proto));
   foreach my $derived (grep { !instanceof PropertyTypeInstance($_) } $proto->derived) {
      $writer->{':types'}->{$derived} // die "internal error: derived type ", $derived->full_name, " spuriously instantiated during schema creation\n";
      $writer->startTag("group");
      $writer->startTag("attribute", name => "type");
      $writer->dataElement("value", $derived->full_name);
      $writer->endTag("attribute");
      $writer->emptyTag("ref", name => element_name_for_type($derived));
      $writer->endTag("group");
   }
   $writer->endTag("choice");
   $writer->endTag("define");
}
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
                      $type->pure_type != $expected_type->pure_type
                      ? (type_attr($type->pure_type, $parent),
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
         write_subobject($writer, $pv, $object, $pv->property->subobject_type, $scope);
         $writer->endTag("property");
      } elsif ($pv->flags & $PropertyValue::is_ref) {
         # TODO: handle explicit references to other objects some day...
      } elsif ($pv->property->flags & $Property::is_multiple) {
         $writer->startTag( "property", name => $pv->property->qual_name, @ext );
         write_subobject($writer, $_, $object, $pv->property->subobject_type, $scope) for @{$pv->values};
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
#############################################################################################
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
#############################################################################################
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
#############################################################################################
sub save_schema {
   my ($output, @queue)=@_;
   my (%types, %prop_types, %props, @prop_queue);
   $types{$_}="" for @queue;

   my $writer=new XMLwriter($output, PREFIX_MAP => { $RelaxNGns=>"" });
   $writer->{':types'}=\%types;
   $writer->{':type_queue'}=\@queue;
   $writer->{':props'}=\%props;
   $writer->{':prop_types'}=\%prop_types;
   $writer->{':prop_queue'}=\@prop_queue;

   $writer->xmlDecl;
   $writer->startTag([ $RelaxNGns, "grammar" ],
                     datatypeLibrary => $W3CSchemaDatatypes,
                     ns => $pmns);
   $writer->emptyTag("include", href => "file://$InstallTop/xml/common_grammar.rng");

   $writer->startTag("define", name => "PolymakeVersion");
   $writer->startTag("attribute", name => "version");
   $writer->dataElement("value", $Version);
   $writer->endTag("attribute");
   $writer->endTag("define");

   $writer->startTag("start");
   if (instanceof PropertyType($queue[0])) {
      produce_schema_for_LooseData($writer, @queue);
   } else {
      $writer->startTag("choice") if @queue > 1;
      foreach my $proto (@queue) {
         $writer->emptyTag("ref", name => element_name_for_type($proto));
      }
      $writer->endTag("choice") if @queue > 1;
   }
   $writer->endTag("start");

   while (defined (my $prop=shift @prop_queue) ||
          defined (my $proto=shift @queue)) {
      if (defined $prop) {
         produce_schema_for_property($prop, $writer);
      } elsif (instanceof ObjectType($proto)) {
         produce_schema_for_Object($proto, $writer, $types{$proto});
      } else {
         $writer->startTag("define", name => element_name_for_type($proto));
         $proto->toXMLschema->($writer, $types{$proto});
         $writer->endTag("define");
      }
   }

   while (my ($name, $proto)=each %prop_types) {
      produce_schema_for_property_type($name, $proto, $writer);
   }

   $writer->endTag("grammar");
   $writer->end;
}
#############################################################################################
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

sub save_schema {
   my $self=shift;
   my $compress=layer_for_compression($self);
   my ($of, $of_k)=new OverwriteFile($self->filename, ":utf8".$compress);
   my $kind;
   XMLwriter::save_schema($of, map {
      if ($_->abstract) {
         die "sorry, can't create a schema for a generic parametrized type, please specify a concrete instance\n";
      }
      if (instanceof ObjectType($_)) {
         if ($kind eq "small") {
            die "can't mix big object types and property types in the same schema\n";
         }
         $kind //= "big";
         if (instanceof ObjectType::Augmented($_)) {
            warn_print( "can't create a standalone schema for an augmented subobject type;\nwill create a schema for ", $_->pure_type->full_name, " instead\n" );
            $_->pure_type
         } elsif (instanceof ObjectType::Specialization($_)) {
            if (defined ($_->full_spez_for)) {
               warn_print( "can't create a standalone schema for a type specialization;\nwill create a schema for ", $_->full_spez_for->full_name, " instead\n" );
               $_->full_spez_for
            } else {
               die "can't create a standalone schema for a partial type specialization\n";
            }
         } else {
            $_
         }
      } elsif (instanceof PropertyType($_)) {
         if ($kind eq "big") {
            die "can't mix big object types and property types in the same schema\n";
         }
         $kind //= "small";
         $_
      } else {
         die "argument ", ref($_), " is neither a big object nor a property type\n";
      }
   } @_);
   close $of;
}

#############################################################################################
package Polymake::Core::XMLstring;
use Encode;

sub load {
   shift;
   my $object=shift;
   my $reader=new XMLreader(undef, $object);
   local $PropertyType::trusted_value=0;
   $object->begin_init_transaction;
   $reader->parse_string(@_);
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
