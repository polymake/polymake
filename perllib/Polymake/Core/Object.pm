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

use strict;
use namespaces;

package Polymake::Core::Object;

use Polymake::Struct (
   [ new => ';$' ],
   [ '$name' => '#1', set_filter => \&name_filter ],  # object name, unique among multiple subobject collections
   '$description',                      # textual description
   '%credits',                          # credit records from producing rules
   '@contents',                         # PropertyValues in the chronological (file) order
   '%dictionary',                       # Property => index in @contents
   '%attachments',                      # "name" => data object
   [ '$transaction' => 'undef' ],       # Transaction
   '$changed',                          # Object changed since last load() or save() (only at top-level)
   '%failed_rules | sensitive_to',      # Rule => undef  rules should not be applied to the object any more
                                        # Property->key (permutation) => contains properties sensitive to this permutation
   [ '$persistent' => 'undef' ],        # some object with load() and save() methods (only for top-level objects)
   [ '$has_cleanup' => 'undef' ],       # has registered a cleanup handler
   [ '$parent' => 'undef' ],            # parent Object
   [ '$property' => 'undef' ],          # as what this is kept in the parent object
   [ '$is_temporary' => 'undef' ],      # subobject or its ancestor was temporarily added
   [ '$parent_index' => 'undef' ],      # index in parent's PropertyValue (if multiple)
);

sub value { shift }             # for compatibility for PropertyValue

use Polymake::Ext;
use Polymake::Enum 'Rule::exec';

my ($at_end, %save_at_end);

####################################################################################
package Polymake::Core::Transaction;

