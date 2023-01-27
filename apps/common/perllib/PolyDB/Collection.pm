#  Copyright (c) 1997-2023
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
#
#  This file is part of the polymake database interface polyDB.
#
#   @author Silke Horn, Andreas Paffenholz
#   http://www.mathematik.tu-darmstadt.de/~paffenholz
#

package PolyDB::Collection;

@ISA = qw( MongoDB::Collection );

use Try::Tiny;

sub prime {
   my ($pkg, $col_handler) = @_;
   bless $col_handler, $pkg;
}

sub drop() {
   die "please use the method remove_collection to remove a collection\n";
}

sub get_own_info_collection {
   my ($self) = @_;
   my $col_info_name = "_collectionInfo." . $self->name;
   $self->database->get_collection($col_info_name)
}

sub get_own_info {
   my ($self) = @_;
   $self->{".cache"}{info} //= do {
      my $info_col = get_own_info_collection($self);
      my $info = try {
         $info_col->find_one({_id => "info.".$default::db_polydb_version})
      } catch {
         die_neatly($_, $info_col);
      };
      if (defined($info)) {
         $self->{".cache"}{info} = $info;
      } else {
         die "can't find collection info record for collection ", $self->name, " - connected to a wrong PolyDB version?\n";
      }
   }
}

sub get_own_schema {
   my ($self) = @_;
   $self->{".cache"}{schema} //= do {
      my $info = $self->get_own_info();
      if (defined(my $schema_key = $info->{schema})) {
         my $info_col = &get_own_info_collection;
         my $schema_doc = try {
            $info_col->find_one({_id => $schema_key})
         } catch {
            die_neatly($_, $info_col);
         };
         if (defined($schema_doc) && is_hash($schema_doc->{schema})) {
            new Polymake::Schema( Schema::document2schema($schema_doc->{schema}) )
         } else {
            die "collection schema $schema_key not found in ", $info_col->name, " - corrupt metadata?\n";
         }
      }
   }
}

sub deserialize_safe {
   my ($data) = @_;
   if (defined($data)) {
      Core::Serializer::deserialize($data)
   } else {
      undef
   }
}

sub make_cursor {
   my ($self, $query, $projection, $raw) = @_;
   my %options;
   if (defined($projection)) {
      $options{projection} = sanitize_projection($projection);
   }
   if ( $raw ) {
      prime Cursor($self->SUPER::find($query, \%options));
   } else {
      prime Cursor($self->SUPER::find($query, \%options), \&deserialize_safe);
   }
}

sub find_one_impl {
   my ($self, $query, $projection, $sort, $raw) = @_;
   if (defined($projection)) {
      $projection = sanitize_projection($projection);
   }
   my $result = try {
      $self->SUPER::find_one($query, $projection, { sort => $sort });
   } catch {
      die_neatly($_, $self);
   };
   $raw ? $result : deserialize_safe($result)
}

# FIXME distinct does not have a sort option
# FIXME the official workaround is to use an aggregate pipeline with a group and sort phase
sub distinct_impl {
   my ($self, $query, $property) = @_;
   try {
      $self->database->run_command([
         distinct => $self->name,
         key      => $property,
         query    => $query
      ])->{values};
   } catch {
      die_neatly($_, $self);
   }
}

sub count_impl {
   my ($self, $query) = @_;
   try {
      $self->database->run_command([
         count => $self->name,
         query => $query
      ])->{n};
   } catch {
      die_neatly($_, $self);
   }
}

sub count_distinct_impl {
   my ($self, $query, $property, $as) = @_;
   try {
      $self->SUPER::aggregate([
         keys %$query ? ({ '$match' => $query }) : (),
         { '$group' => { _id => "\$$property" } },
         { '$count' => $as }
      ])->next;
   } catch {
      die_neatly($_, $self);
   }
}

sub sanitize_projection {
   my ($projection) = @_;
   my (%result, $is_positive);
   unless (is_hash($projection)) {
      croak( "invalid projection specification: must be a hash map" );
   }
   while (my ($k, $v) = each %$projection) {
      if (is_integer($v)) {
         if ($v == 0) {
            # silently ignore suppressing important metadata properties, also in subobjects
            next if ($k =~ /(?:^|\.)_(?:id|ns|info|type|attrs|ext|load|polyDB)\b/);
         } elsif ($v != 1) {
            croak( "invalid projection specification: allowed values are 0 and 1" );
         }
         if (($is_positive //= $v) != $v) {
            croak( "invalid projection specification: mixed 0's and 1's" );
         }
         $result{$k} = $v;
      } else {
         croak( "invalid projection specification: allowed values are 0 and 1" );
      }
   }
   if ($is_positive) {
      # silently add important metadata properties
      $result{$_} = 1 for qw( _ns _info _type _attrs _ext _load _polyDB );
   }
   keys %result ? \%result : undef
}  

sub list_property_completions {
   my ($self, $text) = @_;
   if (defined(my $schema = get_own_schema($self))) {
      # do not propose meta-properties
      $text =~ s/(?:^|\.)\K$/(?:[a-zA-Z]|_id)/;
      $schema->list_property_completions($text);
   } else {
      ()
   }
}

sub replace_one {
   my ($collection, $id, $query) = @_;
   try {
      $collection->SUPER::replace_one({ "_id" => $id}, $query);
   } catch {
      die_neatly($_, $collection);
   }
}

sub insert_one {
   my ($collection, $query) = @_;
   try {
      $collection->SUPER::insert_one($query);
   } catch {
      die_neatly($_, $collection);
   }
}


sub insert_many {
   my ($collection, $query) = @_;
   try {
      $collection->SUPER::insert_many([ @$query ]);
   } catch {
      die_neatly($_, $collection);
   }
}

# the current date as a string in the form yyyy-mm-dd
sub get_date {
   my $now = `date '+%Y-%m-%d'`;
   chomp $now;
   return $now;
}

sub insert_or_replace {
   my ($self, $obj, $options) = @_;

   my $metadata = $obj->get_attachment("polyDB");
   
   ($metadata->{section}, $metadata->{collection}) = $self->{name} =~ /([\w.]+)\.([\w-]+)/;
   if ( defined($options->{uri}) ) {
      $metadata->{uri} = $options->{uri};
   }
   if ( defined($options->{creation_date}) ) {
      $metadata->{creation_date} = $options->{creation_date};
   }
   $metadata->{creation_date} //= get_date();
   $metadata->{version} = $PolyDB::default::db_polydb_version;

   $obj->remove_attachment("polyDB");
   $obj->attach("_polyDB",$metadata);

   my $polymake_object = $options->{schema} ? Core::Serializer::serialize($obj, { schema => $options->{schema} } ) : Core::Serializer::serialize($obj);
   if ( defined($options->{id}) ) { $polymake_object->{_id} = $options->{id}; }
   $polymake_object->{_ns}->{polymake}->[1] = $options->{polymake_version} // $Polymake::Version;

   my $replace = $options->{replace};
   if ($self->find_one({'_id' => $polymake_object->{_id} } ) && !$options->{noinsert}) {
      if ( !$replace ) {
         print "id $id already exists\n" if $options->{verbose};
         return 0;
      }
   } else {
      $replace = false;
   }

   my $output = $options->{noinsert} || $replace ? $self->replace_one($id,$polymake_object) : $self->insert_one($polymake_object);
   die "Error while inserting into database\n" if ( !$options->{noinsert} && !$output->acknowledged );

   return 1;
}

sub insert_array_impl {
   my ($self, $obj_array_ref, $options) = @_;

   my $metadata = ();
   ($metadata->{section}, $metadata->{collection}) = $self->{name} =~ /([\w.]+)\.([\w-]+)/;
   if ( defined($options->{uri}) ) {
      $metadata->{uri} = $options->{uri};
   }

   $metadata->{creation_date} = defined($options->{creation_date}) ? $options->{creation_date} : get_date();
   $metadata->{version} = $PolyDB::default::db_polydb_version;

   my $polymake_object_array = [];
   foreach my $obj (@$obj_array_ref) {
      print "adding ".$obj->name."\n" if $options->{verbose};
      $obj->remove_attachment("polyDB");
      $obj->attach("_polyDB",$metadata);
      my $polymake_object = $options->{schema} ? Core::Serializer::serialize($obj, { schema => $options->{schema} } ) : Core::Serializer::serialize($obj);
      $polymake_object->{_ns}->{polymake}->[1] = $options->{polymake_version} // $Polymake::Version;
      $polymake_object->{_id} = $obj->name; # objects must have a unique name in the collection. This is not checked!
      push @$polymake_object_array, $polymake_object;
   }

   print "writing to database\n" if $options->{verbose};
   my $output = $self->insert_many($polymake_object_array);
   die "Error while inserting into database\n" if !$output->acknowledged;

   return 1;
}


1;


# Local Variables:
# mode: perl
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
