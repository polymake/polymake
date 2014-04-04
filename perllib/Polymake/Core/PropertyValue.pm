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

package Polymake::Core::PropertyValue;

use Polymake::Struct (
   [ new => '$;$$' ],
   [ '$property' => '#1' ],	# Property
   [ '$value' => '#2' ],
   [ '$flags' => '#3' ],	# *ref flags
);

use Polymake::Enum qw( is: temporary=1 strong_ref=2 weak_ref=4 ref=6 );

######################################################################
#
#  Constructor
#
#  new PropertyValue(Property, data, [ flags ]);
#
sub new {
   my $self=&_new;
   if ($self->flags & $is_ref) {
      readonly($self->value);
      weak($self->value) if $self->flags & $is_weak_ref;
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
   my $self=shift;
   defined($self->value) ? $self->property->type->toString->($self->value) : $UNDEF;
}

sub is_temporary { (shift)->flags & $is_temporary }

sub delete_from_subobjects {
   my ($self, $trans)=@_;
   if ($self->property->flags & $Property::is_subobject_array) {
      delete @{$trans->subobjects}{@{$self->value}};
   }
}

sub copy {
   my $self=shift;
   if ($self->flags & $is_ref || !defined($self->value)) {
      new($self, @$self);
   } else {
      $self->property->copy->($self->value, @_);
   }
}
######################################################################
package Polymake::Core::PropertyMultipleValue;

use Polymake::Struct (
   [ new => '$$' ],
   [ '$property' => '#1' ],
   [ '$values' => '#2' ],
   '$has_gaps',
);

sub new {
   my $self=&_new;
   my $i=0;
   $_->parent_index=$i++ for @{$self->values};
   $self;
}

sub value : method {
   if (Object::_expect_array_access()) {
      &defined_values;
   } else {
      my $self=shift;
      if ($self->has_gaps) {
	 foreach (@{$self->values}) {
	    defined($_) and return $_;
	 }
	 undef
      } else {
	 $self->values->[0];
      }
   }
}

sub defined_values {
   my $self=shift;
   if ($self->has_gaps) {
      [ grep { defined } @{$self->values} ]
   } else {
      $self->values;
   }
}

sub toString {
   my $self=shift;
   @{$self->values} ? $self->property->type->toString->($self->value) : $PropertyValue::UNDEF;
}

sub flags { 0 }
sub is_temporary { 0 }

######################################################################
sub copy {
   my $self=shift;
   my $i=-1;
   if (my @subobjs=map { 
         if (defined($_) && !$_->is_temporary && defined (my $subcopy=$_->copy(@_))) {
            $subcopy->parent_index=++$i;
            $subcopy
         } else {
            ()
         }
      } @{$self->values}) {
      new($self, $self->property, \@subobjs);
   } else {
      undef;
   }
}

sub delete_from_subobjects {
   my ($self, $trans)=@_;
   delete @{$trans->subobjects}{@{$self->values}};
}
######################################################################
# private:
sub _find_index {
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
      if (defined($obj)) {
	 my ($i,$content_index,$pv)=(0);
	 foreach my $prop (@props) {
	    defined ($content_index=$obj->dictionary->{$prop->key})  &&
	    defined ($pv=$obj->contents->[$content_index]) &&
	    !$obj->diff_properties($pv,$values[$i++])
	    or next OBJ;
	 }
	 return $index;
      }
   }
   undef
}

sub find {
   my $self=$_[0];
   if ($#_==1 && $_[1] eq '*') {
      &defined_values;
   } elsif (defined (my $index=&_find_index)) {
      $self->values->[$index];
   } else {
      undef;
   }
}

sub find_by_name {
   my ($self, $name)=@_;
   foreach (@{$self->values}) {
      return $_ if defined($_->name) && $_->name eq $name;
   }
   undef
}

sub find_or_create {
   my ($self, $parent, $filter)=@_;
   if (defined (my $index=&_find_index)) {
      $self->values->[$index];
   } else {
      my $need_commit;
      if (!defined ($parent->transaction)) {
	 $need_commit=1;
	 $parent->begin_transaction;
      }
      if (!@{$self->values}) {
	 push @{$parent->contents}, $self;
	 $parent->dictionary->{$self->property->key}=$#{$parent->contents};
      }
      my $temp= @_%2 && pop;
      my $obj=$self->property->type->pure_type->construct->(is_hash($filter) ? (%$filter) : (), @_);
      $self->property->accept->($obj,$parent,0,$temp);
      $obj->parent_index=0+@{$self->values};
      push @{$self->values}, $obj;
      if ($temp) {
	 assign_max($#{$parent->transaction->temporaries},0);
	 push @{$parent->transaction->temporaries}, [ $self->property, $obj->parent_index ];
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
   $self->property->accept->($obj,$parent);
   if (defined (my $index=&_find_index)) {
      $obj->parent_index=$index;
      my ($old)=splice @{$self->values}, $index, 1, $obj;
      if ($old->is_temporary) {
	 $parent->transaction->drop_temp_mark($parent,$obj);
      }
      if (defined($old->transaction)) {
	 delete $parent->transaction->subobjects->{$old};
	 undef $old->transaction;
      }
      undef $old->parent_index;		# but preserve property (for destructor)
      undef $old->parent;
      undef $old->is_temporary;
   } else {
      $obj->parent_index=0+@{$self->values};
      push @{$self->values}, $obj;
   }
}

sub squeeze {
   my $self=shift;
   my $gap=0;
   for (my ($i, $last)=(0, $#{$self->values}); $i<=$last; ++$i) {
      if (defined($self->values->[$i])) {
	 $gap and ($self->values->[$i-$gap]=$self->values->[$i])->parent_index-=$gap;
      } else {
	 ++$gap;
      }
   }
   $self->has_gaps=0;
   ($#{$self->values}-=$gap)>=0
}

1

# Local Variables:
# c-basic-offset:3
# End:
