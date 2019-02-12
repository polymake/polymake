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
use feature 'state';

package Polymake::Core::Object;

use Polymake::Struct (
   [ new => ';$' ],
   [ '$name' => '#1', set_filter => 'name_filter' ],  # object name, unique among multiple subobject collections
   '$description',                      # textual description
   '%credits',                          # credit records from producing rules
   '@contents',                         # PropertyValues in the chronological (file) order
   '%dictionary',                       # Property => index in @contents
   '%attachments',                      # "name" => data object
   [ '$transaction' => 'undef' ],       # Transaction
   '$changed',                          # Object changed since last load() or save() (only for top-level objects)
   '%failed_rules | sensitive_to',      # Rule => undef  rules should not be applied to the object any more
                                        # Property->key (permutation) => contains properties sensitive to this permutation
   [ '$persistent' => 'undef' ],        # some object with load() and save() methods (only for top-level objects)
   [ '$has_cleanup' => 'undef' ],       # has registered a cleanup handler
   [ '$parent' => 'undef' ],            # parent Object
   [ '$property' => 'undef' ],          # as what this is kept in the parent object
   [ '$is_temporary' => 'undef' ],      # subobject or its ancestor was temporarily added
);

sub value { $_[0] }   # for compatibility for PropertyValue

use Polymake::Ext;

my %save_at_end;

####################################################################################
package Polymake::Core::Transaction;

use Polymake::Struct (
   [ new => '$@' ],
   [ '$content_end' => '$#{#1->contents}' ],    # length of Object->contents at the transaction start
   '%subobjects',                               # subobjects with active transactions
   '@temporaries',                              # properties and subobjects temporarily added during the transaction
   [ '$changed' => 'undef' ],                   # boolean:  object was changed during the transaction
   [ '$outer' => '#1->transaction' ],
   [ '$entirely_temp' => '$this->outer && $this->outer->entirely_temp' ],  # all changes are temporary by default
   [ '$dependent' => 'undef' ],
   '%backup',                                   # overwritten or deleted leaf or singular properties: content_index => PropertyValue
                                                # multiple subobject properties with added/deleted instances: content_index => original PropertyValue::Multiple
);

sub rule { undef }

# Lists stored under temporaries have the following structure:
# leading element is the smallest index of a gap in contents (resulting from a remove() operation),
# the rest is a mix of entries of four kinds:
# Property                       : property added temporarily
# [ Property, Object ]           : multiple subobject added temporarily
# [ Property, [ list ] ]         : single subobject with its own list of temporaries
# [ Property, Object, [ list ] ] : multiple subobject with its own list of temporaries

# compare two entries from temporaries
# => -1     : the first dominates
# => 0      : both equal
# => 1      : the second dominates
# => undef  : independent

sub cmp_temporaries {
   my ($t1, $t2)=@_;
   if (is_object($t1)) {
      if (is_object($t2)) {
         # two properties as a whole
         return $t1->key == $t2->key ? 0 : undef;
      } else {
         # a property as a whole vs. a property in a subobject
         return $t1->key == $t2->[0]->key ? -1 : undef;
      }
   }
   if (is_object($t2)) {
      # a property in a subobject vs. a property as a whole
      return $t1->[0]->key == $t2->key ? 1 : undef;
   }
   if ($t1->[0]->key != $t2->[0]->key) {
      # properties in different subobjects
      return undef;
   }
   if ($t1->[0]->flags & Property::Flags::is_multiple) {
      if ($t1->[1] != $t2->[1]) {
         # different instances of a multiple subobject
         return undef;
      }
   }
   # a multiple subobject as a whole vs. some of its properties?
   $#$t1 <=> $#$t2 || (merge_temporaries($t1->[-1], $t2->[-1]), 0);
}

sub include_temporary_item {
   my ($list, $t2)=@_;
   for (my $i=1; $i<=$#$list; ++$i) {
      if (defined (my $cmp=cmp_temporaries($list->[$i], $t2))) {
         if ($cmp>0) {
            $list->[$i]=$t2;
         }
         return;
      }
   }
   push @$list, $t2;
}

sub merge_temporaries {
   my ($dst, $src)=@_;
   assign_min($dst->[0], shift @$src);
   include_temporary_item($dst, $_) for @$src;
}