use Polymake::Struct (
   [ new => '$@' ],
   [ '$content_end' => '$#{ #1 ->contents}' ],  # length of Object->contents at the transaction start
   '%subobjects',                               # subobjects with active transactions
   '@temporaries',                              # properties and subobjects temporarily added during the transaction
   [ '$changed' => 'undef' ],                   # boolean:  object was changed during the transaction
   [ '$outer' => '#1 ->transaction' ],
   [ '$entirely_temp' => '$this->outer && $this->outer->entirely_temp' ],  # all changes are temporary by default
   [ '$dependent' => 'undef' ],
   '%backup',                                   # overwritten or deleted properties: content_index => PropertyValue
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
   if ($t1->[0]->flags & $Property::is_multiple) {
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
                          $object->property->flags & $Property::is_multiple
                          ? [ $object->property, $object, $self->temporaries ]
                          : [ $object->property, $self->temporaries ]);
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
            propagate_temporaries($self,$object,$parent_trans->temporaries);
            $parent_trans->changed ||= $self->changed;
         } else {
            $parent_trans=$parent_obj->begin_transaction;
            propagate_temporaries($self,$object,$parent_trans->temporaries);
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
            if (!defined($object->contents->[$index])) {
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
      if (defined($sub_obj->transaction)) {
         die "internal error: unexpected transaction within rule\n" if defined($sub_obj->transaction->rule);
         bless $sub_obj->transaction, $sub_trans_type;
      } else {
         $sub_obj->transaction=_new($sub_trans_type, $sub_obj);
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
   until (Scheduler::resolve_initial_request($object, [ [ $ObjectType::init_pseudo_prop ], requests_for_subobjects($self) ])) {
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
   my $self=shift;
   map { ( [ [ @_, $_->property, $ObjectType::init_pseudo_prop ] ],
           requests_for_subobjects($_->transaction, @_, $_->property) )
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
   unless ($self->rule->flags & $Rule::is_any_precondition) {
      # is a production rule
      foreach my $item (@{$self->rule->output}) {
         my ($obj, $prop)=Object::descend_and_create($top_object,$item);
         my $pv=new PropertyValue($prop);
         if (defined (my $content_index=$obj->dictionary->{$prop->key})) {
            if (defined (my $pv_old=$obj->contents->[$content_index])) {
               $obj->transaction->backup->{$content_index}=$pv_old;
            }
            $obj->contents->[$content_index]=$pv;
         } else {
            push @{$obj->contents}, $pv;
            $obj->dictionary->{$prop->key}=$#{$obj->contents};
            $self->changed=0;
         }
      }
   }
   $self;
}

sub descend {
   my ($self, $sub_obj)=@_;
   $self->subobjects->{$sub_obj} ||= do {
      if (defined($sub_obj->transaction) && defined($sub_obj->transaction->rule)) {
         die "internal error: unexpected nested transaction within rule\n";
      }
      $sub_obj->transaction=_new(__PACKAGE__, $sub_obj, $self->rule);
      $sub_obj->transaction->dependent=1;
   };
   $sub_obj->transaction
}

sub restore_backup {
   my ($self, $object)=@_;
   if (defined $self->outer) {
      while (my ($index, $pv)=each %{$self->backup}) {
         if ($index <= $self->outer->content_end) {
            $self->outer->backup->{$index} ||= $pv;
         }
      }
   } else {
      while (my ($index, $pv)=each %{$self->backup}) {
         $object->contents->[$index]=$pv;
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
   my $perm_object=$object->add_perm($self->rule->permutation);
   undef $perm_object->transaction;

   # transfer the sensitive properties created by prod_rule into the permutation subobjects
   foreach my $action (@{$self->rule->actions}) {
      my $output=$prod_rule->output->[$action->output];
      my ($base_obj, $sub_perm_obj, $prop);
      if ($action->depth) {
         # must create an additional permutation object in the sub-object
         ($base_obj)=$object->descend(@$output[0..$action->depth]);
         if (defined (my $base_content_index=$base_obj->dictionary->{$prod_rule})) {
            $sub_perm_obj=$base_obj->contents->[$base_content_index]->value;
         } else {
            $sub_perm_obj=$base_obj->add_perm($action->sub_permutation);
            undef $sub_perm_obj->transaction;
         }
         ($base_obj)=$base_obj->descend(@$output[$action->depth..$#$output]);
         ($sub_perm_obj, $prop)=$sub_perm_obj->descend_and_create(@$output[$action->depth..$#$output]);
      } else {
         # the property is transferred into the base permutation object
         ($base_obj)=$object->descend($output);
         ($sub_perm_obj, $prop)=$perm_object->descend_and_create($output);
      }
      my $base_content_index=$base_obj->dictionary->{$prop->key};
      push @{$sub_perm_obj->contents}, $base_obj->contents->[$base_content_index];
      $sub_perm_obj->dictionary->{$prop->key}=$#{$sub_perm_obj->contents};
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
      croak( "Can't convert ", UNIVERSAL::can($_[0],"type") ? $_[0]->type->full_name : ref($_[0]), " to ", $proto->full_name );
   }
   my $self=new($proto->pkg, @_%2 ? shift : name_of_ret_var());
   fill_properties_and_commit($self, $proto, @_);
}

sub new_with_auto_cast : method {
   my ($rule, $proto, $src)=@_;
   my $self=new($proto->pkg, $src->name);
   copy_contents($src, $self);
   do_downcast($self, $rule);
   $self->description=$src->description;
   copy_attachments($self, $src);
   $self;
}

sub new_filled_with_auto_cast : method {
   my ($rule, $proto, $src)=splice @_, 0, 3;
   my $self=new_with_auto_cast($rule, $proto, $src);
   croak( "odd PROPERTY => VALUE list" ) if @_%2;
   fill_properties_and_commit($self, $proto, @_);
}

sub new_copy : method {
   my ($proto, $src, $new_parent)=@_;
   my $self=new($proto->pkg, $src->name);
   weak($self->parent=$new_parent) if defined($new_parent);
   if ($proto->isa($src->type)) {
      # a direct heir
      copy_contents($src, $self);
   } else {
      my $prop;
      foreach my $pv (@{stable_contents($src)}) {
         if (defined($pv) && !$pv->is_temporary && $proto->isa(($prop=$pv->property->declared)->belongs_to)) {
            $prop->upcast($proto);
            if ($prop->flags & $Property::is_multiple) {
               defined($_) && !$_->is_temporary && _add_multi($self, $prop, $_) for @{$pv->values};
            } else {
               _add($self, $prop, $pv->value);
            }
         }
      }
   }
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
      my ($obj, $prop)= $name =~ /\./ ? descend_and_create($self, $proto->encode_request_element($name, $self))
                                      : ($self, $proto->property($name));
      if ($prop->flags & $Property::is_multiple) {
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
   my $pv;
   if (defined (my $content_index=$self->dictionary->{$prop->key})) {
      $pv=$self->contents->[$content_index];
      $prop->accept->($value, $self, 0, $temp);
      $value->parent_index=@{$pv->values};
      push @{$pv->values}, $value;
   } else {
      $pv=$prop->accept->($value, $self, 0, $temp);
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
   push @{$self->contents}, new PropertyMultipleValue($prop->concrete($self), $values);
   $self->dictionary->{$prop->key}=$#{$self->contents};
   $prop->accept->($_, $self, $trusted) for @$values;
}
####################################################################################
# protected:
sub add_perm {
   my ($self, $permutation)=@_;
   my $perm_object=$permutation->create_subobject($self, $PropertyValue::is_temporary);
   if (defined (my $content_index=$self->dictionary->{$permutation->key})) {
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
   my ($self, $up, $permutation)=@_;
   while (--$up >= 0) {
      $self=$self->parent or return;
   }
   add_perm($self, $permutation);
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
   my ($self, $new_parent, $filter)=@_;
   if (@_==1) {
      if ($self->parent && $self->property->flags & $Property::is_locally_derived) {
         # strip off local extensions
         return $self->type->pure_type->construct->($self);
      }
   }
   my $copy=new($self, $self->name);
   $copy->description=$self->description;
   if (defined $new_parent) {
      weak($copy->parent=$new_parent);
      $copy->property=$self->property;
      $new_parent->transaction->descend($copy) if defined($filter);
      if ($self->property->flags & $Property::is_locally_derived) {
         update_locally_derived($copy, $new_parent);
      }
   } elsif (defined $filter) {
      begin_transaction($copy);
   }
   copy_contents($self, $copy, $filter);
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
   my ($self, $dst, $filter)=@_;
   foreach my $pv (@{stable_contents($self)}) {
      if (defined($pv) && !$pv->is_temporary) {
         my ($subobj, $pv_copy)= defined($filter) ? $filter->($dst, $pv) : ($dst, $pv->copy($dst));
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
      if ($self->property->flags & $Property::is_multiple) {
         # check for uniqueness
         my $pv=$parent->contents->[$parent->dictionary->{$self->property->key}];
         foreach my $sibling (@{$pv->values}) {
            if (defined($sibling) && $sibling != $self && $sibling->name eq $new_name) {
               croak( "Parent object '", $parent->name || "UNNAMED", "' already has another subobject ", $self->property->name, " with name '$new_name'" );
            }
         }
      }
      while (!defined($self->transaction) && defined($parent)) {
         $self=$parent;
         $parent=$self->parent;
      }
   }
   if (defined($self->transaction)) {
      $self->transaction->changed=1;
   } elsif (!$self->changed) {
      $self->changed=1;
      $self->ensure_save_changes if $self->persistent;
   }
   return $new_name;
}

# to be called from C++ library
sub set_name {
   $_[0]->name=$_[1];
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
   if ($target_proto->isa($proto)) {
      if (defined($self->transaction) && defined($self->transaction->rule)) {
         croak( "production rule must not change the Object type" );
      }
      # downcast
      if ($target_proto != $proto) {
         my $rule;
         foreach ($proto, @{$proto->super}) {
            if (defined ($rule=$_->auto_casts->{$target_proto})) {
               $proto->auto_casts->{$target_proto}=$rule if $_ != $proto;
               last;
            }
         }
         if (defined $rule) {
            do_downcast($self,$rule);
         } else {
            croak( "downcast from ", $proto->full_name, " to ", $target_proto->full_name, " is not allowed" );
         }
      }
      return $self;
   }

   $proto->isa($target_proto)
      or croak( "invalid cast from ", $proto->full_name, " to ", $target_proto->full_name, "; use copy instead" );

   if (defined($self->property)) {
      if ($self->property->flags & $Property::is_locally_derived) {
         if (!$self->property->type->super->[1]->isa($target_proto)) {
            croak( "can't cast a subobject under ", $self->property->name, " to ", $target_proto->full_name );
         }
         $target_proto=$self->property->type->local_type($target_proto);
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
   # upcast: remove properties defined in the subtypes
   foreach my $pv (@{$self->contents}) {
      if (defined($pv)) {
         if ($target_proto->isa($pv->property->belongs_to)) {
            $pv->property->upcast($target_proto);
         } else {
            assign_min($self->transaction->temporaries->[0], delete $self->dictionary->{$pv->property->key});
            undef $pv;
         }
      }
   }
   bless $self, $target_proto->pkg;
   $self->transaction->changed=1;
   $self->transaction->commit($self) if $need_commit;
   $self;
}
####################################################################################
my $undef_underway;

# four different sorts of descending into subobjects,
# kept separately to optimize a little on the costs of extra branching

# protected:
sub descend {
   my $prop=pop;
   my ($self, @path)=@_;
   if (!is_object($prop)) {
      @path=@$prop;
      $prop=pop @path;
   }
   $undef_underway=0;
   foreach my $prop (@path) {
      if (defined (my $content_index=$self->dictionary->{$prop->key})) {
         my $pv=$self->contents->[$content_index];
         if (defined($pv) && defined($pv->value)) {
            $self=$pv->value;
            next;
         }
         $undef_underway=1;
      }
      return;
   }
   ($self, $prop);
}

# protected:
sub descend_partially {
   my $prop=pop;
   my ($self, $visited, @path)=@_;
   if (!is_object($prop)) {
      @path=@$prop;
      $prop=pop @path;
   }
   foreach my $prop (@path) {
      push @$visited, $self;
      if (defined (my $content_index=$self->dictionary->{$prop->key})) {
         my $pv=$self->contents->[$content_index];
         if (defined($pv) && defined($pv->value)) {
            $self=$pv->value;
            next;
         }
      }
      return;
   }
   ($self, $prop);
}

# protected:
sub descend_with_transaction {
   my $prop=pop;
   my ($self, @path)=@_;
   my $trans=$self->transaction;
   if (!is_object($prop)) {
      @path=@$prop;
      $prop=pop @path;
   }
   foreach my $prop (@path) {
      if (defined (my $content_index=$self->dictionary->{$prop->key})) {
         my $pv=$self->contents->[$content_index];
         if (defined($pv) && defined($pv->value)) {
            $trans=$trans->descend($pv->value);
            $self=$pv->value;
            next;
         }
      }
      return;
   }
   ($self, $prop);
}

# protected:
sub descend_and_create {
   my $prop=pop;
   my ($self, @path)=@_;
   my $trans=$self->transaction;
   if (!is_object($prop)) {
      @path=@$prop;
      $prop=pop @path;
   }
   foreach my $prop (@path) {
      my $content_index=$self->dictionary->{$prop->key};
      if (defined $content_index) {
         my $pv=$self->contents->[$content_index];
         if (defined($pv) && defined($pv->value)) {
            $trans=$trans->descend($pv->value);
            $self=$pv->value;
            next;
         }
      }
      my $pv=$prop->create_subobject($self);
      if (defined $content_index) {
         $self->contents->[$content_index]=$pv;
      } else {
         push @{$self->contents}, $pv;
         $self->dictionary->{$prop->key}=$#{$self->contents};
      }
      $self=$pv->value;
   }
   ($self, $prop);
}
####################################################################################
# protected:
sub create_undefs {
   my $self=shift;
   my $trans=begin_transaction($self);
   foreach my $item (@_) {
      my ($obj, $prop)=descend_and_create($self,$item);
      push @{$obj->contents}, new PropertyValue($prop);
      $obj->dictionary->{$prop->key}=$#{$obj->contents};
   }
   $trans->changed=1;
   $trans->commit($self);
}
####################################################################################
# protected:
sub _lookup_pv {
   my ($self, $req)=@_;
   my ($obj, $prop, $content_index, $pv);

   foreach my $req_item (@$req) {
      my (@path, @types);
      if (($obj, $prop)=descend_partially($self,\@path,$req_item)
          and defined ($content_index=$obj->dictionary->{$prop->key})
          and defined ($pv=$obj->contents->[$content_index])) {
         if ($Application::plausibility_checks and
             !defined($pv->value) && defined($obj->transaction) and
             defined($obj->transaction->rule) and
             $content_index > $obj->transaction->content_end || exists $obj->transaction->backup->{$content_index}) {
            die "rule '", $obj->transaction->rule->header, "' attempts to read its output property ", $prop->name,
                " before it is created\n";
         }
         return $pv;
      }

      # try to find a shortcut

      my $ascent_till=-1;
      if (is_object($req_item)) {
         $path[0]=$self;
         $types[0]=$self->type;
      } else {
         @types=map { $_->type } @path;
         # deduce the types of lacking subobjects
         while ($#types < $#$req_item) {
            my $prop_type=$req_item->[$#types]->type;
            push @types, $prop_type->abstract ? $prop_type->concrete_type($types[-1]) : $prop_type;
            --$ascent_till;
         }
      }

      for (my $i=-1; $i>=$ascent_till; --$i) {
         my $top=$self;
         my $subobj_level;
         if (is_object($req_item)) {
            $subobj_level=0;
         } else {
            $prop=$req_item->[$i];
            $subobj_level=@$req_item+$i;
         }
         my $prod_key=$prop->key;
         my $skip_levels=$i-$ascent_till;
         for (;;) {
            # only look for shortcuts rooted in existing subobjects
            if (--$skip_levels<0) {
               my ($cur_obj, $cur_obj_type)= $subobj_level>=0 ? ($path[$subobj_level], $types[$subobj_level]) : ($top, $top->type);
               my $shortcuts=$cur_obj_type->get_shortcuts_for($prop);
               if ($i==-1) {
                  # consider all shortcuts delivering the requested property directly
                  # but only those without preconditions as we do not want to check these here!
                  foreach my $rule (@$shortcuts) {
                     if (defined ($pv=$rule->descend_to_source($cur_obj))
                         and @{$rule->preconditions} == 0) {
                        return $pv;
                     }
                  }
               } else {
                  # consider shortcuts for subobjects on the path to the requested property, look there for the rest of the path
                  # but only those without preconditions as we do not want to check these here!
                  foreach my $rule (@$shortcuts) {
                     if (defined ($pv=$rule->descend_to_source($cur_obj))
                         and defined ($pv=_lookup_pv($pv->value, [ [ @$req_item[$i+1..-1] ] ]))
                         and @{$rule->preconditions} == 0) {
                        return $pv;
                     }
                  }
               }
            }
            if (--$subobj_level>=0) {
               $prop=$req_item->[$subobj_level];
            } elsif (defined $top->parent) {
               $prop=$top->property;
               $top=$top->parent;
            } else {
               last;
            }
            last unless defined ($prod_key=$prod_key->{$prop->key});
            $prop=$prod_key;
         }
      }
   }
   undef;
}

sub lookup_pv {         # 'request' => PropertyValue or Object
   my ($self, $req)=@_;
   _lookup_pv($self, $self->type->encode_read_request($req, $self, 1));
}

sub lookup {
   if (defined (my $pv=&lookup_pv)) {
      if (@_>2) {
         $pv->property->flags & $Property::is_multiple
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
   my $pv=&lookup_pv;
   defined($pv) ? ($pv->value, $pv->property->name) : ();
}
####################################################################################
# queries for special rule preconditions
sub exists_req {
   my ($self, $req)=@_;
   eval_input_list($self, $self->type->encode_read_request($req), 1);
}

sub exists_prop {
   my ($self, $path)=@_;
   my $content_index;
   if (my ($obj, $prop)=descend($self, $path)) {
      defined( $content_index=$obj->dictionary->{$prop->key} ) &&
      defined( $obj->contents->[$content_index] )
   } else {
      0
   }
}
####################################################################################
# private:
sub _put_pv {
   my ($self, $prop, $pv)=@_;
   my $content_index=$self->dictionary->{$prop->key};
   if (defined $self->transaction->rule) {
      # creating a property in a rule
      defined($content_index)
         or croak( "attempt to create property '", $prop->name, "' which is not declared as a rule target" );
      $self->contents->[$content_index]=$pv;

   } elsif (defined $content_index) {
      # overwriting an existing property
      if ($self->transaction->content_end<0 || $prop->flags & ($Property::is_multiple | $Property::is_mutable)) {
         $self->contents->[$content_index]=$pv;
      } else {
         croak( "can't change the property ", $prop->name );
      }

   } else {
      # new property created outside of a production rule
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
   if (defined($self->transaction)) {
      $temp ||= $self->transaction->entirely_temp;
   } else {
      if ($prop->flags & ($Property::is_multiple | $Property::is_mutable)) {
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
      push @{$self->transaction->temporaries}, ($prop->flags & $Property::is_multiple ? [ $prop, $pv->values->[0] ] : $prop);
   } elsif (not $prop->flags & $Property::is_non_storable) {
      $self->transaction->changed=1;
   }
   $self->transaction->commit($self) if $need_commit;
   $pv->value if defined(wantarray);
}
####################################################################################
sub put_ref {
   my ($self, $prop, $value, $flags)=@_;
   _put_pv($self, $prop, new PropertyValue($prop, $value, $flags || (defined($value) && $PropertyValue::is_strong_ref)));
}
####################################################################################
# private:
sub ensure_unique_name {
   my ($pv, $added_obj, $temp)=@_;
   my ($found, $next);
   if (defined $added_obj->name) {
      foreach my $sibling (@{$pv->values}) {
         if (defined($sibling) && $sibling != $added_obj && $sibling->name eq $added_obj->name) {
            $found=$added_obj->name;
            last;
         }
      }
      return unless defined($found);
      $next=2;
   } else {
      $found= $temp ? "temp" : "unnamed";
      $next=0;
   }
   foreach my $sibling (@{$pv->values}) {
      if (defined($sibling) && $sibling != $added_obj && $sibling->name =~ m/^\Q$found\E\#(\d+)$/) {
         assign_max($next, $1+1);
      }
   }
   if (defined $added_obj->name) {
      warn_print( "The subobject name ", $added_obj->name, " already occurs among other instances of ", $pv->property->name, ".\n",
                  "Replaced with a unique name '$found#$next'." );
   }
   set_name_trusted($added_obj, "$found#$next");
}
####################################################################################
sub add {
   my ($self, $prop_name)=splice @_, 0, 2;
   my $need_commit;
   my $prop=$self->type->lookup_property($prop_name) ||
            try_auto_cast($self,$prop_name,0) ||
            croak( "unknown property ", $self->type->full_name, "::$prop_name" );
   if ($prop->flags & $Property::is_multiple) {
      my $temp= is_integer($_[0]) && $_[0] == $PropertyValue::is_temporary && shift;
      my $value = is_object($_[0]) ? shift : new_named($prop->concrete($self)->type, @_%2 ? shift : undef);

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
      ensure_unique_name($pv, $value, $temp);
      if (@_) {
         eval { fill_properties($value, $value->type, @_) };
         if ($@) {
            $self->rollback;
            die $@;
         }
      }
      if ($temp) {
         assign_max($#{$self->transaction->temporaries},0);
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

sub add_temp { add(@_,1) }
####################################################################################
sub take {
   my ($self, $prop_name, $value, $temp)=@_;
   my @req=$self->type->encode_request_element($prop_name, $self);
   my $need_commit;
   if (!defined($self->transaction)) {
      if ($req[-1]->flags & ($Property::is_multiple & $Property::is_mutable)) {
         $need_commit=1;
         begin_transaction($self);
      } else {
         croak( "can't add or change the property $prop_name" );
      }
   }
   my ($obj, $prop)=descend_and_create($self,@req);
   put($obj,$temp,$prop,$value);
   $self->transaction->commit($self) if $need_commit;
}

sub take_temp { take(@_,1) }
####################################################################################
sub remove {
   my $self=shift;
   @_ or return;
   my $need_commit;
   if (is_object($_[0])) {
      # some sanity checks up front
      foreach my $sub_obj (@_) {
         is_object($sub_obj)
           or croak( "can't mix subobjects and property names in the same call to remove()" );
         $sub_obj->property->flags & $Property::is_multiple
           or croak( "only multiple sub-objects can be removed by reference" );
         $sub_obj->parent==$self
           or croak( "only own sub-objects can be removed" );
      }
      if (defined($self->transaction)) {
         if (defined($self->transaction->rule)) {
            croak( "attempt to remove a sub-object in a rule" );
         }
      } else {
         $need_commit=1;
         begin_transaction($self);
      }

      foreach my $sub_obj (@_) {
         my $content_index=$self->dictionary->{$sub_obj->property->key};
         my $sub_index=$sub_obj->parent_index;
         my $pv;
         if (defined($content_index)  and
             ($pv=$self->contents->[$content_index], $pv->values->[$sub_index]==$sub_obj)) {
            if (defined($sub_obj->transaction) && !$need_commit) {
               $sub_obj->transaction->rollback($sub_obj);
               delete $self->transaction->subobjects->{$sub_obj};
            }
            if ($sub_index == $#{$pv->values}) {
               pop @{$pv->values};
               if (!@{$pv->values}) {
                  delete $self->dictionary->{$sub_obj->property->key};
                  undef $self->contents->[$content_index];
                  assign_min($self->transaction->temporaries->[0], $content_index);
               }
            } else {
               undef $pv->values->[$sub_index];
               $pv->has_gaps=1;
               if (!$sub_obj->is_temporary) {
                  assign_max($#{$self->transaction->temporaries},0);
                  push @{$self->transaction->temporaries}, [ $sub_obj->property, undef ];
               }
            }
            undef $sub_obj->parent;
            undef $sub_obj->property;
            undef $sub_obj->parent_index;
            undef $sub_obj->is_temporary;
         } else {
            $self->rollback;
            croak( "internal inconsistency: sub-object list for property ", $subobj->property->name, " is corrupted" );
         }
      }
      $self->transaction->changed=1;
      $self->transaction->commit($self) if $need_commit;

   } else {
      my $from_production_rule=defined($self->transaction) && defined($self->transaction->rule);
      my @to_remove=map {
         ref($_)
           and croak( "can't mix property names and subobjects in the same call to remove()" );
         my @req=$self->type->encode_request_element($_, $self);
         if ($Application::plausibility_checks && $from_production_rule) {
            defined($self->transaction->rule->matching_input(\@req))
              or croak( "a production rule can only remove its own source properties" );
         }
         @req==1 ? @req : \@req
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
            if (defined($pv) and not($from_production_rule or $prop->flags & ($Property::is_multiple | $Property::is_mutable))) {
               if (instanceof Object($pv) && !($prop->flags & $Property::is_produced_as_whole)) {
                  push @reconstruct_request, enumerate_atomic_properties($pv, is_object($req) ? [ $req ] : $req);
               } else {
                  push @reconstruct_request, [ $req ];
               }
            }
            $obj->transaction->backup->{$content_index} ||= $pv;
            undef $obj->contents->[$content_index];
            assign_min($obj->transaction->temporaries->[0], $content_index);
            $obj->transaction->changed=1 unless $prop->flags & $Property::is_non_storable;
         } else {
            $self->rollback if $need_commit;
            local $_=$req;
            croak( "property ", &Rule::print_path, " does not exist in the given object" );
         }
      }
      if (@reconstruct_request) {
         # create a nested transaction isolating all possible Scheduler feats
         $need_commit or $self->begin_transaction;
         $self->transaction->entirely_temp=1;
         my $sched=new Scheduler::InitRuleChain($self, create Rule('request', \@reconstruct_request, 1));
         my $allow= $sched->gather_rules && defined($sched->resolve($self, 1));
         $need_commit or $self->transaction->commit($self);
         unless ($allow) {
            $self->rollback;
            croak( "can't remove the ",
                   @reconstruct_request>1 ? "properties " : "property ",
                   join(", ", map { &Rule::print_path } @reconstruct_request),
                   ": neither multiple, nor mutable, nor unambiguously reconstructible from the remaining properties" );
         }
      }
      $self->transaction->commit($self) if $need_commit;
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
   my %squeeze_multi;
   foreach (@$data) {
      my $content_index;
      if (!is_object($_)) {
         defined($content_index=$self->dictionary->{$_->[0]->key}) or next;
         my $pv=$self->contents->[$content_index];
         # TODO: !defined($sub_obj) will become impossible after complete phasing out of parent_index and squeeze_multi
         if (defined(my $sub_obj=$_->[1])) {
            if (is_object($sub_obj)) {
               # via multiple sub-object (might be removed in the meanwhile)
               if ($#$_==2) {
                  # only single properties are temporary
                  cleanup($sub_obj,$_->[2]);
                  next if $#{$sub_obj->contents}>=0;
               }
               if ($sub_obj == $pv->values->[-1]) {
                  pop @{$pv->values};
                  next if @{$pv->values};
               } else {
                  my $sub_index=$#{$pv->values};
                  do {
                     $sub_index>=0
                       or croak( "internal inconsistency: multiple subobject instance recorded in temporary list does not occur in the parent property" );
                  } while ($pv->values->[--$sub_index] != $sub_obj);
                  undef $pv->values->[$sub_index];
                  $squeeze_multi{$pv}=$content_index;
                  next;
               }
            } else {
               # via singular sub-object: $pv==subobject, $sub_obj==list of properties
               cleanup($pv, $sub_obj);
               next if $#{$pv->contents}>=0;
            }
         } else {
            $pv->has_gaps or croak( "internal inconsistency: flag has_gaps for property ", $pv->property->name, " is not set although child $sub_index removed" );
            $squeeze_multi{$pv}=$content_index;
            next;
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

   # TODO: this disappears when parent_index is phased out
   while (my ($pv, $content_index)=each %squeeze_multi) {
      if (!$pv->squeeze) {
         delete $self->dictionary->{$pv->property->key};
         if ($content_index==$#{$self->contents}) {
            pop @{$self->contents};
         } else {
            undef $self->contents->[$content_index];
            assign_min($deleted_index, $content_index);
         }
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
   my $self=shift;
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
   my $self=shift;
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
sub eval_input_list {   # [ PropertyPath, ... ] => true (ready) || false (not exist) || undef
   my ($self, $input_list, $allow_undefs)=@_;
   my ($content_index, $pv, $rc);
   foreach my $item (@$input_list) {
      if (my ($obj, $prop)=descend($self,$item)) {
         if (defined ($content_index=$obj->dictionary->{$prop->key}) &&
             defined ($pv=$obj->contents->[$content_index])) {
            return $allow_undefs || defined($pv->value) || undef;
         } else {
            $rc=0;
         }
      } elsif (!$undef_underway) {
         $rc=0;
      }
   }
   $rc
}
####################################################################################
sub allow_undef_value {
   my ($self, $prop)=@_;
   if (defined($self->transaction) && defined($self->transaction->rule) &&
       !($self->transaction->rule->flags & ($Rule::is_definedness_check | $Rule::is_initial))) {
      die "attempt to use a property ", is_object($prop) ? $prop->name : $prop, " with undefined value\n";
   }
   undef
}

sub give_pv {
   my ($self, $req_string)=@_;
   my $req=$self->type->encode_read_request($req_string,$self);
   _lookup_pv($self,$req)
   || do {
      if (defined($self->transaction)) {
         if (defined($self->transaction->rule)) {
            die "attempt to access ", ($#$req>0 ? "undefined properties " : "an undefined property "), $req_string, "\n";
         }
         $self->commit;
      }
      provide_request($self, [ $req ]);
      _lookup_pv($self,$req)
      or die( $#$req>0 ? "properties " : "property ", $req_string, " not created as expected" );
   }
}

sub give {
   if (@_==2) {
      &give_pv->value;
   } else {
      my ($self, $req_string)=splice @_, 0, 2;
      my $req=$self->type->encode_read_request($req_string,$self);
      if ($#$req>0) {
         croak( "mixing alternatives with property-based lookup of multiple sub-objects is not allowed" );
      }
      if (my ($obj, $prop)=descend($self,$req->[0])) {
         $prop->flags & $Property::is_multiple
            or croak( "property-based lookup can only be performed on sub-objects with `multiple' attribute" );
         get_multi($obj, @_, $prop);
      } else {
         die "can't create any subobject on a non-existing path $req->[0]\n";
      }
   }
}

sub give_with_name {
   my $pv=&give_pv;
   ($pv->value, $pv->property->name);
}
####################################################################################
# protected:
sub provide_request {
   my ($self, $request)=@_;
   my ($success, $sched)=&Scheduler::resolve_request;
   return 1 if $success>0;
   my @lacking;
   foreach my $input_list (@$request) {
      eval_input_list($self, $input_list, 1)
         or push @lacking, $input_list;
   }
   if ($success==0) {
      if ($Verbose::rules) {
         my @failed_precond;
         while (my ($rule, $rc)=each %{$sched->run}) {
            if ($rc==$exec_failed && ($rule->flags & $Rule::is_any_precondition)) {
               push @failed_precond, $rule->header."\n";
            }
         }
         my $lacking=join(", ", map { "'".Rule::print_input_list($_)."'" } @lacking);
         if (@failed_precond) {
            warn_print("could not compute $lacking probably because of unsatisfied preconditions:\n", @failed_precond);
         } else {
            warn_print("available properties insufficient to compute $lacking");
         }
      }
      create_undefs($self, map { @$_ } @lacking);
      0;
   } else {
      die "no more rules available to compute ", join(", ", map { "'".Rule::print_input_list($_)."'" } @lacking), "\n";
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
   provide_request($self, [ map { $self->type->encode_read_request($_, $self) } @_ ]);
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
   my $for_auto_cast;
   my $sched=new Scheduler::InitRuleChain($self, create Rule('request', [ map { $self->type->encode_read_request($_,$self,\$for_auto_cast) } @_ ], 1));
   if (defined $for_auto_cast) {
      my $chain;
      if (@{$for_auto_cast->rules} &&
          defined ($chain=$sched->gather_rules &&
                          $sched->resolve($self,!$Scheduler::dry_run))) {
         unshift @{$chain->rules}, @{$for_auto_cast->rules};
      }
      $chain;
   } else {
      $sched->gather_rules &&
      $sched->resolve($self,!$Scheduler::dry_run);
   }
}
####################################################################################
sub disable_rules {
   my $self=shift;
   my @rules=$self->type->find_rules_by_pattern(@_)
     or die "no matching rules found\n";
   foreach my $rule (@rules) {
      foreach my $prod ($rule, $rule->with_permutation ? ($rule->with_permutation, @{$rule->with_permutation->actions}) : ()) {
         $Scope->begin_locals;
         local $self->failed_rules->{$prod}=1;
         $Scope->end_locals;
      }
   }
}
####################################################################################
sub apply_rule {
   my $self=shift;
   my @rules=$self->type->find_rules_by_pattern(@_)
     or die "no matching rules found\n";
   Scheduler::resolve_rules($self, \@rules)
     or croak( "can't apply ", @rules>1 ? "any of requested rules" : "the requested rule",
               " due to insufficient source data and/or unsatisfied preconditions" );
}
####################################################################################
sub list_names {
   my $self=shift;
   if (defined(my $prop=$self->property)) {
      if ($prop->flags & $Property::is_multiple) {
         my $pv=$self->parent->contents->[$self->parent->dictionary->{$prop->key}];
         return map { $_->name } grep { defined($_) } @{$pv->values};
      }
   }
   $self->name
}
####################################################################################
sub list_properties {
   my ($self, $rec)=@_;
   map {
      if (defined($_)) {
         if (!$rec || instanceof PropertyValue($_)) {
            $_->property->name
         } elsif (instanceof Object($_)) {
            my $prefix=$_->property->name;
            map { "$prefix.$_" } list_properties($_,$rec)
         } else {
            my $prop=$_->property->name;
            my $prefix;
            map { $prefix="$prop\[".$_->name."]"; map { "$prefix.$_" } list_properties($_,$rec) } @{$_->defined_values}
         }
      } else {
         ()
      }
   } @{$self->contents};
}
####################################################################################
sub enumerate_atomic_properties {
   my ($self, $descent_path)=@_;
   map {
      if (defined($_) && !($_->property->flags & ($Property::is_multiple | $Property::is_mutable))) {
         if (instanceof PropertyValue($_)) {
            [ @$descent_path, $_->property->declared ]
         } else {
            enumerate_atomic_properties($_, local_push($descent_path, $_->property->declared));
         }
      } else {
         ()
      }
   } @{$self->contents};
}
####################################################################################
sub properties {
    my ($self)=@_;
    print $self->name, "\n", $self->description, "\n";
    map { print $_, "\n", $self->$_,"\n\n" } $self->list_properties;
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
   if (defined $content_index) {
      _get_alternatives();
   } else {
    TRY_ALT: {
         my $subtype=$prop->type;
         my $descend_path;
         my @alt= instanceof ObjectType($subtype) ? _get_alternatives($descend_path) : _get_alternatives();

         if (defined $descend_path) {

            if (defined($self->transaction)) {
               if (defined($self->transaction->rule)) {
                  die "attempt to access an undefined property ", $prop->name, "\n";
               }
               $self->commit;
            }

            my @req=($prop);
            foreach (@$descend_path) {
               if (defined (my $subprop=$subtype->lookup_property($_))) {
                  push @req, $subprop;
                  last unless instanceof ObjectType($subprop->type);
                  $subtype=$subprop->specialization($subtype)->type;
               } else {
                  last;
               }
            }

            if (@alt && @$descend_path==$#req) {
               provide_request($self, [ [ \@req,
                                          map { [ (@req[0..$#req-1], $subtype->lookup_property($_) || croak( "unknown property $_" )) ]
                                              } @alt ] ]);
            } else {
               provide_request($self, [ [ \@req ] ]);
            }
            $content_index=$self->dictionary->{$prop->key};

         } else {

            foreach my $alt_prop (@alt) {
               $alt_prop=$self->type->lookup_property($alt_prop)
                         || croak( "unknown property $alt_prop" );
               last TRY_ALT if defined ($content_index=$self->dictionary->{$alt_prop->key});
            }

            if (defined($self->transaction)) {
               if (defined($self->transaction->rule)) {
                  if (@alt) {
                     die "attempt to access undefined properties ", join(" | ", map { $_->name} $prop, @alt), "\n";
                  } else {
                     die "attempt to access an undefined property ", $prop->name, "\n";
                  }
               }
               $self->commit;
            }

            unshift @alt, $prop;
            provide_request($self, [ \@alt ]);
            foreach $prop (@alt) {
               last if defined ($content_index=$self->dictionary->{$prop->key});
            }
         }
      }
      defined($content_index) or die "can't create property ", $prop->name, "\n";
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
         return defined($content_index) ? $self->contents->[$content_index]->defined_values : [ ];
      }
      if (@_==1 && !ref($_[0])) {
         # search by name
         return defined($content_index) ? $self->contents->[$content_index]->find_by_name(@_) : undef;
      }
      (defined($content_index) ? $self->contents->[$content_index] : new PropertyMultipleValue($prop->concrete($self), [ ]))->find_or_create($self,@_);
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
         $self->contents->[$content_index]->replace_or_add($self,@_,$value);
      } else {
         push @{$self->contents}, $prop->accept->($value,$self);
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
      if (is_object($data) && defined (my $type=UNIVERSAL::can($data,"type"))) {
         $type=$type->();
         if (instanceof ObjectType($type)) {
            croak( "can't attach a subobject of type ", $type->full_name, ": only atomic properties are allowed as attachments" );
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
         $data= $construct ? $data->type->construct->($self->give($construct), $data) : $data->type->construct->($data);
      }
      $self->attachments->{$name}=[ $data, $construct ];
   }
}
####################################################################################
# this method is called only as fallback,
# if no ObjectType in the type hierarchy has a specific suffix
sub default_file_suffix : method {
   (shift)->type->application->default_file_suffix
}
####################################################################################
# protected:
sub update_locally_derived {
   my ($self, $parent)=@_;
   $self->property=$self->property->concrete($parent);
   bless $self, $self->property->type->local_type($self->type->pure_type)->pkg;
}

sub update_locally_derived_rec {
   my $self=shift;
   foreach my $pv (@{$self->contents}) {
      if (instanceof Object($pv)) {
         if ($pv->property->flags & $Property::is_locally_derived) {
            update_locally_derived($pv, $self);
         }
         update_locally_derived_rec($pv);
      } elsif (instanceof PropertyMultipleValue($pv)) {
         if ($pv->property->flags & $Property::is_locally_derived) {
            $pv->property=$pv->property->concrete($self);
         }
         foreach my $subobj (@{$pv->values}) {
            if ($pv->property->flags & $Property::is_locally_derived) {
               $subobj->property=$pv->property;
               bless $subobj, $pv->property->type->local_type($subobj->type->pure_type)->pkg;
            }
            update_locally_derived_rec($subobj);
         }
      }
   }
}

# protected:
sub cast_to_type {
   my ($self, $target_proto)=@_;
   if (defined($self->property) && $self->property->flags & $Property::is_locally_derived) {
      $target_proto=$self->property->type->local_type($target_proto);
   }
   bless $self, $target_proto->pkg;
   update_locally_derived_rec($self);
}

# protected:
sub do_downcast {
   my ($self, $rule)=@_;
   if (@{$rule->preconditions}) {
      Scheduler::resolve_rule($self,$rule)>0
      or croak( "Can't perform a downcast to ", $rule->output->full_name, " due to unsatisfied preconditions" );
   }
   $rule->execute($self);
}

# protected:
#  mode==1: "dry run", only check if possible but don't change anything
#  mode==ARRAY: gather rules for a canned schedule
sub try_auto_cast {
   my ($self, $prop_name, $mode)=@_;
   if (my ($rule, $obj, $prop)=$self->type->lookup_auto_cast($prop_name, $self, 0)) {
      if (ref($mode)) {
         if (defined (my $chain=Scheduler::resolve_rule($obj, $rule, !$Scheduler::dry_run))) {
            if ($Scheduler::dry_run) {
               @{$chain->rules}=($rule);
            } else {
               cast_to_type($obj, $rule->output);
            }
            $$mode=$chain;
         } else {
            $$mode=new Scheduler::RuleChain($obj, undef);
         }
      } elsif (!$mode) {
         do_downcast($obj, $rule);
      }
      ($rule->output, $prop)
   } else {
      ()
   }
}

sub AUTOLOAD {
   my $self=$_[0];
   my ($prop_name)=$AUTOLOAD=~/($id_re)$/xo;
   if (instanceof Object($self)) {
      if (my ($rule, $obj, $prop)=$self->type->lookup_auto_cast($prop_name, $self, 1)) {
         do_downcast($obj, $rule);
         if (is_object($prop)) {
            # it's indeed a property
            push @_, $prop;
            $prop->flags & $Property::is_multiple ? &get_multi : &get;
         } else {
            # it's a method
            &$prop;
         }
      } else {
         croak( "Object ", $self->type->full_name, " does not have a property or method $prop_name" );
      }
   } else {
      croak( "undefined subroutine $AUTOLOAD called" );
   }
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
   my $proto=$self->type;
   if ($self->parent && $self->property->flags & $Property::is_locally_derived) {
      $proto=$proto->pure_type;
   }
   my (@descend_to_permuted, @perms);
   if ($_[0] eq "permutation") {
      $perms[0]=$proto->lookup_permutation($_[1]) || croak( "unknown permutation $_[1]" );
      splice @_, 0, 2;
   } else {
      my @path=$proto->encode_request_element($_[0], $self);
      @perms=grep { defined($_->find_sensitive_sub_property(@path)) } $proto->list_permutations;
      unless (@perms) {
         my $subobj=$self;
         my ($content_index, $pv);
         for (my $i=0; $i<$#path; ++$i) {
            if (defined ($content_index=$subobj->dictionary->{$path[$i]->key}) &&
                defined ($pv=$subobj->contents->[$content_index])) {
               if (@perms=grep { defined($_->find_sensitive_sub_property(@path[$i+1..$#path])) } $pv->property->type->list_permutations) {
                  @descend_to_permuted=@path[0..$i];
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
         @path=$proto->encode_request_element($_[$i], $self);
         if (@descend_to_permuted) {
            splice @path, 0, scalar @descend_to_permuted;
         }
         @perms= grep { defined($_->find_sensitive_sub_property(@path)) } @perms
            or croak( "copy_permuted: no permutation type known which would simultaneously affect ", join(", ", map { $_[$_*2] } 0..$i/2));
      }
      if (@perms!=1) {
         croak( "copy_permuted: permutation type can't be unambiguously chosen; permutations in question are ",
                join(", ", map { $_->name } @perms) );
      }
   }
   my ($rule, $filter)=permuting Rule($proto, shift(@perms), @descend_to_permuted);
   my $copy=new($proto->pkg, $self->name);
   $copy->description=$self->description;
   begin_transaction($copy);

   if ($self->parent && $self->property->flags & $Property::is_locally_derived) {
      copy_contents($self, $copy,
                    sub {
                       $proto->isa($_[1]->property->declared->belongs_to) ? &$filter : ()
                    });
   } else {
      copy_contents($self, $copy, $filter);
   }

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
               if ($pv->property->flags & $Property::is_multiple) {
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
   my ($self, $pv1, $pv2, $ignore)=@_;
   if ($pv1->property->flags & $Property::is_multiple) {
      my $v2=instanceof PropertyMultipleValue($pv2) ? $pv2->values : $pv2;
      if ((my $l=$#{$pv1->values}) == $#$v2) {
         if (wantarray) {
            map { my $o1=$pv1->values->[$_]; map { $_->[2] ||= $o1; $_ } $o1->diff($v2->[$_], ignore_subproperties($ignore, $pv1->property->name)) } 0..$l;
         } else {
            $pv1->values->[$_]->diff($v2->[$_], ignore_subproperties($ignore, $pv1->property->name)) and return 1 for 0..$l;
            ()
         }
      } else {
         !wantarray || [ $pv1, $pv2 ]
      }
   } else {
      my $v2=instanceof PropertyValue($pv2) ? $pv2->value : $pv2;
      if (defined($pv1->value) && defined($v2)) {
         if (instanceof Object($pv1->value)) {
            if (ref($pv1->value) eq ref($v2)) {
               if (wantarray) {
                  map { $_->[2] ||= $pv1->value; $_ } $pv1->value->diff($v2, ignore_subproperties($ignore, $pv1->property->name));
               } else {
                  $pv1->value->diff($v2, ignore_subproperties($ignore, $pv1->property->name));
               }
            } else {
               !wantarray || [ $pv1, $pv2 ]
            }
         } else {
            (defined($pv1->property->equal) ? select_method($pv1->property->equal,$self,1)
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
   if (keys %options) {
      croak( "Object::diff - unrecognized option(s) ", join(", ", keys %options) );
   }

   my %ignore;
   if ($ignore) {
      @ignore{@$ignore}=@_;
   }

   my ($my_contents, $other_contents)=
      $ignore
      ? ( map { [ grep { !exists $ignore{$_->property->name} } @{$_->contents} ] } $self, $other )
      : ( $self->contents, $other->contents );

   if (@$my_contents != @$other_contents) {
      if (wantarray) {
         push @diff, map { [ undef, $_ ] } grep { defined($_) && !exists $self->dictionary->{$_->property->key} } @$other_contents;
      } else {
         return 1;
      }
   }

   foreach my $pv (@$my_contents) {
      my $other_index=$other->dictionary->{$pv->property->key};
      if (defined $other_index) {
         if (wantarray) {
            push @diff, diff_properties($self, $pv, $other->contents->[$other_index], $ignore);
         } else {
            diff_properties($self, $pv, $other->contents->[$other_index], $ignore) and return 1;
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
sub register_object_saving {
   add AtEnd("Object", sub {
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
                      $obj->changed=0;
                   }
                }
             });
   1;
}

sub ensure_save_changes {
   my $self=shift;
   $at_end ||= register_object_saving();
   $save_at_end{$self}=undef;
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

sub attach {
   my ($self, $name, $value)=@_;
   $self->attachments->{$name}=$value;
}


1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
