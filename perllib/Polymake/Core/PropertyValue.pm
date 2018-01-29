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

package Polymake::Core::PropertyValue;

use Polymake::Struct (
   [ new => '$;$$' ],
   [ '$property' => '#1' ],     # Property
   [ '$value' => '#2' ],
   [ '$flags' => '#3' ],        # *ref flags
);

use Polymake::Enum qw( is: temporary=1024 strong_ref=1 weak_ref=2 ref=3 );

######################################################################
#
#  Constructor
#
#  new PropertyValue(Property, data, [ flags ]);
#
sub new {
   my $self=&_new;
   if ($self->flags & $is_ref) {
      weak($self->value) if $self->flags & $is_weak_ref;
      readonly($self->value);
   } else {
      readonly_deep($self->value);
   }
   $self;
}
######################################################################
sub DESTROY {
   my ($self)=@_;
   readwrite($self->value);
}
######################################################################
declare $UNDEF="==UNDEF==\n";
declare $UNDEF_re=qr/^\s*==UNDEF==\s*$/;

sub toString {
   my ($self)=@_;
   defined($self->value) ? $self->property->type->toString->($self->value) : $UNDEF;
}

sub is_temporary { $_[0]->flags & $is_temporary }

sub delete_from_subobjects {
   my ($self, $trans)=@_;
   if ($self->property->flags & $Property::is_subobject_array) {
      delete @{$trans->subobjects}{@{$self->value}};
   }
}

sub copy {
   my ($self, $parent_obj, $to_prop)=@_;
   $to_prop //= $self->property;
   if ($self->flags & $is_ref || !defined($self->value)) {
      # TODO: special treatment for inner-object-tree references
      new($self, $to_prop, $self->value, $self->flags);
   } else {
      $to_prop->copy->($self->value, $parent_obj, $to_prop != $self->property);
   }
}
######################################################################
package _::BackRefToParent;

use Polymake::Struct (
   [ new => '$;$' ],
   [ '$owner' => 'weak(#1)' ],
);

sub value { $_[0]->owner->parent }
sub property { $_[0]->owner->property }
sub is_temporary { $_[0]->owner->is_temporary }
sub flags { $is_weak_ref }
*copy=\&new;

sub toString { '@PARENT' }

######################################################################
package __::Multiple;

use Polymake::Struct (
   [ new => '$$' ],
   [ '$property' => '#1' ],
   [ '$values' => '#2' ],
   [ '$created_by_rule' => 'undef' ],
);

sub value : method {
   my ($self)=@_;
   if (Object::_expect_array_access()) {
      $self->values;
   } else {
      $self->values->[0];
   }
}

sub toString {
   my ($self)=@_;
   @{$self->values} ? $self->property->type->toString->($self->value) : $PropertyValue::UNDEF;
}

sub flags { 0 }
sub is_temporary { 0 }

sub select_now {
   my ($self, $index)=@_;
   if (is_object($index)) {
      defined($self->created_by_rule) && defined($index=$self->created_by_rule->{$index})
        or return;
   }
   if ($index && @_==3) {
      my $scope=($_[2] //= new Scope());
      $scope->begin_locals;
      local_swap($self->values, 0, $index);
      $scope->end_locals;
      $index=0;
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
   my $self=shift; shift; # parent
   my $proto=$self->property->type;
   my (@props, @values);
   if (is_string($_[0])) {
      for (my $i=0; $i<$#_; ++$i) {
         push @props, $proto->property($_[$i]);
         push @values, $_[++$i];
      }
   } elsif (is_hash($_[0])) {
      my $list=shift;
      while (my ($prop, $value)=each %$list) {
         push @props, $proto->property($prop);
         push @values, $value;
      }
   } else {
      croak( "PROPERTY => value or { PROPERTY => value, ... } are allowed as multiple property filter expression" );
   }
   my $index=-1;
 OBJ:
   foreach my $obj (@{$self->values}) {
      ++$index;
      my ($i, $content_index, $pv)=(0);
      foreach my $prop (@props) {
         defined ($content_index=$obj->dictionary->{$prop->key})  &&
         defined ($pv=$obj->contents->[$content_index]) &&
         !$obj->diff_properties($pv, $values[$i++])
           or next OBJ;
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
   my ($self, $parent, $filter)=@_;
   &find_instance // do {
      my $need_commit;
      if (!defined($parent->transaction)) {
         $need_commit=1;
         $parent->begin_transaction;
      }
      if (!@{$self->values}) {
         push @{$parent->contents}, $self;
         $parent->dictionary->{$self->property->key}=$#{$parent->contents};
      }
      my $temp= @_%2 && pop;
      my $obj=$self->property->accept->($self->property->type->pure_type->construct->(is_hash($filter) ? (%$filter) : (), @_), $parent, 0, $temp);
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
   my $obj=pop;
   my ($self, $parent, $filter)=@_;
   $obj=$self->property->accept->($obj, $parent);
   ensure_unique_name($self, $obj);
   if (my ($index, $old)=&find_instance) {
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
   my $sub_index=list_index($self->values, $sub_obj);
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
