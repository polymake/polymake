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
#
#  This file is part of the polymake database interface polyDB.
#
#   @author Silke Horn, Andreas Paffenholz
#   http://solros.de
#   http://www.mathematik.tu-darmstadt.de/~paffenholz
#

use strict;

package PolyDB::DBInsert;

sub add_properties_at_level {
   my ($input_object, $property_mask,$initial,$m_initial, $multiple) = @_;

   my $required_properties = [];
   my $required_multiple = {};
   my $required_attachments = [];

   foreach my $pn ( keys %$property_mask ) {
      if ( $pn eq "attachments" ) {
         foreach my $at ( keys %{$property_mask->{$pn}} ) {
            push @$required_attachments, $initial eq "" ? $at : $initial.".".$at;
         }
      } else {
         my $new_initial = $initial eq "" ? $pn : $initial.".".$pn;
         my $new_m_initial = $m_initial eq "" ? $pn : $m_initial.".".$pn;
         my $read_request;
         if ( eval { $read_request = $input_object->type->encode_read_request($new_initial) } ) {
            if ( ref $property_mask->{$pn} eq "HASH" ) {
               my $prop = local pop @{$read_request->[0]};
               if ( $prop->flags & Core::Property::Flags::is_subobject ) {
                  my $is_multiple = $prop->flags & Core::Property::Flags::is_multiple;
                  my ($rp,$rm,$ra) = add_properties_at_level($input_object, $property_mask->{$pn},$new_initial, $is_multiple ? "" : $new_m_initial, $is_multiple ? 1 : $multiple);
                  push @$required_properties, @$rp;

                  push @$required_attachments, @$ra;
                  if ( $is_multiple ) {
                     $required_multiple->{$new_m_initial} = $rm;
                  } elsif ( $multiple ) {
                     $required_multiple = { %$required_multiple, %$rm };
                  }
               } else {
                  push @$required_properties, $new_initial;
                  if ( $multiple ) {
                     $required_multiple->{$new_m_initial} = 1;
                  }
               }
            } else {
               push @$required_properties, $new_initial;
               if ( $multiple ) {
                  $required_multiple->{$new_m_initial} = 1;
               }
            }
         }
      }
   }
   return $required_properties, $required_multiple, $required_attachments;
}

# assuming that the mask for desired properties is a nested hash map, e.g.
# $mask = { 'VERTICES' => 1, 'TRIANGULATION' => { 'FACETS' => 1, ... }, ... };

sub create_filter_for_mask {
   my ($property_mask, $original_property_mask)=@_;
   sub {
      my ($parent, $pv, $property)=@_;
      $property //= $pv->property; # if $property is defined then do nothing, else replace it with $pv->property
      my $mask_element = $property_mask->{$property->name};
      my $original_mask_element = $original_property_mask->{$property->name};

      if ( defined($mask_element) || defined($original_mask_element) ) {
         if ( (ref($mask_element) && $property->flags & Core::Property::Flags::is_subobject) &&
              ( !defined($original_mask_element) || ( ref($original_mask_element) && $property->flags & Core::Property::Flags::is_subobject) ) ) {
            # recursively copy selected properties of a subobject or of all instances of a multiple subobject
            ($parent, $pv->copy($parent, $property, create_filter_for_mask($mask_element, $original_mask_element)));
         } else {
         # either an atomic property or the entire subobject shall be copied
           ($parent, $pv->copy($parent, $property));
         }
       } else {
          # property does not occur in the mask: empty return suppresses copying
          ()
       }
   }
}

sub copy_object_with_mask_filter {
  my ($object, $property_mask,$original_property_mask) = @_;
  $object->copy(undef, undef, create_filter_for_mask($property_mask, $original_property_mask));
}

sub copy_all_attachments {
   my ($input_object,$output_object) = @_;

   $output_object->copy_attachments($input_object);
   foreach my $pv (@{$input_object->contents}) {
      if ($pv->property->flags & Core::Property::Flags::is_subobject) {
         copy_all_attachments($input_object->{$pv->{$property}},$output_object->{$property});
      }
   }
}

sub provide_multiple_properties {

   my ($input_object, $required_multiple) = @_;
   foreach my $propstring (keys %$required_multiple) {
      if ( ref($required_multiple->{$propstring}) ) {
         my $input_subobject;
         my $prop;
         if ( $propstring =~ /\./ ) {
            (my $initial,$prop) = split(/\.([^\.]+)$/, $propstring);
            $input_subobject = $input_object->give($initial);
         } else {
            $prop = $propstring;
            $input_subobject = $input_object;
         }
         foreach my $multiple_subobject (@{$input_subobject->give($prop)}) {
            provide_multiple_properties($multiple_subobject,$required_multiple->{$propstring});
         }
      } else {
         $input_object->provide($propstring);
      }
   }
}

sub adjust_properties {

   my ($input_object, $options ) = @_;

   if ( defined($options->{'type_information'}) ) {
      my $output_object;
      my $property_mask = $options->{'type_information'}->{"property_mask"};
      my $original_property_mask = {};

      if ( $options->{"keep_all_props"}) {
         my @known_props = $input_object->list_properties(1);
         foreach my $propstring (@known_props) {
            my @props = split(/\./,$propstring);
            foreach (@props) {
               s/\[.*\]$//;
            }
            my $mask_pointer = $original_property_mask;
            my $last = pop(@props);
            while (my $x = shift(@props) ) {
               if ( defined($mask_pointer->{$x})) {
                  $mask_pointer = $mask_pointer->{$x};
               } else {
                  $mask_pointer->{$x} = {};
                  $mask_pointer = $mask_pointer->{$x};
               }
            }
            $mask_pointer->{$last} = 1;
         }
      }

      my ($required_properties,$required_multiple,$required_attachments) = add_properties_at_level($input_object, $property_mask,"", "", 0);

      $input_object->provide(@$required_properties) unless $options->{"nonew"};
      provide_multiple_properties($input_object,$required_multiple) unless $options->{"nonew"};
      $output_object = copy_object_with_mask_filter($input_object,$property_mask,$original_property_mask);

      foreach my $at (@$required_attachments) {
         if ( $at =~ /\./ ) {
            my ($prop,$attachment) = split(/\.([^\.]+)$/, $at);
            my $sub_in = $input_object;
            my $sub_out = $output_object;
            while ( $prop =~ /\./ ) {
               (my $p,$prop) = split(/\./, $prop,2);
               $sub_in = $sub_in->give($p);
               $sub_out = $sub_out->give($p);
            }
            $sub_in = $sub_in->give($prop);
            $sub_out = $sub_out->give($prop);
            if ( exists($sub_in->attachments->{$attachment}) ) {
               $sub_out->attach($attachment,$sub_in->get_attachment($attachment));
            } else {
               if ( !$options->{"nonew"} ) {
                  croak("attachment ".$at." required but not provided\n");
               }
            }
         } else {
            if ( exists($input_object->attachments->{$at}) ) {
               $output_object->attach($at,$input_object->get_attachment($at));
            } else {
               if ( !$options->{"nonew"} ) {
                  croak("attachment ".$at." required but not provided\n");
               }
            }
         }
      }
      return $output_object;
   } else {
      return $input_object;
   }
}

sub prepare_xml {
   my ($obj) = @_;

   $obj->remove_attachment("polyDB");
   my $xml = save Core::XMLstring($obj);
   return $xml;
}

# the current date as a string in the form yyyy-mm-dd
sub get_date {
   my $now = `date '+%Y-%m-%d'`;
   chomp $now;
   return $now;
}

