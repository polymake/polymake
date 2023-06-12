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

use strict;
use namespaces;
use warnings qw(FATAL void syntax misc);

package Polymake::Core::PropertyValue;

use Polymake::Struct (
   [ new => '$;$$' ],
   [ '$property' => '#1' ],     # Property
   [ '$value' => '#2' ],
   [ '$flags' => '#3' ],        # *ref flags
);

use Polymake::Enum Flags => {
   is_temporary => 1024,
   is_strong_ref => 1,
   is_weak_ref => 2,
   is_ref => 3,
   is_shortcut => 4
};

######################################################################
#
#  Constructor
#
#  new PropertyValue(Property, data, [ flags ]);
#
sub new {
   my $self = &_new;
   if ($self->flags & Flags::is_ref) {
      weak($self->value) if $self->flags & Flags::is_weak_ref;
      readonly($self->value);
   } else {
      readonly_deref($self->value);
   }
   $self;
}
######################################################################
declare $UNDEF="==UNDEF==\n";
declare $UNDEF_re=qr/^\s*==UNDEF==\s*$/;

sub toString {
   my ($self)=@_;
   defined($self->value) ? $self->property->type->toString->($self->value) : $UNDEF;
}

sub is_temporary { $_[0]->flags & Flags::is_temporary }

sub is_storable {
   my ($self, $allow_shortcuts) = @_;
   if (not($self->property->flags & Property::Flags::is_non_storable
           or $self->flags & Flags::is_weak_ref)) {
      if ($self->flags & Flags::is_shortcut) {
         $allow_shortcuts && $allow_shortcuts->($self->property->name)
      } else {
         true
      }
   }
}

sub delete_from_subobjects {
   my ($self, $trans)=@_;
   if ($self->property->flags & Property::Flags::is_subobject_array) {
      delete @{$trans->subobjects}{@{$self->value}};
   }
}

sub copy {
   my ($self, $parent_obj, $to_prop) = @_;
   $to_prop //= $self->property;
   if ($self->flags & Flags::is_ref || !defined($self->value)) {
      # TODO: special treatment for inner-object-tree references
      new($self, $to_prop, $self->value, $self->flags);
   } else {
      $to_prop->copy->($self->value, $parent_obj, $to_prop != $self->property);
   }
}

sub serialize {
   my ($self, $options) = @_;
   if (is_object($self->value)) {
      my $type = $self->value->type;
      my $result;
      if (defined(my $prescribed_type = $options->{type})) {
         $prescribed_type ||= $self->property->type;
         $result = $prescribed_type->serialize->($type == $prescribed_type ? $self->value : $prescribed_type->construct->($self->value), $options);
         $type = $prescribed_type;
      } else {
         $result = $type->serialize->($self->value, $options);
      }
      if ($type == $self->property->type) {
         $result
      } else {
         ($result, $type)
      }
   } else {
      $self->value
   }
}

######################################################################
package Polymake::Core::PropertyValue::BackRefToParent;

use Polymake::Struct (
   [ new => '$;$' ],
   [ '$owner' => 'weak(#1)' ],
);

sub value { $_[0]->owner->parent }
sub property { $_[0]->owner->property }
sub is_temporary { $_[0]->owner->is_temporary }
sub is_storable { false }
sub flags { Flags::is_weak_ref }
*copy=\&new;

sub toString { '@PARENT' }
sub serialize { () }

######################################################################
package Polymake::Core::PropertyValue::Multiple;

use Polymake::Struct (
   [ new => '$$' ],
   [ '$property' => '#1' ],
   [ '$values' => '#2' ],
   [ '$created_by_rule' => 'undef' ],
);

sub value : method {
   my ($self)=@_;
   if (BigObject::_expect_array_access()) {
      $self->values;
   } else {
      $self->values->[0];
   }
}

sub toString {
   my ($self)=@_;
   @{$self->values} ? $self->property->type->toString->($self->value) : $PropertyValue::UNDEF;
}