sub propagate_temporaries {
   my ($self, $object, $parent_temp)=@_;
   assign_max($#$parent_temp,0);
   include_temporary_item($parent_temp,
                          $object->property->flags & Property::Flags::is_multiple
                          ? [ $object->property, $object, $self->temporaries ]
                          : [ $object->property, $self->temporaries ]);
}

sub forget_temporary_subobject {
   my ($self, $subobj)=@_;
   my $temps=$self->temporaries;
   # the list containing the temporary object might have been moved into cleanup in the meanwhile
   # therefore it's not an error if we don't find it here
   for (my $i=1; $i<=$#$temps; ++$i) {
      my $item=$temps->[$i];
      if (!is_object($item) && $item->[1] == $subobj) {
         splice @$temps, $i, 1;
         last;
      }
   }
}

sub commit {
   my ($self, $object, $parent_trans)=@_;
   $object->transaction=$self->outer;
   $_->transaction->commit($_, $self) for keys %{$self->subobjects};

   if (defined $parent_trans) {
      if (@{$self->temporaries} && !$object->is_temporary) {
         propagate_temporaries($self, $object, $parent_trans->temporaries);
      }
      $parent_trans->changed ||= $self->changed;
      return;
   }
   if (defined $self->outer) {
      if (@{$self->temporaries} && !$object->is_temporary) {
         if (@{$self->outer->temporaries}) {
            merge_temporaries($self->outer->temporaries, $self->temporaries);
         } else {
            $self->outer->temporaries=$self->temporaries;
         }
      }
      $self->outer->changed ||= $self->changed;
      return;
   }
   if (defined (my $parent_obj=$object->parent)) {
      return if $object->is_temporary;
      if (@{$self->temporaries}) {
         if (defined ($parent_trans=$parent_obj->transaction)) {
            propagate_temporaries($self, $object, $parent_trans->temporaries);
            $parent_trans->changed ||= $self->changed;
         } else {
            $parent_trans=$parent_obj->begin_transaction;
            propagate_temporaries($self, $object, $parent_trans->temporaries);
            $parent_trans->changed=$self->changed;
            $parent_trans->commit($parent_obj);
         }
         return;
      }
      if ($self->changed) {
         do {
            $object=$parent_obj;
            if (defined $object->transaction) {
               $object->transaction->changed=1;
               return;
            }
         } while (defined($parent_obj=$object->parent));
      } else {
         return;
      }
   }
   if ($self->changed) {
      if (!$object->changed) {
         $object->changed=1;
         $object->ensure_save_changes if $object->persistent;
      }
   }
   if (@{$self->temporaries}) {
      if ($object->has_cleanup) {
         merge_temporaries($Scope->cleanup->{$object}, $self->temporaries);
      } else {
         $object->has_cleanup=1;
         $Scope->cleanup->{$object}=$self->temporaries;
      }
   }
}

sub rollback {
   my ($self, $object)=@_;
   if ((my $limit=$self->content_end)>=0) {
      while (my ($index, $old_pv)=each %{$self->backup}) {
         if ($index <= $limit) {
            if (defined( my $changed_pv=$object->contents->[$index] )) {
               $changed_pv->delete_from_subobjects($self, $old_pv);
            } else {
               $object->dictionary->{$old_pv->property->key}=$index;
            }
            $object->contents->[$index]=$old_pv;
         }
      }

      foreach my $pv (@{$object->contents}[$limit+1..$#{$object->contents}]) {
         if (ref($pv)) {
            delete $object->dictionary->{$pv->property->key};
            $pv->delete_from_subobjects($self);
         }
      }
      $#{$object->contents}=$limit;
      $_->transaction->rollback($_) for keys %{$self->subobjects};
   } else {
      $#{$object->contents}=-1;
      $object->dictionary={ };
   }
   $object->transaction=$self->outer;
}

sub descend {
   my ($self, $sub_obj, $sub_obj_is_new)=@_;
   $self->subobjects->{$sub_obj} ||= do {
      my $sub_trans_type= $sub_obj_is_new && ref($self) eq __PACKAGE__ ? "Polymake::Core::InitTransaction" : __PACKAGE__;
      if (!defined($sub_obj->transaction)
            or
          defined($sub_obj->transaction->rule)
            ? croak( "nested transaction within rule ", $sub_obj->transaction->rule->header )
            : $sub_obj->transaction->dependent) {
         $sub_obj->transaction=_new($sub_trans_type, $sub_obj);
      } else {
         bless $sub_obj->transaction, $sub_trans_type;
      }
      $sub_obj->transaction->dependent=1;
   };
   $sub_obj->transaction
}

####################################################################################
package Polymake::Core::InitTransaction;

use Polymake::Struct (
   [ '@ISA' => 'Transaction' ],
   [ '$content_end' => '-1' ],
);

sub commit {
   my ($self, $object, $parent_trans)=@_;
   until (Scheduler::resolve_initial_request($object, [ [ ObjectType::init_pseudo_prop() ], requests_for_subobjects($self, $object) ])) {
      my $err=$@;
      if (defined $parent_trans) {
         $parent_trans->rollback($object->parent);
      } else {
         &rollback;
         if (instanceof Object::Replacement($err)) {
            undef $@;
            %{$self->subobjects}=();
            $object->transaction=$self;
            $object->fill_properties($object->type, @{$err->properties});
            while (my ($name, $data)=each %{$err->attachments}) {
               $object->attach($name, $data);
            }
            next;
         }
      }
      die "initial check failed: $err\n";
   }
   &Transaction::commit;
}

# private:
sub requests_for_subobjects {
   my ($self, $parent, @descend)=@_;
   my $prop;
   map {
      $prop=$_->property;
      if ($prop->flags & Property::Flags::is_multiple) {
         my $pv=$parent->contents->[$parent->dictionary->{$prop->key}];
         if (my $index=list_index($pv->values, $_)) {
            die "internal error: stray multiple subobject instance\n" if $index<0;
            $prop=new Property::SelectMultiInstance($prop, $index);
         }
      }
      ( [ @descend, $prop, ObjectType::init_pseudo_prop() ],
        requests_for_subobjects($_->transaction, $_, @descend, $prop) )
   } keys %{$self->subobjects}
}

####################################################################################
package Polymake::Core::RuleTransaction;

use Polymake::Struct (
   [ '@ISA' => 'Transaction' ],
   [ new => '$$' ],
   [ '$rule' => '#2' ],
);

sub new {
   my $self=&_new;
   my $top_object=$_[0];
   $top_object->transaction=$self;
   foreach my $out (@{$self->rule->output}) {
      my ($obj, $prop)=$top_object->descend_and_create($out);
      my $pv=new PropertyValue($prop);
      if (defined (my $content_index=$obj->dictionary->{$prop->key})) {
         if (defined (my $pv_old=$obj->contents->[$content_index])) {
            $obj->transaction->backup->{$content_index}=$pv_old;
         } else {
            $self->changed=0;   # enforce rule execution
         }
         $obj->contents->[$content_index]=$pv;
      } else {
         push @{$obj->contents}, $pv;
         $obj->dictionary->{$prop->key}=$#{$obj->contents};
         $self->changed=0;   # enforce rule execution
      }
   }
   $self;
}

sub descend {
   my ($self, $sub_obj)=@_;
   $self->subobjects->{$sub_obj} ||= do {
      if (defined($sub_obj->transaction) && defined($sub_obj->transaction->rule)) {
         croak( "nested transaction within rule ", $sub_obj->transaction->rule->header );
      }
      $sub_obj->transaction=_new(__PACKAGE__, $sub_obj, $self->rule);
      $sub_obj->transaction->dependent=1;
   };
   $sub_obj->transaction
}

sub restore_default_multi_instance {
   my ($object, $index, $old_pv)=@_;
   # the new instance must go to the end of the list in order to reinstall the default one
   my $pv=$object->contents->[$index];
   if (@{$old_pv->values} + 1 == @{$pv->values} && $old_pv->values->[0] == $pv->values->[1]) {
      push @{$pv->values}, shift @{$pv->values};
   }
}

sub restore_backup {
   my ($self, $object)=@_;
   if (defined $self->outer) {
      while (my ($index, $pv)=each %{$self->backup}) {
         if ($index <= $self->outer->content_end) {
            $self->outer->backup->{$index} //= $pv;
         }
         if ($pv->property->flags & Property::Flags::is_multiple) {
            restore_default_multi_instance($object, $index, $pv);
         }
      }
   } else {
      while (my ($index, $pv)=each %{$self->backup}) {
         if ($pv->property->flags & Property::Flags::is_multiple) {
            restore_default_multi_instance($object, $index, $pv);
         } else {
            $object->contents->[$index]=$pv;
         }
      }
   }
}

sub commit {
   my ($self, $object)=@_;
   &restore_backup;
   if (@_==2 && defined (my $perm_deputy=$self->rule->with_permutation)) {
      # the rule just executed was the first one to create some sensitive properties:
      # record this now so as to avoid expensive search on the next scheduling
      # but only in the top-level transaction
      $perm_deputy->record_sensitivity($object);
   }
   &Transaction::commit;
}

####################################################################################
package Polymake::Core::RuleTransactionWithPerm;

use Polymake::Struct (
   [ '@ISA' => 'RuleTransaction' ],
);

sub commit {
   my ($self, $object)=@_;
   my $prod_rule=$self->rule->rule;

   # transfer the sensitive properties created by prod_rule into the permutation subobjects
   foreach my $action (@{$self->rule->actions}) {
      my $output=$prod_rule->output->[$action->output];
      my ($base_obj, $perm_obj, $prop);
      if ($action->depth) {
         {  # must create an additional permutation object in the sub-object
            local splice @$output, $action->depth + 1;
            ($base_obj)=$object->descend($output);
         }
         $perm_obj=$base_obj->add_perm($action->sub_permutation, $prod_rule);
         undef $perm_obj->transaction;
         local splice @$output, 0, $action->depth;
         ($base_obj)=$base_obj->descend($output);
         ($perm_obj, $prop)=$perm_obj->descend_and_create($output);
      } else {
         # the property is transferred into the base permutation object
         @{$self->rule->perm_path}==1 or die "internal error: wrong PermAction::depth==0\n";
         $perm_obj=$object->add_perm($self->rule->perm_path->[0], $prod_rule);
         undef $perm_obj->transaction;
         ($base_obj)=$object->descend($output);
         ($perm_obj, $prop)=$perm_obj->descend_and_create($output);
      }
      my $base_content_index=$base_obj->dictionary->{$prop->key};
      push @{$perm_obj->contents}, $base_obj->contents->[$base_content_index];
      $perm_obj->dictionary->{$prop->key}=$#{$perm_obj->contents};
      $base_obj->contents->[$base_content_index]=delete $base_obj->transaction->backup->{$base_content_index};
   }

   &restore_backup;
   &Transaction::commit;
}

####################################################################################
package Polymake::Core::Object;

# The following new_XXX routines are registered for the overloaded construct method
# by the prototype (ObjectType) constructor.
# They may not be called directly.

sub new_empty : method {
   my $self=new((shift)->pkg, name_of_ret_var());
   begin_init_transaction($self);
   $self;
}

sub new_named : method {
   my ($proto, $name)=@_;
   my $self=new($proto->pkg, $name);
   begin_init_transaction($self);
   $self;
}

sub new_filled : method {
   my $proto=shift;
   if (ref($_[0])) {
      croak( "Can't convert ", UNIVERSAL::can($_[0], "type") ? $_[0]->type->full_name : ref($_[0]), " to ", $proto->full_name );
   }
   my $self=new($proto->pkg, @_%2 ? shift : name_of_ret_var());
   fill_properties_and_commit($self, $proto, @_);
}

sub new_copy : method {
   my ($proto, $src)=@_;
   my $self=new($proto->pkg, $src->name);
   copy_contents($self, $src, $proto != $src->type && ($proto->isa($src->type) ? -1 : 1));
   $self->description=$src->description;
   copy_attachments($self, $src);
   $self;
}

sub new_filled_copy : method {
   my ($proto, $src)=splice @_, 0, 2;
   my $self=new_copy($proto, $src);
   croak( "odd PROPERTY => VALUE list" ) if @_%2;
   fill_properties_and_commit($self, $proto, @_);
}
####################################################################################
# private:
sub fill_properties {
   my ($self, $proto)=splice @_, 0, 2;
   for (my ($i, $e)=(0, $#_); $i<$e; $i+=2) {
      my ($name, $value)=@_[$i, $i+1];
      my ($obj, $prop)= $name =~ /\./ ? descend_and_create($self, [ $proto->encode_descending_path($name) ])
                                      : ($self, $proto->property($name));
      if ($prop->flags & Property::Flags::is_multiple) {
         _add_multi($obj, $prop, $value);
      } else {
         _add($obj, $prop, $value);
      }
   }
}

# private:
sub fill_properties_and_commit {
   my $self=$_[0];
   my $trans= @_ > 2 ? begin_init_transaction($self) : begin_transaction($self);
   &fill_properties;
   $trans->commit($self);
   $self
}
####################################################################################
sub _add {
   my ($self, $prop, $value, $trusted)=@_;
   if (!$trusted && exists $self->dictionary->{$prop->key}) {
      croak( "multiple values for property ", $prop->name );
   }
   push @{$self->contents}, $prop->accept->($value, $self, $trusted);
   $self->dictionary->{$prop->key}=$#{$self->contents};
}
####################################################################################
# private:
sub _add_multi {
   my ($self, $prop, $value, $temp)=@_;
   $value=$prop->accept->($value, $self, 0, $temp);
   my $pv;
   if (defined (my $content_index=$self->dictionary->{$prop->key})) {
      $pv=$self->contents->[$content_index];
      push @{$pv->values}, $value;
   } else {
      $pv=new PropertyValue::Multiple($value->property, [ $value ]);
      push @{$self->contents}, $pv;
      $self->dictionary->{$prop->key}=$#{$self->contents};
   }
   $pv
}

# protected:
sub _add_multis {
   my ($self, $prop, $values, $trusted)=@_;
   unless ($trusted) {
      if (exists $self->dictionary->{$prop->key}) {
         croak( "multiple occurence of property ", $prop->name );
      }
      my %names;
      foreach my $instance (@$values) {
         $names{$instance->name}++ and croak( "duplicate subobject name '", $instance->name, "'" );
      }
   }
   push @{$self->contents},
        new PropertyValue::Multiple($prop, [ map { $prop->accept->($_, $self, $trusted) } @$values ]);
   $self->dictionary->{$prop->key}=$#{$self->contents};
}
####################################################################################
# protected:
sub add_perm {
   my ($self, $permutation, $for_rule)=@_;
   my ($content_index, $perm_object);
   if (defined ($content_index=$self->dictionary->{$permutation->key}) &&
       defined ($perm_object=$self->contents->[$content_index]) &&
       $perm_object->name == $for_rule) {
      return $perm_object;
   }

   $perm_object=$permutation->create_subobject($self, PropertyValue::Flags::is_temporary);
   $perm_object->name=$for_rule;

   if (defined $content_index) {
      $self->contents->[$content_index]=$perm_object;
   } else {
      push @{$self->contents}, $perm_object;
      $self->dictionary->{$permutation->key}=$#{$self->contents};
   }

   assign_max($#{$self->transaction->temporaries}, 0);
   push @{$self->transaction->temporaries}, $permutation;
   $perm_object;
}

sub add_perm_in_parent {
   my ($self, $up, $permutation, $for_rule)=@_;
   while (--$up >= 0) {
      $self=$self->parent or return;
   }
   add_perm($self, $permutation, $for_rule);
}
####################################################################################
sub begin_transaction {
   my $self=shift;
   $self->transaction=new Transaction($self);
}

sub begin_init_transaction {
   my $self=shift;
   $self->transaction=new InitTransaction($self);
}
####################################################################################
sub print_me : method {
   my ($proto, $self)=@_;
   $proto->full_name . (length($self->name) ? ": ".$self->name : " anonymous object")
}
####################################################################################
sub load {
   my $self=_new(shift);
   my $ph=shift;
   ($self->persistent=is_object($ph) ? $ph : new XMLfile($ph))->load($self, @_);
   $self;
}

sub fromXMLstring {
   my $self=_new(shift);
   load XMLstring($self, @_);
   $self;
}

sub fromXMLstream {
   my $self=_new(shift);
   load_stream XMLfile($self, @_);
   $self;
}

sub toXMLstring {
   save XMLstring(@_);
}

sub reset_to_empty {
   my $self=shift;
   bless $self;
   @{$self->contents}=();
   %{$self->dictionary}=();
   %{$self->attachments}=();
   %{$self->credits}=();
   $self->description="";
   undef $self->transaction;
}

sub isa { $_[0]->type->isa($_[1]) }
####################################################################################
sub copy {
   my ($self, $new_parent, $parent_property, $filter)=@_;
   my ($copy, $adjust_types);
   if (defined $new_parent) {
      if (defined($parent_property) && $parent_property != $self->property) {
         $adjust_types=1;
         $copy=new($parent_property->subobject_type->pkg, $self->name);
      } else {
         $copy=new($self, $self->name);
      }
      weak($copy->parent=$new_parent);
      $copy->property=$parent_property // $self->property;
      $new_parent->transaction->descend($copy) if defined $filter;
   } else {
      $adjust_types=defined($self->parent) && $self->property->flags & Property::Flags::is_augmented;
      $copy=new($self->type->pure_type->pkg, $self->name);
      $copy->description=$self->description;
      begin_transaction($copy) if defined $filter;
   }
   copy_contents($copy, $self, $adjust_types, $filter);
   if (defined($new_parent) && !@{$copy->contents}) {
      delete $new_parent->transaction->subobjects->{$copy};
      undef
   } else {
      $copy
   }
}

# private:
# copy of an object made during an active production rule must be in a self-consistent state, that is, as at the rule start
sub stable_contents {
   my $self=shift;
   if (defined($self->transaction)) {
      if (defined($self->transaction->rule)) {
         return [ map { $self->transaction->backup->{$_} || $self->contents->[$_] } 0..$self->transaction->content_end ];
      } else {
         $self->commit;
      }
   }
   $self->contents;
}

# private:
sub copy_contents {
   my ($self, $src, $adjust_types, $filter)=@_;
   foreach my $pv (@{stable_contents($src)}) {
      if (defined($pv) && !$pv->is_temporary) {
         my ($subobj, $pv_copy);
         if ($adjust_types) {
            my $new_prop=$pv->property->instance_for_owner($self->type, $adjust_types<0)
              or next;
            ($subobj, $pv_copy)= defined($filter) ? $filter->($self, $pv, $new_prop) : ($self, $pv->copy($self, $new_prop));
         } else {
            ($subobj, $pv_copy)= defined($filter) ? $filter->($self, $pv) : ($self, $pv->copy($self));
         }
         if (defined $pv_copy) {
            push @{$subobj->contents}, $pv_copy;
            $subobj->dictionary->{$pv_copy->property->key}=$#{$subobj->contents};
         }
      }
   }
}
####################################################################################
sub name_filter : method {
   my ($self, $member_name, $new_name)=@_;
   if (defined(my $parent=$self->parent)) {
      if ($self->property->flags & Property::Flags::is_multiple) {
         # check for uniqueness
         my $pv=$parent->contents->[$parent->dictionary->{$self->property->key}];
         foreach my $sibling (@{$pv->values}) {
            if ($sibling != $self && $sibling->name eq $new_name) {
               croak( "Parent object '", $parent->name || "UNNAMED", "' already has another subobject ", $self->property->name, " with name '$new_name'" );
            }
         }
      }
      while (!defined($self->transaction) && defined($parent)) {
         $self=$parent;
         $parent=$self->parent;
      }
   }
   set_changed($self);
   return $new_name;
}

# to be called from C++ library
sub set_name {
   $_[0]->name=$_[1];
}

sub set_changed {
   my ($self)=@_;
   if (defined($self->transaction)) {
      $self->transaction->changed=1;
   } elsif (!$self->changed) {
      $self->changed=1;
      $self->ensure_save_changes if $self->persistent;
   }
}

# protected:
sub set_name_trusted {
   # circumvent the set filter
   $_[0]->[0]=$_[1];
}
####################################################################################
sub cast_me {
   my ($self, $target_proto)=@_;
   my $proto=$self->type;
   return $self if $target_proto == $proto;

   $proto->isa($target_proto)
      or croak( "invalid cast from ", $proto->full_name, " to ", $target_proto->full_name, "; use copy instead" );

   if (defined($self->property)) {
      if ($self->property->flags & Property::Flags::is_augmented) {
         if (!$self->property->type->pure_type->isa($target_proto)) {
            croak( "can't cast a subobject under ", $self->property->name, " to ", $target_proto->full_name );
         }
         $target_proto=$self->property->type->final_type($target_proto);
      } else {
         if (!$self->property->type->isa($target_proto)) {
            croak( "can't cast a subobject under ", $self->property->name, " to ", $target_proto->full_name );
         }
      }
   }

   my $need_commit;
   if (defined($self->transaction)) {
      croak( "production rule may not change the Object type" ) if defined($self->transaction->rule);
   } else {
      $need_commit=1;
      begin_transaction($self);
   }
   perform_cast($self, $target_proto);
   $self->transaction->changed=1;
   $self->transaction->commit($self) if $need_commit;
   $self;
}

# protected:
# change the type to/from the augmented type
# remove properties undefined for the target type
sub perform_cast {
   my ($self, $target_proto, $down)=@_;
   foreach my $pv (@{$self->contents}) {
      if (defined($pv)) {
         my $old_prop=$pv->property;
         next if $down && !($old_prop->flags & Property::Flags::is_augmented);
         if (defined (my $prop=$old_prop->instance_for_owner($target_proto, $down))) {
            if ($prop != $old_prop) {
               $pv->property=$prop;
               if ($prop->type != $old_prop->type) {
                  $old_prop->flags & Property::Flags::is_augmented
                    or croak( "internal error: upcast from ", $self->type->full_name, " to ", $target_proto->full_name,
                              " changed the type of property ", $prop->name );
                  foreach my $subobj ($prop->flags & Property::Flags::is_subobject_array
                                      ? @{$pv->value} :
                                      $prop->flags & Property::Flags::is_multiple
                                      ? @{$pv->values} : ($pv)) {
                     $self->transaction->descend($subobj) unless $down;
                     perform_cast($subobj, $prop->subobject_type->final_type($subobj->type), $down);
                  }
               }
            }
         } else {
            assign_min($self->transaction->temporaries->[0], delete $self->dictionary->{$pv->property->key});
            undef $pv;
         }
      }
   }
   bless $self, $target_proto->pkg;
}
####################################################################################
# four different sorts of descending into subobjects,
# kept separately to optimize a little on the costs of extra branching

# protected:
sub descend {
   my ($self, $path)=@_;
   my $prop = local pop @$path;
   my ($content_index, $pv);
   foreach my $prop (@$path) {
      unless (defined ($content_index=$self->dictionary->{$prop->property_key}) &&
              defined ($pv=$self->contents->[$content_index]) &&
              defined ($self=$pv->value)) {
         return;
      }
   }
   ($self, $prop);
}

# protected:
sub descend_partially {
   my ($self, $visited, $path)=@_;
   my $prop = local pop @$path;
   my ($content_index, $pv);
   foreach $prop (@$path) {
      push @$visited, $self;
      if (defined ($content_index=$self->dictionary->{$prop->property_key}) &&
          defined ($pv=$self->contents->[$content_index])) {
         if (defined ($self=$pv->value)) {
            next;
         } else {
            undef $_[0];
         }
      }
      return;
   }
   ($self, $prop);
}

# protected:
sub descend_with_transaction {
   my ($self, $path)=@_;
   my $prop = local pop @$path;
   my ($content_index, $pv);
   my $trans=$self->transaction;
   foreach $prop (@$path) {
      unless (defined ($content_index=$self->dictionary->{$prop->property_key}) &&
              defined ($pv=$self->contents->[$content_index]) &&
              defined ($self=$pv->value)) {
         return;
      }
      $trans=$trans->descend($self);
   }
   ($self, $prop);
}

# protected:
sub descend_and_create {
   my ($self, $path)=@_;
   my $prop = local pop @$path;
   my $trans = $self->transaction;
   foreach $prop (@$path) {
      my $content_index=$self->dictionary->{$prop->property_key};
      if (defined $content_index) {
         my $pv=$self->contents->[$content_index];
         if (defined $pv) {
            if ($prop->flags & Property::Flags::is_multiple_new) {
               # a rule can only create one multiple subobject instance of the same kind
               ($pv->created_by_rule //= { })->{$trans->rule} //= do {
                  $trans->backup->{$content_index}=$pv->backup;
                  my $new_instance=$pv->property->create_subobject($self, $trans->entirely_temp);
                  $pv->ensure_unique_name($new_instance, $trans->entirely_temp);
                  # the new instance must appear as the default one until the rule has finished;
                  # RuleTransaction::restore_backup will eventually bring it back to the ned of the list
                  unshift @{$pv->values}, $new_instance;
                  $#{$pv->values}
               };
               $self=$pv->value;
               next;
            } elsif (defined $pv->value) {
               $trans=$trans->descend($pv->value);
               $self=$pv->value;
               next;
            }
         }
      }
      my $pv=$prop->create_subobject($self, $trans->entirely_temp);
      if (($prop->flags & (Property::Flags::is_multiple | Property::Flags::is_multiple_new)) == Property::Flags::is_multiple) {
         # a new multiple subobject instance created outside rules, e.g. by take()
         $pv=new PropertyValue::Multiple($pv->property, [ $pv ]);
      }
      if (defined $content_index) {
         $self->contents->[$content_index]=$pv;
      } else {
         push @{$self->contents}, $pv;
         $self->dictionary->{$prop->property_key}=$#{$self->contents};
      }
      $self=$pv->value;
   }
   ($self, $prop);
}
####################################################################################
# protected:
sub select_multi_instance {
   my ($self)=@_;
   my $pv=$self->parent->contents->[$self->parent->dictionary->{$self->property->key}];
   if (my $index=list_index($pv->values, $self)) {
      die "internal error: stray multiple subobject instance\n" if $index<0;
      $pv->select_now($index, $_[1] //= new Scope());
   }
}

sub set_as_default_sanity_checks {
   my ($self)=@_;
   defined(my $prop=$self->property) or croak("Selected object is not a subobject at all");
   $prop->flags & Property::Flags::is_multiple or croak("Selected subobject is not of a multiple type");
   if (defined($self->transaction) && defined($self->transaction->rule)
       or defined($self->parent->transaction) && defined($self->parent->transaction->rule)) {
      croak("Manipulating the order of multiple subobjects within a production rule is not allowed");
   }
}

sub set_as_default_now {
   my ($self)=@_;
   &set_as_default_sanity_checks;
   select_multi_instance($self, $Scope);
}

sub set_as_default {
   my ($self)=@_;
   &set_as_default_sanity_checks;
   my $pv=$self->parent->contents->[$self->parent->dictionary->{$self->property->key}];
   if (my $index=list_index($pv->values, $self)) {
      croak("Internal error: selected subobject is not attached to its parent");
      my $need_commit= !defined($self->transaction) && begin_transaction($self);
      splice @{$pv->values}, $index, 1;
      unshift @{$pv->values}, $self;
      $self->transaction->changed=1;
      $self->transaction->commit($self) if $need_commit;
   }
}
####################################################################################
# private:
sub create_undefs {
   my $self=shift;
   my $trans=begin_transaction($self);
   foreach my $out (@_) {
      my ($obj, $prop)=descend_and_create($self, $out);
      push @{$obj->contents}, new PropertyValue($prop);
      $obj->dictionary->{$prop->key}=$#{$obj->contents};
   }
   $trans->changed=1;
   $trans->commit($self);
}
####################################################################################
# [ Property ], for_planning_flag => PropertyValue || 0 (does not exist) || undef
# $for_planning = TRUE forbids to apply shortcuts
sub lookup_descending_path {
   my ($self, $path, $for_planning)=@_;
   my (@existing_path, $content_index, $pv, $obj, $prop);
   if (($obj, $prop) = descend_partially($self, \@existing_path, $path)) {
      if (defined ($content_index = $obj->dictionary->{$prop->key}) &&
          defined ($pv = $obj->contents->[$content_index])) {
         if ($Application::plausibility_checks && !$for_planning   # nested scheduler runs should still see this as `undef'
               and
             !defined($pv->value) && defined($obj->transaction)
               and
             defined($obj->transaction->rule)
               and
             $content_index > $obj->transaction->content_end
             || exists $obj->transaction->backup->{$content_index}) {
            die "rule '", $obj->transaction->rule->header, "' attempts to read its output property ", $prop->name,
            " before it is created\n";
         }
         return ($obj, $pv);
      }
   } else {
      # encountered an undef during descent?
      defined($self) or return;
   }

   # shortcuts are not applied when a rule chain is composed or a production or initial rule is being executed
   return 0 if $for_planning or defined($self->transaction) && defined($self->transaction->rule);

   # try to find a shortcut
   my ($rule, $rc);
   if (@$path == 1) {
      $existing_path[0] = $self;
   }

   # iterate for all missing properties in the request path, starting with the last one
   for (my $depth = $#$path; $depth >= $#existing_path; --$depth) {
      my $top = $self;
      $prop = $path->[$depth];
      my $prod_key = $prop->key;

      # ascend in the object hierarchy until a suitable shortcut is found
      for (my $cur_depth = $depth; ;) {
         # only look for shortcuts rooted in existing subobjects
         if ($cur_depth <= $#existing_path) {
            my $cur_obj = $cur_depth >= 0 ? $existing_path[$cur_depth] : $top;
            my $shortcuts = $cur_obj->type->get_shortcuts_for($prop);
            if ($depth == $#$path) {
               # consider all shortcuts delivering the requested property directly
               # but only those without preconditions as we do not want to check these here!
               foreach $rule (@$shortcuts) {
                  if (@{$rule->preconditions} == 0
                      and ($rc, $pv) = $rule->execute($cur_obj)
                      and $rc == Rule::Exec::OK) {
                     return ($cur_obj, $pv);
                  }
               }
            } else {
               # consider shortcuts for subobjects on the path to the requested property, look there for the rest of the path
               # but only those without preconditions as we do not want to check these here!
               foreach $rule (@$shortcuts) {
                  if (@{$rule->preconditions} == 0
                      and ($rc, $pv) = $rule->execute($cur_obj)
                      and $rc == Rule::Exec::OK) {
                     local splice @$path, 0, $depth + 1;
                     if ((($cur_obj, $pv) = lookup_descending_path($pv->value, $path)) == 2) {
                        return ($cur_obj, $pv);
                     }
                  }
               }
            }
         }
         if (--$cur_depth >= 0) {
            $prop = $path->[$cur_depth];
         } elsif (defined $top->parent) {
            $prop = $top->property;
            $top = $top->parent;
         } else {
            last;  # topmost object reached
         }
         if (defined ($prod_key = $prod_key->{$prop->key})) {
            $prop = $prod_key;
         } else {
            last;  # no further production rules or shortcuts for $prop above this level
         }
      }
   }
   0
}

sub lookup_request {
   my ($self, $req, $for_planning)=@_;
   my $pv;
   foreach my $path (@$req) {
      $pv=lookup_descending_path($self, $path, $for_planning)
        and last;
   }
   $pv
}

sub lookup_pv {         # 'request' => PropertyValue or Object or 0 or undef
   my ($self, $req_string)=@_;
   lookup_request($self, $self->type->encode_read_request($req_string));
}

sub lookup {
   if (my $pv=&lookup_pv) {
      if (@_>2) {
         $pv->property->flags & Property::Flags::is_multiple
           or croak( "property-based lookup can only be performed on sub-objects with `multiple' attribute" );
         $pv->find(@_[2..$#_]);
      } else {
         $pv->value;
      }
   } else {
      _expect_array_access() ? [ ] : undef;
   }
}
sub lookup_with_name {
   if (my $pv=&lookup_pv) {
      ($pv->value, $pv->property->name)
   } else {
      ()
   }
}
####################################################################################
sub lookup_property_path {
   my ($self, $prop_path)=@_;
   my $up=$prop_path->up;
   while ($up-- > 0) {
      $self=$self->parent or return;
   }
   lookup_descending_path($self, $prop_path->down);
}

sub value_at_property_path {
   my $pv=&lookup_property_path;
   $pv ? $pv->value : croak("property path ", $prop_path->toString, " does not lead to a defined value");
}
####################################################################################
# private:
sub _put_pv {
   my ($self, $prop, $pv)=@_;
   my $content_index=$self->dictionary->{$prop->key};
   if (defined $self->transaction->rule) {
      # creating a property in a rule
      defined($content_index)
         or croak( "Attempt to create property '", $prop->name, "' which is not declared as a rule target" );
      if ($prop->flags & Property::Flags::is_multiple) {
         # storing the entire instance at once
         $self->contents->[$content_index]->values->[0]=$pv;
      } else {
         $self->contents->[$content_index]=$pv;
      }

   } elsif (defined $content_index) {
      # overwriting an existing property
      if ($prop->flags & Property::Flags::is_multiple) {
         croak( "There is already an instance of a multiple subobject ", $prop->name, "; use method add() for additional instances" );
      } elsif ($self->transaction->content_end<0 || $prop->flags & Property::Flags::is_mutable) {
         $self->contents->[$content_index]=$pv;
      } else {
         croak( "May not change the property ", $prop->name );
      }

   } else {
      # new property created outside of a production rule
      if ($prop->flags & Property::Flags::is_multiple) {
         $pv=new PropertyValue::Multiple($pv->property, [ $pv ]);
      }
      push @{$self->contents}, $pv;
      $self->dictionary->{$prop->key}=$content_index=$#{$self->contents};
   }
}
####################################################################################
sub put {
   my ($self, $temp, $prop, $value);
   if (@_==3) {
      ($self, $prop, $value)=@_;
   } else {
      ($self, $temp, $prop, $value)=@_;
   }
   my $need_commit;
   if (defined $self->transaction) {
      $temp ||= $self->transaction->entirely_temp;
   } else {
      if ($prop->flags & (Property::Flags::is_multiple | Property::Flags::is_mutable)) {
         $need_commit=1;
         begin_transaction($self);
      } else {
         croak( "can't add or change the property ", $prop->name );
      }
   }
   my $pv=$prop->accept->($value, $self, 0, $temp);
   _put_pv($self, $prop, $pv);
   if ($temp) {
      assign_max($#{$self->transaction->temporaries}, 0);
      push @{$self->transaction->temporaries}, ($prop->flags & Property::Flags::is_multiple ? [ $prop, $pv->values->[0] ] : $prop);
   } elsif (not $prop->flags & Property::Flags::is_non_storable) {
      $self->transaction->changed=1;
   }
   $self->transaction->commit($self) if $need_commit;
   $pv->value if defined(wantarray);
}
####################################################################################
sub put_ref {
   my ($self, $prop, $value, $flags)=@_;
   _put_pv($self, $prop, new PropertyValue($prop, $value, $flags || (defined($value) && PropertyValue::Flags::is_strong_ref)));
}
####################################################################################
sub add {
   my ($self, $prop_name)=splice @_, 0, 2;
   my $need_commit;
   my $prop=$self->type->property($prop_name);
   if ($prop->flags & Property::Flags::is_multiple) {
      my $temp= is_integer($_[0]) && $_[0] == PropertyValue::Flags::is_temporary && shift;
      my $value = is_object($_[0]) ? shift : new_named($prop->type, @_%2 ? shift : undef);

      if (defined($self->transaction)) {
         if (!$temp) {
            if (defined($self->transaction->rule)) {
               croak( "Attempt to add a subobject in an unrelated rule" );
            }
            $temp=$self->transaction->entirely_temp;
         }
      } else {
         $need_commit=@_>0;
         begin_transaction($self);
      }

      my $pv=_add_multi($self, $prop, $value, $temp);
      $value=$pv->values->[-1];    # might be replaced by a copy
      $pv->ensure_unique_name($value, $temp);
      if (@_) {
         eval { fill_properties($value, $value->type, @_) };
         if ($@) {
            $self->rollback;
            die $@;
         }
      }
      if ($temp) {
         assign_max($#{$self->transaction->temporaries}, 0);
         push @{$self->transaction->temporaries}, [ $prop, $value ];
      } else {
         $self->transaction->changed=1;
      }
      $self->transaction->commit($self) if $need_commit;
      $value
   } else {
      croak( "only subobjects declared as 'multiple' can be added" );
   }
}
####################################################################################
sub take {
   my ($self, $prop_name, $value, $temp)=@_;
   my @req=$self->type->encode_descending_path($prop_name);
   my $need_commit;
   if (!defined($self->transaction)) {
      if ($req[-1]->flags & (Property::Flags::is_multiple | Property::Flags::is_mutable)) {
         $need_commit = 1;
         begin_transaction($self);
      } else {
         croak( "can't add or change the property $prop_name" );
      }
   }
   my ($obj, $prop)=descend_and_create($self, \@req);
   put($obj, $temp, $prop, $value);
   $self->transaction->commit($self) if $need_commit;
}
####################################################################################
sub add_twin_backref {
   my ($self, $prop, $parent)=@_;
   push @{$self->contents},
        defined($parent) && $parent != $self->parent
        ? new PropertyValue($prop, $parent, PropertyValue::Flags::is_weak_ref)
        : new PropertyValue::BackRefToParent($self);
   $self->dictionary->{$prop->key}=$#{$self->contents};
}
####################################################################################
sub forget_parent_property {
   my ($self)=@_;
   my $prop=$self->property;
   undef $self->property;
   if ($prop->flags & (Property::Flags::is_augmented | Property::Flags::is_twin)) {
      begin_transaction($self);
      if ($prop->flags & Property::Flags::is_twin) {
         my $content_index=delete $self->dictionary->{$prop->key};
         defined($content_index)
           or die "internal error: missing back-reference for twin property ", $prop->name, "\n";
         undef $self->contents->[$content_index];
         assign_min($self->transaction->temporaries->[0], $content_index);
      }
      if ($prop->flags & Property::Flags::is_augmented) {
         perform_cast($self, $prop->type->pure_type);
      }
      $self->transaction->changed=1;
      $self->transaction->commit($self);
   }
}
####################################################################################
sub remove {
   my ($need_commit, @adjust_subobjects);
   if (@_==1) {
      my ($sub_obj)=@_;
      my $self=$sub_obj->parent;
      # some sanity checks up front
      defined($self) && $sub_obj->property->flags & Property::Flags::is_multiple
        or croak( "only multiple sub-objects can be removed by reference" );

      if (defined($self->transaction)) {
         if (defined($self->transaction->rule)) {
            croak( "attempt to remove a sub-object in a rule" );
         }
      } else {
         $need_commit=1;
         begin_transaction($self);
      }
      my $content_index=$self->dictionary->{$sub_obj->property->key};
      my $pv;
      if (defined($content_index)  and
          defined($pv=$self->contents->[$content_index])) {
         if (defined($sub_obj->transaction) && !$need_commit) {
            $sub_obj->transaction->rollback($sub_obj);
            delete $self->transaction->subobjects->{$sub_obj};
         }
         $self->transaction->backup->{$content_index} //= $pv->backup;
         $pv->remove_instance($sub_obj);
         if (!@{$pv->values}) {
            delete $self->dictionary->{$sub_obj->property->key};
            undef $self->contents->[$content_index];
            assign_min($self->transaction->temporaries->[0], $content_index);
         }
         push @adjust_subobjects, $sub_obj;
      } else {
         $self->rollback;
         croak( "internal inconsistency: parent object has lost the property ", $sub_obj->property->name );
      }
      $self->transaction->changed=1;
      $self->transaction->commit($self) if $need_commit;

   } else {
      my $self=shift;
      my $from_production_rule=defined($self->transaction) && defined($self->transaction->rule);
      my @to_remove=map {
         my @req=$self->type->encode_descending_path($_);
         if ($Application::plausibility_checks && $from_production_rule) {
            $self->transaction->rule->has_matching_input(\@req)
              or croak( "a production rule can only remove its own source properties" );
         }
         \@req
      } @_;

      if (!defined($self->transaction)) {
         $need_commit=1;
         begin_transaction($self);
      }
      my (@reconstruct_request, $obj, $prop);
      unless ($from_production_rule) {
         # Scheduler must know the truth even when some properties are removed
         has_sensitive_to($self, $_) for $self->type->list_permutations;
      }
      foreach my $req (@to_remove) {
         if (($obj, $prop)=descend_with_transaction($self, $req) and
             defined (my $content_index=delete $obj->dictionary->{$prop->key})) {

            my $pv=$obj->contents->[$content_index];
            if (defined($pv)) {
               unless ($from_production_rule or $prop->flags & (Property::Flags::is_multiple | Property::Flags::is_mutable)) {
                  if (($pv->property->flags & (Property::Flags::is_subobject | Property::Flags::is_subobject_array | Property::Flags::is_produced_as_whole)) == Property::Flags::is_subobject) {
                     push @reconstruct_request, enumerate_atomic_properties($pv, $req);
                  } else {
                     push @reconstruct_request, [ $req ];
                  }
               }
               if ($pv->property->flags & Property::Flags::is_subobject) {
                  if ($pv->property->flags & Property::Flags::is_multiple) {
                     push @adjust_subobjects, grep { refcnt($_)>1 } @{$pv->values};
                  } elsif ($pv->property->flags & Property::Flags::is_twin &&
                           instanceof PropertyValue($pv) &&
                           $pv->flags & PropertyValue::Flags::is_weak_ref) {
                     $obj->dictionary->{$prop->key}=$content_index;
                     $self->rollback if $need_commit;
                     croak( "back-reference of a twin property ", $prop->name, " cannot be removed separately" );
                  } elsif (refcnt($pv)>2) {
                     # refcnt==2 when the subobject is only stored in $obj->contents and in $pv
                     push @adjust_subobjects, $pv;
                  }
               }
            }
            $obj->transaction->backup->{$content_index} //= $pv;
            undef $obj->contents->[$content_index];
            assign_min($obj->transaction->temporaries->[0], $content_index);
            $obj->transaction->changed=1 unless $prop->flags & Property::Flags::is_non_storable;
         } else {
            $self->rollback if $need_commit;
            croak( "property ", Property::print_path($req), " does not exist in the given object" );
         }
      }
      if (@reconstruct_request) {
         # create a nested transaction isolating all possible Scheduler feats
         $need_commit or $self->begin_transaction;
         $self->transaction->entirely_temp=1;
         my $sched=new Scheduler::InitRuleChain($self, create Rule('request', \@reconstruct_request, 1), 1);
         my $allow= $sched->gather_rules && defined($sched->resolve);
         $need_commit or $self->transaction->commit($self);
         unless ($allow) {
            $self->rollback;
            croak( "can't remove the ",
                   @reconstruct_request>1 ? "properties " : "property ",
                   join(", ", map { Property::print_path($_) } @reconstruct_request),
                   ": neither multiple, nor mutable, nor unambiguously reconstructible from the remaining properties" );
         }
      }
      $self->transaction->commit($self) if $need_commit;
   }

   foreach my $sub_obj (@adjust_subobjects) {
      # automatically triggers forget_parent_property($sub_obj)
      undef $sub_obj->parent;
      undef $sub_obj->is_temporary;
   }
}
####################################################################################
sub commit {
   my ($self)=@_;
   while ($self->transaction->dependent) {
      $self=$self->parent;
   }
   $self->transaction->commit($self);
}

sub rollback {
   my ($self)=@_;
   while ($self->transaction->dependent) {
      $self=$self->parent;
   }
   $self->transaction->rollback($self);
}

# private:
sub delete_from_subobjects {
   my ($self, $trans)=@_;
   delete $trans->subobjects->{$self};
}
####################################################################################
sub cleanup {
   my ($self, $data)=@_;
   my $deleted_index=shift @$data;
   foreach (@$data) {
      my $content_index;
      if (!is_object($_)) {
         defined($content_index=$self->dictionary->{$_->[0]->key}) or next;
         my $pv=$self->contents->[$content_index];
         if (is_object(my $sub_obj=$_->[1])) {
            # via multiple sub-object (might be removed in the meanwhile)
            if ($#$_==2) {
               # only single properties are temporary
               cleanup($sub_obj, $_->[2]);
               next if $#{$sub_obj->contents}>=0;
            }
            $pv->remove_instance($sub_obj) if defined($sub_obj->parent);
            next if @{$pv->values};
         } else {
            # via singular sub-object: $pv==subobject, $sub_obj==list of properties
            cleanup($pv, $sub_obj);
            next if $#{$pv->contents}>=0;
         }
         delete $self->dictionary->{$_->[0]->key};

      } else {
         # atomic property
         defined($content_index=delete $self->dictionary->{$_->key}) or next;
      }
      if ($content_index==$#{$self->contents}) {
         pop @{$self->contents};
      } else {
         undef $self->contents->[$content_index];
         assign_min($deleted_index, $content_index);
      }
   }

   if (defined $deleted_index) {
      my ($gap, $last)=(1, $#{$self->contents});
      while (++$deleted_index<=$last) {
         my $pv=$self->contents->[$deleted_index];
         if (defined($pv)) {
            $self->contents->[$deleted_index-$gap]=$pv;
            $self->dictionary->{$pv->property->key}=$deleted_index-$gap;
         } else {
            ++$gap;
         }
      }
      $#{$self->contents}-=$gap;
   }

   $self->has_cleanup=0;
}

sub cleanup_now {
   my ($self)=@_;
   cleanup($self, delete $Scope->cleanup->{$self});
}
####################################################################################
sub save : method {
   my $self=shift;
   if (defined($self->property)) {
      croak( "A sub-object can't be saved without its parent" );
   }
   if (defined($self->transaction)) {
      if (defined($self->transaction->rule)) {
         croak( "Object::save called from within a rule" );
      }
      $self->transaction->commit($self);
   }
   cleanup_now($self) if $self->has_cleanup;
   $self->persistent->save($self);
   delete $save_at_end{$self};
   $self->changed=0;
}
####################################################################################
sub DESTROY {
   my $self=shift;
   if (defined($self->parent)) {
      if (defined($self->transaction) && defined (my $parent_trans=$self->parent->transaction)) {
         delete $parent_trans->subobjects->{$self};
      }

      # If $Scope object does not exist, we are in the final object destruction phase;
      # the persistent objects have already been saved in the AtEnd action,
      # all others may be discarded without remorse.
   } elsif (defined($Scope) && !defined($self->property)) {
      if (defined($self->persistent) && $self->changed) {
         if (defined($self->transaction)) {
            $self->transaction->rollback($self);
         }
         if ($self->changed) {
            if ($self->has_cleanup) {
               cleanup($self, delete $Scope->cleanup->{$self});
            }
            $self->persistent->save($self);
         } elsif ($self->has_cleanup) {
            delete $Scope->cleanup->{$self};
         }
         delete $save_at_end{$self};
      } elsif ($self->has_cleanup) {
         delete $Scope->cleanup->{$self};
      }
   }
}

sub dont_save {
   my ($self)=@_;
   if (!defined($self->property) && defined($Scope) && defined($self->persistent)) {
      if (defined($self->transaction)) {
         $self->transaction->rollback($self);
      }
      if ($self->changed) {
         if ($self->has_cleanup) {
            cleanup($self, delete $Scope->cleanup->{$self});
         }
         $self->changed=0;
         delete $save_at_end{$self};
      }
   }
   $self
}
####################################################################################
sub allow_undef_value {
   my ($self, $prop)=@_;
   if (defined($self->transaction) && defined($self->transaction->rule) &&
       !($self->transaction->rule->flags & (Rule::Flags::is_definedness_check | Rule::Flags::is_initial))) {
      die "attempt to use a property ", is_object($prop) ? $prop->name : $prop, " with undefined value\n";
   }
   undef
}

sub give_pv {
   my ($self, $req_string)=@_;
   my $req=$self->type->encode_read_request($req_string);
   my $pv;
   $pv=lookup_request($self, $req)
     or
   defined($pv)
   ? do {
        if (defined($self->transaction)) {
           if (defined($self->transaction->rule)) {
              die "attempt to access a non-existing property $req_string\n";
           }
           $self->commit;
        }
        provide_request($self, [ $req ]);
        $pv=lookup_request($self, $req)
          or
        defined($pv) ? die( "property $req_string not created as expected" ) : undef;
     }
   : undef;
}

sub give {
   if (@_==2) {
      if (my $pv=&give_pv) {
         $pv->value;
      } elsif (defined($_[0]->transaction) && defined($_[0]->transaction->rule)) {
         die "attempt to access an undefined property $_[1]\n";
      } else {
         undef
      }
   } else {
      my ($self, $req_string)=splice @_, 0, 2;
      my $req=$self->type->encode_read_request($req_string);
      if (@$req > 1) {
         croak( "mixing alternatives with property-based lookup of multiple sub-objects is not allowed" );
      }
      if (my ($obj, $prop)=descend($self, $req->[0])) {
         $prop->flags & Property::Flags::is_multiple
           or croak( "property-based lookup can only be performed on sub-objects with `multiple' attribute" );
         get_multi($obj, @_, $prop);
      } else {
         die "can't create any subobject under a non-existing path $req_string\n";
      }
   }
}

sub give_with_name {
   if (my $pv=&give_pv) {
      ($pv->value, $pv->property->name)
   } else {
      ()
   }
}

# "REQUEST, ..." => (value, ...)
sub give_list {
   my ($self, $req_string)=@_;
   my @requests=map { $self->type->encode_read_request($_) } split /\s*,\s*/, $req_string;
   if (provide_request($self, \@requests)) {
      map { $self->lookup_request($_)->value } @requests
   } else {
      croak( "can't provide $req_string" );
   }
}
####################################################################################
# protected:
sub provide_request {
   my ($self, $request)=@_;
   my ($success, $sched)=&Scheduler::resolve_request;
   return 1 if $success>0;
   my ($obj, $prop, $content_index, @lacking, @undefs);

   foreach my $input (@$request) {
      my $found;
      my @undefs_here;
      foreach my $path (@$input) {
         my @existing_path;
         if (($obj, $prop)=descend_partially($self, \@existing_path, $path)
               and
             defined($content_index=$obj->dictionary->{$prop->key})
               and
             defined($obj->contents->[$content_index])) {
            $found=1; last;
         } elsif (not $path->[scalar @existing_path]->flags & Property::Flags::is_multiple) {
            push @undefs_here, $path;
         }
      }
      unless ($found) {
         push @lacking, $input;
         push @undefs, @undefs_here;
      }
   }

   my $lacking=join(", ", map { "'" . Property::print_path($_) . "'" } @lacking);
   if ($success==0) {
      if ($Verbose::rules) {
         my @failed_precond;
         while (my ($rule, $rc) = each %{$sched->run}) {
            if ($rc == Rule::Exec::failed && $rule->flags & Rule::Flags::is_precondition) {
               push @failed_precond, $rule->header."\n";
            }
         }
         if (@failed_precond) {
            warn_print("could not compute $lacking probably because of unsatisfied preconditions:\n", @failed_precond);
         } else {
            warn_print("available properties insufficient to compute $lacking");
         }
      }
      create_undefs($self, @undefs);
      0;
   } else {
      die "no more rules available to compute $lacking\n";
   }
}
####################################################################################
sub provide {
   my $self=shift;
   if (defined($self->transaction)) {
      if (defined($self->transaction->rule)) {
         croak( "Object->provide not allowed within a rule" );
      } else {
         commit($self);
      }
   }
   provide_request($self, [ map { $self->type->encode_read_request($_) } @_ ]);
}
####################################################################################
sub get_schedule {
   my $self=shift;
   if (defined($self->transaction)) {
      if (defined($self->transaction->rule)) {
         croak( "Object->get_schedule not allowed within a rule" );
      } else {
         commit($self);
      }
   }
   Scheduler::get_chain($self, [ map { $self->type->encode_read_request($_) } @_ ]);
}
####################################################################################
sub disable_rules {
   my $self=shift;
   my @rules=$self->type->find_rules_by_pattern(@_)
     or die "no matching rules found\n";
   foreach my $rule (@rules) {
      foreach my $prod ($rule, $rule->with_permutation ? ($rule->with_permutation, @{$rule->with_permutation->actions}) : ()) {
         local with($Scope->locals) {
            local $self->failed_rules->{$prod}=1;
         }
      }
   }
}
####################################################################################
sub apply_rule {
   my $self=shift;
   my @rules=$self->type->find_rules_by_pattern(@_)
     or die "no matching rules found\n";
   Scheduler::resolve_rules($self, \@rules) > 0
     or croak( "can't apply ", @rules>1 ? "any of requested rules" : "the requested rule",
               " due to insufficient source data and/or unsatisfied preconditions" );
}
####################################################################################
sub list_names {
   my $self=shift;
   if (defined(my $prop=$self->property)) {
      if ($prop->flags & Property::Flags::is_multiple) {
         my $pv=$self->parent->contents->[$self->parent->dictionary->{$prop->key}];
         return map { $_->name } @{$pv->values};
      }
   }
   $self->name
}
####################################################################################
sub list_properties {
   my ($self, $rec)=@_;
   map {
      my $prop=$_->property;
      my $prop_name=$prop->name;
      if ($rec) {
         if ($prop->flags & Property::Flags::is_multiple) {
            map { my $prefix="$prop_name\[".$_->name."]"; map { "$prefix.$_" } list_properties($_, $rec) } @{$_->values}
         } elsif ($prop->flags & Property::Flags::is_subobject and !instanceof PropertyValue::BackRefToParent($_) ) {
            map { "$prop_name.$_" } list_properties($_, $rec)
         } else {
            $prop_name
         }
      } else {
         $prop_name
      }
   } grep { defined } @{$self->contents};
}
####################################################################################
sub enumerate_atomic_properties {
   my ($self, $descent_path)=@_;
   map {
      if (defined($_) && !($_->property->flags & (Property::Flags::is_multiple | Property::Flags::is_mutable))) {
         if (instanceof PropertyValue($_)) {
            [ [ @$descent_path, $_->property ] ]
         } else {
            local push @$descent_path, $_->property;
            enumerate_atomic_properties($_, $descent_path);
         }
      } else {
         ()
      }
   } @{$self->contents};
}
####################################################################################
sub properties {
   my ($self,%options)=@_;
   my $depth = $options{maxdepth}+0 // 0;
   my @data;
   push @data, "name: ".$self->name if $self->name;
   push @data, "type: ".$self->type->full_name;
   push @data, "description: ".$self->description if $self->description;
   $data[-1] .= "\n";
   foreach ($self->list_properties) {
      my $pv = $self->give_pv($_);
      if ($pv->property->flags & Property::Flags::is_subobject) {
         push @data, $_;
         if ($depth != 0 and !instanceof PropertyValue::BackRefToParent($pv)) {
            foreach my $value ($pv->property->flags & Property::Flags::is_multiple ? @{$pv->values} : ($pv->value)) {
               push @data, ">>",$value->properties(maxdepth=>$depth-1) =~ s/^/  /gmr, "<<\n";
               chomp $data[-2];
            }
         } else {
            push @data, "type: ".$pv->property->type->full_name;
            if (instanceof PropertyValue::BackRefToParent($pv)) {
               push @data, "-- backreference to parent object";
            }
            $data[-1] .= "\n";
         }
      } else {
         push @data, $_, $pv->property->type->toString->($pv->value)."\n";
      }
   }
   my $text = join "\n", @data;
   defined(wantarray()) ? $text : print $text;
}
####################################################################################
sub schedule {
    my ($self, $property)=@_;
    print map { "$_\n" } $self->get_schedule($property)->list;
}
####################################################################################
sub get {
   my ($self, $prop)=@_;
   my $content_index=$self->dictionary->{$prop->key};
   unless (defined $content_index) {
      if (defined($self->transaction)) {
         if (defined($self->transaction->rule)) {
            die "attempt to access a non-existing property ", $prop->name, "\n";
         }
         $self->commit;
      }

      if ($prop->flags & Property::Flags::is_subobject
          and my @sub_prop_names=_get_descend_path()) {

         $prop=$prop->instance_for_owner($self->type, 1);
         my $sub_type=$prop->type;
         my @req_path=($prop);
         foreach (@sub_prop_names) {
            if (defined (my $sub_prop=$sub_type->lookup_property($_))) {
               push @req_path, $sub_prop;
               last unless $sub_prop->flags & Property::Flags::is_subobject;
               $sub_type=$sub_prop->type;
            } else {
               last;
            }
         }
         my @req=( \@req_path );
         provide_request($self, [ \@req ]);
      } else {
         provide_request($self, [ [ [ $prop ] ] ]);
      }
      defined($content_index=$self->dictionary->{$prop->key})
        or die "can't create property ", $prop->name, "\n";
   }
   $self->contents->[$content_index]->value // &allow_undef_value;
}
####################################################################################
sub get_multi {
   my $iterate;
   if (@_==2 && !($iterate=_expect_array_access())) {
      &get;
   } else {
      my $self=shift;
      my $prop=pop;
      my $content_index=$self->dictionary->{$prop->key};
      if ($iterate) {
         return defined($content_index) ? $self->contents->[$content_index]->values : [ ];
      }
      if (@_==1) {
         my $arg_ref=ref($_[0]);
         if ($arg_ref eq "CODE") {
            # filter by arbitrary expression
            if (defined($content_index)) {
               foreach (@{$self->contents->[$content_index]->values}) {
                  return $_ if &{$_[0]};
               }
            }
            return undef;
         }
         if ($arg_ref eq "") {
            # search by name
            return defined($content_index) ? $self->contents->[$content_index]->find_by_name(@_) : undef;
         }
      }
      if (defined $content_index) {
         $self->contents->[$content_index]->find_or_create($self, @_);
      } else {
         (new PropertyValue::Multiple($prop->instance_for_owner($self->type, 1), [ ]))->find_or_create($self, @_);
      }
   }
}
####################################################################################
sub put_multi {
   if (@_==3 or @_==4 && !ref($_[1])) {
      &put;
   } else {
      my $self=shift;
      my ($prop, $value)=splice @_, -2;
      my $need_commit;
      if (!defined($self->transaction)) {
         $need_commit=1;
         begin_transaction($self);
      }
      my $content_index=$self->dictionary->{$prop->key};
      if (defined $content_index) {
         $self->contents->[$content_index]->replace_or_add($self, @_, $value);
      } else {
         $value = $prop->accept->($value, $self);
         push @{$self->contents}, new PropertyValue::Multiple($value->property, [ $value ]);
         $self->dictionary->{$prop->key}=$#{$self->contents};
      }
      $self->transaction->changed=1;
      $self->transaction->commit($self) if $need_commit;
      $value;
   }
}
####################################################################################
sub get_attachment {
   my ($self, $name)=@_;
   if (defined($self->transaction) && defined($self->transaction->rule)) {
      croak( "production rules may not access attachments" );
   }
   my $at=$self->attachments->{$name};
   $at && $at->[0]
}

sub remove_attachment {
   my ($self, $name)=@_;
   if (defined($self->transaction)) {
      if (defined($self->transaction->rule)) {
         croak( "porduction rules may not operate on attachments" );
      }
      $self->transaction->changed=1;
   } else {
      begin_transaction($self);
      $self->transaction->changed=1;
      $self->transaction->commit($self);
   }
   delete $self->attachments->{$name};
}

sub list_attachments {
   my $self=shift;
   keys %{$self->attachments};
}

sub attach {
   my ($self, $name, $data, $construct)=@_;
   if (ref($data)) {
      if (is_object($data) && defined (my $type=UNIVERSAL::can($data, "type"))) {
         $type=$type->();
         if (instanceof ObjectType($type)) {
            croak( "can't attach a subobject of type ", $type->full_name, ": only atomic properties are allowed as attachments" );
         }
         if ($construct) {
            $construct=trim_spaces($construct);
            # this will happily croak if there are unknown properties
            my @paths=map { $self->type->encode_property_path($_) } split /\s*,\s*/, $construct;
            if (defined($type->construct_node) and
                $type->construct_node->min_arg > @paths+1 || $type->construct_node->max_arg < @paths+1) {
               croak( "wrong number of additional constructor arguments specified for type ", $type->full_name );
            }
         } elsif (defined($type->construct_node) && !defined($type->construct_node->resolve([$type, [ ]]))) {
            # can't be deserialized without additional arguments
            croak( "type ", $type->full_name, " needs additional constructor arguments but no property paths are specified" );
         }
      } else {
         croak( "can't attach ", ref($data), ": it does not belong to any declared property type" );
      }
   }
   if (defined($self->transaction)) {
      if (defined($self->transaction->rule)) {
         croak( "production rules may not operate on attachments" );
      }
      $self->transaction->changed=1;
   } else {
      begin_transaction($self);
      $self->transaction->changed=1;
      $self->transaction->commit($self);
   }
   $self->attachments->{$name}=[ $data, $construct ];
}

sub copy_attachments {
   my ($self, $src)=@_;
   while (my ($name, $at)=each %{$src->attachments}) {
      my ($data, $construct)=@$at;
      if (is_object($data)) {
         $data=$data->type->construct->($construct ? $self->give_list($construct) : (), $data);
      }
      $self->attachments->{$name}=[ $data, $construct ];
   }
}
####################################################################################
# this method is called only as fallback,
# if no ObjectType in the type hierarchy has a specific suffix
sub default_file_suffix : method {
   $_[0]->type->application->default_file_suffix
}
####################################################################################
sub add_credit {
   my ($self, $credit)=@_;
   my $parent;
   while (defined ($parent=$self->parent)) { $self=$parent; }
   $self->credits->{$credit->product} ||= $credit;
   $credit->display if $Verbose::credits > $credit->shown;
}
####################################################################################
sub copy_permuted {
   my $self=shift;
   my $proto=$self->type->pure_type;
   my @perms;
   if ($_[0] eq "permutation") {
      my $perm_name=splice @_, 0, 2;
      if (ref($perm_name)) {
         @$perm_name or croak( "invalid option 'permutation => empty ARRAY'" );
         $perms[0]=$perm_name;
      } else {
         $perms[0]=[ $proto->encode_descending_path($perm_name, $self) ];
      }
      $perms[0]->[-1]->flags & Property::Flags::is_permutation
        or croak( "invalid option 'permutation => $perm_name' which is not a permutation" );
   } else {
      my @path=$proto->encode_descending_path($_[0]);
      @perms = map { bless [ $_ ], "Polymake::Core::Rule::PermutationPath" }
               grep { defined($_->find_sensitive_sub_property(@path)) }
                    $proto->list_permutations;
      unless (@perms) {
         my $subobj=$self;
         my ($content_index, $pv);
         for (my $i=0; $i<$#path; ++$i) {
            if (defined ($content_index=$subobj->dictionary->{$path[$i]->key}) &&
                defined ($pv=$subobj->contents->[$content_index])) {
               if (@perms = map { bless [ @path[0..$i], $_ ], "Polymake::Core::Rule::PermutationPath" }
                            grep { defined($_->find_sensitive_sub_property(@path[$i+1..$#path])) }
                                 $pv->property->type->list_permutations) {
                  last;
               }
               $subobj=$pv->value;
            } else {
               croak( "copy_permuted: ", join(".", map { $->name } @path[0..$i]), " does not exist in the object" );
            }
         }
         @perms or croak( "copy_permuted: $_[0] is not affected by any permutation" );
      }
      for (my $i=2; $i<$#_; $i+=2) {
         @path=$proto->encode_descending_path($_[$i]);
         @perms= grep { defined($_->find_sensitive_sub_property(@path)) } @perms
           or croak( "copy_permuted: no permutation type known which would simultaneously affect ", join(", ", map { $_[$_*2] } 0..$i/2));
      }
      if (@perms!=1) {
         croak( "copy_permuted: permutation type can't be unambiguously chosen; permutations in question are ",
                join(", ", map { Property::print_path($_) } @perms) );
      }
   }

   my ($rule, $filter)=permuting Rule($proto, @perms);
   my $copy=new($proto->pkg, $self->name);
   begin_transaction($copy);
   copy_contents($copy, $self, defined($self->parent) && $self->property->flags & Property::Flags::is_augmented, $filter);
   $copy->description=$self->description;

   for (my $i=0; $i<$#_; $i+=2) {
      take($copy, $_[$i], $_[$i+1]);
   }
   Scheduler::resolve_permutation($copy, $rule) > 0
     or croak( "couldn't permute all properties" );
   cleanup_now($copy);
   $copy;
}
####################################################################################
# private:
sub search_for_sensitive {
   my ($self, $prop_hash, $in_props, $taboo)=@_;
   my ($content_index, $pv);
   while (my ($prop_key, $down)=each %$prop_hash) {
      if ($prop_key == $Permutation::sub_key) {
         while (my ($prop_key, $sub_hash)=each %$down) {
            if (defined ($content_index=$self->dictionary->{$prop_key}) &&
                defined ($pv=$self->contents->[$content_index]) &&
                defined ($pv->value)) {
               if ($pv->property->flags & Property::Flags::is_multiple) {
                  foreach (@{$pv->values}) {
                     if ($_ != $taboo && search_for_sensitive($_, $sub_hash, $in_props)) {
                        keys %$down; keys %$prop_hash;   # reset iterators
                        return 1;
                     }
                  }
               } elsif ($pv->value != $taboo && search_for_sensitive($pv->value, $sub_hash, $in_props)) {
                  keys %$down; keys %$prop_hash;   # reset iterators
                  return 1;
               }
            }
         }
      } else {
         if (defined ($content_index=$self->dictionary->{$prop_key}) &&
             defined ($pv=$self->contents->[$content_index]) &&
             defined ($pv->value)) {
            if ($in_props or $pv->value != $taboo && search_for_sensitive($pv->value, $down->sensitive_props,1)) {
               keys %$prop_hash;        # reset the iterator;
               return 1;
            }
         }
      }
   }
   0
}

sub search_for_sensitive_in_ancestors {
   my ($self, $permutation)=@_;
   my $hash=$permutation->parent_permutations;
   while ($self->parent) {
      $hash=$hash->{$self->property->key} or last;
      if (defined (my $list=$hash->{$Permutation::sub_key})) {
         foreach my $parent_perm_prop (@$list) {
            return 1 if has_sensitive_to($self->parent, $parent_perm_prop, $self);
         }
      }
      $self=$self->parent;
   }
   0
}

sub has_sensitive_to {
   my ($self, $permutation, $taboo)=@_;
   $self->sensitive_to->{$permutation->key} //=
      (search_for_sensitive($self, $permutation->sensitive_props, 1) ||
       search_for_sensitive($self, $permutation->sub_permutations, 0, $taboo) ||
       ($self->parent ? &search_for_sensitive_in_ancestors : 0));
}
####################################################################################
# PropertyValue1, PropertyValue2, [ ignore list ] => scalar context: >0 if not equal, undef if equal
#                                                 => list context:   (pv1, pv2) if not equal, () if equal
sub diff_properties {
   my ($self, $pv1, $pv2, $ignore, $ignore_shortcuts) = @_;
   if ($pv1->property->flags & Property::Flags::is_multiple) {
      my $v2 = instanceof PropertyValue::Multiple($pv2) ? $pv2->values : $pv2;
      if ((my $l = $#{$pv1->values}) == $#$v2) {
         my @options = (ignore_subproperties($ignore, $pv1->property->name), ignore_shortcuts => $ignore_shortcuts);
         if (wantarray) {
            map { my $o1 = $pv1->values->[$_]; map { $_->[2] //= $o1; $_ } $o1->diff($v2->[$_], @options) } 0..$l;
         } else {
            $pv1->values->[$_]->diff($v2->[$_], @options) and return 1 for 0..$l;
            ()
         }
      } else {
         !wantarray || [ $pv1, $pv2 ]
      }
   } elsif (instanceof PropertyValue::BackRefToParent($pv1) ||
            instanceof PropertyValue::BackRefToParent($pv2)) {
      ref($pv1) eq ref($pv2) ? () : !wantarray || [ $pv1, $pv2 ]
   } else {
      my $v2 = $pv2;
      if (instanceof PropertyValue($pv2)) {
         $v2 = $pv2->value;
      } elsif (defined($pv2) && !($pv1->property->flags & Property::Flags::is_subobject) &&
               !defined($v2 = eval { $pv1->property->accept->($pv2, $self, false, true)->value })) {   # !trusted, temp
         return !wantarray || [ $pv1, $pv2 ];
      }
      if (defined($pv1->value) && defined($v2)) {
         if ($pv1->property->flags & Property::Flags::is_subobject) {
            if (ref($pv1->value) eq ref($v2)) {
               my @options = (ignore_subproperties($ignore, $pv1->property->name), ignore_shortcuts => $ignore_shortcuts);
               if (wantarray) {
                  map { $_->[2] //= $pv1->value; $_ } $pv1->value->diff($v2, @options);
               } else {
                  $pv1->value->diff($v2, @options);
               }
            } else {
               !wantarray || [ $pv1, $pv2 ]
            }
         } else {
            (defined($pv1->property->equal) ? select_method($pv1->property->equal, $self, 1)
                                            : $pv1->property->type->equal)
            ->($pv1->value, $v2) ? () : !wantarray || [ $pv1, $pv2 ]
         }
      } else {
         defined($pv1->value) == (defined($v2)) ? () : !wantarray || [ $pv1, $pv2 ]
      }
   }
}

# private:
sub ignore_subproperties {
   my ($list, $prop_name)=@_;
   if ($list &&= [ map { $_ =~ /^$prop_name\./ ? $' : () } @$list ]  and  @$list) {
      (ignore => $list)
   } else {
      ()
   }
}

# Object1, Object2, options => scalar context: >0 if not equal, 0 if equal
#                           => list context:   [ pv1, pv2, subobj ], ... if not equal
sub diff {
   my ($self, $other, %options)=@_;
   my @diff;
   my $ignore=delete $options{ignore};
   my $ignore_shortcuts=delete $options{ignore_shortcuts};
   if (keys %options) {
      croak( "Object::diff - unrecognized option(s) ", join(", ", keys %options) );
   }

   my %ignore;
   if ($ignore) {
      @ignore{@$ignore}=@_;
   }

   my ($self_contents, $other_contents)=
      ( map { [ grep { defined($_)  and
                       !exists $ignore{$_->property->name}  and
                       !( $ignore_shortcuts && instanceof PropertyValue($_) && $_->flags & PropertyValue::Flags::is_ref )
                     } @{$_->contents} ]
            } $self, $other );

   if (@$self_contents != @$other_contents) {
      if (wantarray) {
         push @diff, map { [ undef, $_ ] } grep { defined($_) && !exists $self->dictionary->{$_->property->key} } @$other_contents;
      } else {
         return 1;
      }
   }

   foreach my $pv (@$self_contents) {
      my $other_index=$other->dictionary->{$pv->property->key};
      if (defined $other_index) {
         if (wantarray) {
            push @diff, diff_properties($self, $pv, $other->contents->[$other_index], $ignore, $ignore_shortcuts);
         } else {
            diff_properties($self, $pv, $other->contents->[$other_index], $ignore, $ignore_shortcuts) and return 1;
         }
      } elsif (wantarray) {
         push @diff, [ $pv, undef ];
      } else {
         return 1;
      }
   }

   @diff
}
####################################################################################
sub ensure_save_changes {
   my ($self) = @_;
   state $at_end = add AtEnd("Object",
      sub {
         foreach my $obj (keys %save_at_end) {
            if (defined $obj->transaction) {
               $obj->rollback;
            }
            if ($obj->has_cleanup) {
               err_print( "object $obj ", defined($obj->name) && "(".$obj->name.")",
                          ": pending cleanup at end" );
            } elsif (!$obj->changed) {
               err_print( "object $obj ", defined($obj->name) && "(".$obj->name.")",
                          ": registered for save at end without changes" );
            } else {
               $obj->persistent->save($obj);
               $obj->changed = false;
            }
         }
      });
   $save_at_end{$self} = undef;
}

####################################################################################
package Polymake::Core::Object::Replacement;

use Polymake::Struct (
   '@properties',
   '%attachments',
);

sub new {
   my $self=&_new;
   @{$self->properties}=@_;
   $self;
}

sub add {
   my ($self, $name, $subobj)=@_;
   undef $subobj->parent;
   undef $subobj->property;
   undef $subobj->transaction;
   push @{$self->properties}, $name, $subobj;
}

sub attach {
   my ($self, $name, $value)=@_;
   $self->attachments->{$name}=$value;
}

# compares known properties, ignores others
sub equal {
   my ($self, $object)=@_;
   my $proto=$object->type;
   my $pv;
   for (my ($i, $last)=(0, $#{$self->properties}); $i<$last; $i+=2) {
      is_object($pv=$object->lookup_request($proto->encode_read_request($self->properties->[$i]), 1))
        && !diff_properties($object, $pv, $self->properties->[$i+1])
        or return 0;
   }
   1
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