# prepare metadata
# need to define
# * creation_date
# * contributor
# * type_information_key
# * package->polymake with
#    * type
#    * app
#    * xml
# this will appear in the database as 'polyDB'-entry
sub prepare_metadata {

   my ($obj, $type_information, $options) = @_;

   my $metadata = {};
   # check if object has already polyDB attachment
   if ( defined( my $polydb_attachment = $obj->get_attachment("polyDB") ) ) {
      foreach (keys %{$polydb_attachment}) {
         $metadata->{$_} = $polydb_attachment->{$_};
      }
   }

   # set the contributor, precedence is
   # given in object > set as option > given in template
   if ( !defined( $metadata->{"contributor"}) ) {
      if ( defined($options->{"contributor"}) ) {
         $metadata->{"contributor"} = $options->{"contributor"};
      } else {
         if ( defined($type_information->{"contributor"}) ) {
            $metadata->{"contributor"} = $type_information->{"contributor"};
         }
      }
   }

   # set the creation date of the entry
   # either already stored in the object (e.g. when copying the db)
   # or we take the current day
   if ( !defined( $metadata->{"creation_date"}) ) {
      $metadata->{"creation_date"} = get_date();
   }

   # Potentially overwrite a template key from the options, if one is given in options and in the object
   if ( defined($options->{type_information_key}) ) {
      $metadata->{"type_information_key"} = $options->{type_information_key};
   }

   my $version = do {
      if ( defined($options->{'version'}) ) {
         $options->{'version'};
      } elsif ( defined($type_information->{'version'}) ) {
         $type_information->{'version'};
      } else {
         $Polymake::Version;
      }
   };
   # we replace version in the xml for the one given in options or type_information
   # this is potentially unsafe if the version used to create the document writes properties differently
   # however, this is necessary to provide accessibility of the data for older versions of polymake
   $metadata->{'package'}->{'polymake'} = {
      version => $version,
      type => $obj->type->qualified_name,
      tag => "object"
   };
   ($metadata->{"package"}->{"polymake"}->{"app"}) = $metadata->{"package"}->{"polymake"}->{"type"} =~ /^(.+?)(?=::)/;
   if ( !$options->{'noxml'} ) {
      my $xml = prepare_xml($obj);
      $xml =~ s/version="(\d+.)+\d+" xmlns/version=\"$version\" xmlns/g;
      $metadata->{'package'}->{'polymake'}->{'xml'} = $xml;
   }

   return $metadata;
}

# $obj: the original polymake object to insert
# $collection: handler for database/collection in the MongoDB
# $id: id for the object to insert
# $options: hash containing options, in particular
#   - template: the tamplate defining which properties to store in json and xml
#   - contributor:
sub insert_into_col {
   my ($obj, $collection_handler, $id, $options) = @_;

   my $use_type_information = 0;   # do we use a template file to filter the properties
   my $type_information;
   if ( defined($type_information = $options->{type_information}) ) {
      $use_type_information = 1;
   }

   my $polymake_object;
   if ( $use_type_information ) {
      # returns a new polymake object with the properties of $obj copied to it
      # if all should be kept or in template, properties required by template computed and nonoew not set
      $obj = adjust_properties($obj, $options);

      my $json_modifier = $type_information->{'modifier'} // $type_information->{'json_modifier'};

      # convert to perl array
      $polymake_object = PolymakeJsonConversion::polymake2perl($obj, {property_mask=>$type_information->{'property_mask'}, json_modifier=>$json_modifier, keep_all_props=>$options->{"keep_all_props"} });

      # additional data is a perl function that uses polymake to compute additional date from the polymake object $obj
      # this may add new properties to $obj, but this is okay as we have already stored the xml in $polymake_object
      # (it might also recompute properties that we removed during the call to adjust properties. But if, then most likely they should have been included in the properties stored in the xml)
      if ( defined($options->{'type_information'}->{'additional_data'}) ) {
         my $func = eval($options->{'type_information'}->{'additional_data'});
         $func->($obj, $polymake_object);
      }
   } else {

      # convert to perl array
      # save the given polymake object as is
      $polymake_object = PolymakeJsonConversion::polymake2perl($obj);
   }

   my $metadata = prepare_metadata($obj, $type_information, $options);
   $polymake_object->{"polyDB"} = $metadata;
   $polymake_object->{'_id'} = $id;

   # insert into database
   my $o = $options->{noinsert} || $collection_handler->insert_one($polymake_object);

   # return success
   return $o;
}

1;


# Local Variables:
# mode: perl
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