sub serialize {
   my ($self, $options) = @_;
   if (defined(my $schema = $options->{schema})) {
      my $result = [ map { ($options->{schema} = $schema->[$_]) ? $self->values->[$_]->serialize($options) : () } 0..$#$schema ];
      @$result ? $result : ()
   } else {
      [ map { $_->serialize($options) } @{$self->values} ]
   }
}

sub flags { 0 }
sub is_temporary { false }
sub is_storable { true }

sub select_now {
   my ($self, $index)=@_;
   if (is_object($index)) {
      defined($self->created_by_rule) && defined($index = $self->created_by_rule->{$index})
        or return;
   }
   if ($index > 0) {
      local with($_[2]) {
         local swap @{$self->values}, 0, $index;
      }
      $index = 0;
   }
   $self->values->[$index]
}

####################################################################################
sub ensure_unique_name {
   my ($self, $subobj, $temp)=@_;
   my ($found, $next);
   if (defined $subobj->name) {
      foreach my $sibling (@{$self->values}) {
         if ($sibling != $subobj && $sibling->name eq $subobj->name) {
            $found=$subobj->name;
            last;
         }
      }
      return unless defined($found);
      $next=2;
   } else {
      $found= $temp ? "temp" : "unnamed";
      $next=0;
   }
   foreach my $sibling (@{$self->values}) {
      if ($sibling != $subobj && $sibling->name =~ m/^\Q$found\E\#(\d+)$/) {
         assign_max($next, $1+1);
      }
   }
   if (defined $subobj->name) {
      warn_print( "The subobject name ", $subobj->name, " already occurs among other instances of ", $self->property->name, ".\n",
                  "Replaced with a unique name '$found#$next'." );
   }
   $subobj->set_name_trusted("$found#$next");
}
######################################################################
sub copy {
   my ($self, $parent_obj, $to_prop, $filter)=@_;
   $to_prop //= $self->property;
   if (my @copies=map {
         if (!$_->is_temporary && defined (my $subcopy=$_->copy($parent_obj, $to_prop, $filter))) {
            $subcopy
         } else {
            ()
         }
      } @{$self->values}) {
      new($self, $to_prop, \@copies);
   } else {
      undef;
   }
}

sub delete_from_subobjects {
   my ($self, $trans, $old_pv)=@_;
   if (defined $old_pv) {
      delete @{$trans->subobjects}{ grep { !contains($old_pv->values, $_) } @{$self->values}};
   } else {
      delete @{$trans->subobjects}{@{$self->values}};
   }
}

# create a shallow copy for transaction backup
sub backup {
   my ($self)=@_;
   inherit_class([ $self->property, [ @{$self->values} ], undef ], $self)
}
######################################################################
# private:
sub find_instance {
   my $self = shift;
   # maybe parent:
   shift if instanceof Core::BigObject($_[0]);
   my $proto = $self->property->type;
   my (@paths, @values);
   if (is_string($_[0])) {
      for (my $i = 0; $i < $#_; ++$i) {
         push @paths, [ $proto->encode_descending_path($_[$i]) ];
         push @values, $_[++$i];
      }
   } elsif (is_hash($_[0])) {
      my $list = shift;
      while (my ($path, $value)=each %$list) {
         push @paths, [ $proto->encode_descending_path($path) ];
         push @values, $value;
      }
   } else {
      croak( "PROPERTY => value or { PROPERTY => value, ... } are allowed as multiple property filter expression" );
   }
   my $index = -1;
 OBJ:
   foreach my $obj (@{$self->values}) {
      ++$index;
      for (my $i = 0; $i < @paths; ++$i) {
         (my ($subobj, $pv) = $obj->lookup_descending_path($paths[$i])) == 2
           or next OBJ;
         $subobj->diff_properties($pv, $values[$i])
           and next OBJ;
      }
      return ($index, $obj);
   }
   ()
}

sub find {
   if ($#_==1 && ref($_[1]) eq "") {
      $_[1] eq '*' ? $_[0]->values : &find_by_name;
   } else {
      scalar(&find_instance);
   }
}

sub find_by_name {
   my ($self, $name)=@_;
   foreach (@{$self->values}) {
      return $_ if $_->name eq $name;
   }
   undef
}

sub find_or_create {
   my ($self, $parent, $filter) = @_;
   &find_instance // do {
      local interrupts(block);
      my $need_commit;
      if (!defined($parent->transaction)) {
         $need_commit = true;
         $parent->begin_transaction;
      }
      if (!@{$self->values}) {
         push @{$parent->contents}, $self;
         $parent->dictionary->{$self->property->key}=$#{$parent->contents};
      }
      my $temp = @_%2 && pop;
      my $obj = $self->property->accept->($self->property->type->pure_type->construct->(is_hash($filter) ? (%$filter) : (), @_), $parent, 0, $temp);
      ensure_unique_name($self, $obj, $temp);
      push @{$self->values}, $obj;
      if ($temp) {
         assign_max($#{$parent->transaction->temporaries}, 0);
         push @{$parent->transaction->temporaries}, [ $self->property, $obj ];
      } else {
         $parent->transaction->changed=1;
      }
      $parent->transaction->commit($parent) if $need_commit;
      $obj;
   }
}

sub replace_or_add {
   local interrupts(block);
   my $obj = pop;
   my ($self, $parent, $filter) = @_;
   $obj = $self->property->accept->($obj, $parent);
   ensure_unique_name($self, $obj);
   if (my ($index, $old) = &find_instance) {
      splice @{$self->values}, $index, 1, $obj;
      if ($old->is_temporary) {
         $parent->transaction->forget_temporary_subobject($old);
      }
      if (defined($old->transaction)) {
         delete $parent->transaction->subobjects->{$old};
         undef $old->transaction;
      }
      undef $old->parent;
      undef $old->is_temporary;
   } else {
      push @{$self->values}, $obj;
   }
}

sub remove_instance {
   my ($self, $sub_obj)=@_;
   my $sub_index = list_index($self->values, $sub_obj);
   if ($sub_index>=0) {
      splice @{$self->values}, $sub_index, 1;
   } else {
      die( "internal inconsistency: multiple subobject instance does not occur in the parent property\n" );
   }
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
