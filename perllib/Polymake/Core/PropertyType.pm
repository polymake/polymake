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

package Polymake::Core::PropertyType;

declare $trusted_value;
declare $nested_instantiation=0;

sub canonical_fallback { }
sub equal_fallback { $_[0] == $_[1] }
sub isa_fallback : method { UNIVERSAL::isa($_[1], $_[0]->pkg) }
sub coherent_type_fallback { undef }
sub parse_fallback : method { croak( "no string parsing routine defined for class ", $_[0]->full_name ) }
sub toString_fallback { "$_[0]" }
sub serialize_fallback { $_[0] }
sub deserialize_meth : method { $_[0]->construct->($_[1]) }

sub init_fallback { }
sub performs_deduction { 0 }
sub type_param_index { undef }

sub type : method { $_[0] }
define_function(__PACKAGE__, ".type", \&type);
sub generic { undef }

use Polymake::Struct (
   [ new => '$$$;$' ],
   [ '$name | full_name | mangled_name | xml_name' => '#1' ],
   [ '$pkg' => '#2' ],
   [ '$application' => '#3' ],
   [ '$extension' => '$Application::extension' ],
   [ '$super' => '#4' ],
   [ '$params' => 'undef' ],
   [ '$dimension' => '0' ],
   [ '&canonical' => '->super || \&canonical_fallback' ],          # object => void
   [ '&equal'  => '->super || \&equal_fallback' ],                 # object1, object2 => bool
   [ '&isa' => '->super || \&isa_fallback' ],                      # object => bool
   [ '$upgrades' => 'undef' ],                                     # optional hash PropertyType => 1/0/-1
   [ '&coherent_type' => '->super || \&coherent_type_fallback' ],  # object of a different type => PropertyType of this or a derived type or undef
   [ '&toString' => '->super || \&toString_fallback' ],            # object => "string"
   [ '&toXML' => '->super' ],                                      # object, XMLwriter, opt. attributes =>
   [ '&toXMLschema' => 'undef || \&Polymake::Core::XMLwriter::type_toXMLschema' ],          # XMLwriter, opt. attributes =>
   [ '&XMLdatatype' => '->super' ],                                # => "string" referring to a XML Schema datatype or a pattern
   [ '&serialize' => '->super || \&serialize_fallback' ],          # object, { options } => ( value, { attrs } ) or nothing
   [ '&deserialize' => '->super || \&deserialize_meth' ],
   [ '&JSONschema' => 'undef' ],                                   # TODO
   [ '$construct_node' => 'undef' ],                               # Overload::Node
   [ '&construct' => '\&construct_object' ],                       # args ... => object
   [ '&parse' => '->super || \&parse_fallback' ],                  # "string" => object
   [ '&abstract' => 'undef || \&type' ],
   [ '&perform_typecheck' => '\&concrete_typecheck' ],             # cf. Overload resolution
   [ '&init' => '->super || \&init_fallback' ],
   [ '$context_pkg' => 'undef' ],
   [ '$cppoptions' => 'undef' ],
   [ '$operators' => 'undef' ],
   [ '$help' => 'undef' ],              # InteractiveHelp (in interactive mode, when comments are supplied for user methods)
);

declare @override_methods=qw( canonical equal isa coherent_type serialize parse toString XMLdatatype init );

####################################################################################
#
#  Constructor
#
#  new PropertyType('name', 'package', Application, super PropertyType)
#
sub new {
   my $self=&_new;
   my $pkg=$self->pkg;
   Struct::learn_package_retrieval($self, \&pkg);
   my $self_sub=sub { $self };
   define_function($pkg, "type", $self_sub, 1);
   define_function($pkg, ".type", $self_sub);
   if ($self->super) {
      $self->dimension=$self->super->dimension;
      $self->params=$self->super->params;
      $self->operators=$self->super->operators;
      if ($self->construct_node=$self->super->construct_node) {
         create_method_new($self);
      }
      establish_inheritance($self, $self->super);
   }
   $self;
}

##################################################################################
sub establish_inheritance {
   my ($self, $super1, $super2)=@_;
   my @isa=($super1->pkg);
   if ($super2 && $super2 != $super1) {
      push @isa, $super2->pkg;
   }
   namespaces::using($self->pkg, @isa);
   no strict 'refs';
   @{$self->pkg."::ISA"}=@isa;
   mro::set_mro($self->pkg, "c3");
}

sub derived {
   my ($self)=@_;
   map { $_->type } @{mro::get_isarev($self->pkg)}
}

# perl 5.16 had a weird bug in c3 mro
if ($] < 5.018) {
   *derived = sub {
      my ($self)=@_;
      map { $_->type } grep { not /::SUPER$/ } @{mro::get_isarev($self->pkg)}
   }
}
##################################################################################
sub add_constructor {
   my ($self, $name, $label, $code, $arg_types)=@_;
   $self->construct_node //= do {
      create_method_new($self);
      new_root Overload::Node;
   };
   Overload::add_instance($self->pkg, ".$name", $label, $code, $arg_types, undef, $self->construct_node);
}

sub create_method_new : method {
   my ($self)=@_;
   if ($self->abstract) {
      # bare generic name: try to create the type instance with default parameters
      define_function($self->pkg, "new", sub { splice @_, 0, 1, $self->pkg->typeof(); &construct_object });
   } else {
      define_function($self->pkg, "new", sub { splice @_, 0, 1, $self; &construct_object });
   }
}

sub construct_object : method {
   if (@_ == 2 && is_string($_[1])) {
      $_[0]->parse->($_[1])
   } else {
      &{ $_[0]->construct_node->resolve(\@_) // Overload::complain(undef, "new", 1, \@_) }
   }
}
##################################################################################
sub parse_error {
   my $start=pos;
   die "invalid ", $_[0]->full_name, " value starting at \"",
       length($_)>$start+30 ? substr($_,$start,30)."..." : substr($_,$start),
       "\"\n";
}
##################################################################################
# used in overload resolution
sub deduce_least_derived {
   my ($self, $other)=@_;
   $self==$other || UNIVERSAL::isa($other->pkg, $self->pkg)
   ? $self :
   UNIVERSAL::isa($self->pkg, $other->pkg)
   ? $other : undef
}

sub deduce_most_enhanced {
   my ($self, $other)=@_;
   if ($self==$other) {
      $self
   } elsif (defined (my $upgrades=$self->upgrades)) {
      my $rel=$upgrades->{$other};
      $rel ? ($rel>0 ? $self : $other) : undef
   } else {
      undef
   }
}

sub concrete_typecheck : method {
   my ($self, undef, $given)=@_;
   $self==$given || UNIVERSAL::isa($given->pkg, $self->pkg)
   ? $self : undef
}

sub no_typecheck : method { croak( $_[0]->name, " inadvertently involved in overload resolution" ) }

sub descend_to_generic {
   my ($self, $pkg)=@_;
   if (defined $pkg) {
      do {
         return $self if $self->context_pkg eq $pkg || $self->pkg eq $pkg;
      } while ($self=$self->super);
   }
   undef
}

##################################################################################
# used in overload resolution
sub typecheck : method {
   my ($self, $full_args, $args, $arg_index, $backtrack)=@_;
   if (defined (my $obj_proto = Overload::fetch_type($args->[$arg_index]))) {
      $self->perform_typecheck->($full_args, $obj_proto, $backtrack);
   }
}
##################################################################################
sub add_upgrade_relations {
   my $self=shift;
   foreach my $other (@_) {
      my $upgrades=($self->upgrades //= { });
      $upgrades->{$other}=1;
      while (my ($third, $rel3)=each %{$other->upgrades //= { } }) {
         if ($rel3>0) {
            $upgrades->{$third}=1;
            $third->upgrades->{$self}=-1;
         }
      }
      $other->upgrades->{$self}=-1;
   }
   $self
}
#################################################################################
# produce a name sufficient for reconstruction from a data file
sub qualified_name {
   my ($self, $in_app) = @_;
   (not(defined($in_app) and $in_app == $self->application || $in_app->imported->{$self->application->name}) && $self->application->name . "::") .
   $self->name
}

sub required_extensions {
   my ($self, $include_bundled) = @_;
   defined($self->extension) && ($include_bundled >= $self->extension->is_bundled) ? ($self->extension) : ()
}
#################################################################################
sub locate_own_help_topic {
   my ($self, $whence, $force)=@_;
   return if !defined($self->application);
   my $full_name=$self->full_name;
   my $topic=$self->application->help->find($whence, $full_name) //
             (defined($self->generic)
              ? ($force ? croak( "internal error: asked to create a help node for a parametrized type instance" )
                        : $self->application->help->find($whence, $self->generic->name))
              : ($force ? $self->application->help->add([$whence, $full_name])
                        : return));
   if (!exists $topic->annex->{type}) {
      weak($topic->annex->{type}=$self);
      push @{$topic->related},
           uniq( map { ($_, @{$_->related}) } grep { defined and $_ != $topic and !defined($_->spez_topic) } map { $_->help_topic }
                 $whence eq "property_types" ? defined($self->super) ? $self->super : () : @{$self->super} );
   }
   $self->help=$topic;
}

sub help_topic {
   my $self = shift;
   $self->help // locate_own_help_topic($self, "property_types", @_);
}
#################################################################################
package Polymake::Core::PropertyParamedType;

use Polymake::Struct (
   [ '@ISA' => 'PropertyType' ],
   [ new => '$$$' ],
   [ '$name' => '#1->name' ],
   [ '$pkg' => '#1->pkg' ],
   [ '$application' => '#1->application' ],
   [ '$extension' => '#1->extension' ],
   [ '$super' => '#2 // #1' ],
   [ '$params' => '#3' ],
   [ '$generic' => '#1' ],
   '@derived_abstract',
);

use Polymake::Struct (
   [ 'alt.constructor' => '_new_generic' ],
   [ new => '$$$$$' ],
   [ '$name' => '#1' ],
   [ '$pkg' => '#2' ],
   [ '$application' => '#3' ],
   [ '$extension' => '$Application::extension' ],
   [ '$super' => '#4' ],
   [ '$generic' => 'undef' ],
   [ '$params' => '#5' ],
   [ '&abstract' => '\&type' ],
   [ '&perform_typecheck' => 'undef' ],
);

####################################################################################
#
#  Constructor
#
#  new PropertyParamedType(generic PropertyParamedType, super PropertyType, [ template params ])
#
sub new {
   my $self=&_new;
   Struct::learn_package_retrieval($self, \&PropertyType::pkg);
   scan_params($self);
   unless ($self->abstract) {
      if (defined($self->super) && $self->super != $self->generic) {
         # methods defined for the own generic class have precedence over those from the super class
         my $generic_index=Struct::get_field_index(\&generic);
         foreach my $method (@override_methods) {
            if (defined($self->generic->$method)) {
               $self->$method=$generic_index;
            }
         }
      }
      my $self_sub=sub { $self };
      define_function($self->pkg, "type", $self_sub, 1);
      define_function($self->pkg, ".type", $self_sub);
      $self->dimension=$self->super->dimension;
      $self->operators=$self->super->operators;
      $self->construct_node=$self->super->construct_node;
      $self->create_method_new();
      establish_inheritance($self, $self->generic, $self->super);
      if (defined($self->generic->cppoptions)) {
         $self->application->cpp->add_template_instance($self, $self->generic, $nested_instantiation);
      }
      $self->init->();
   }
   $self;
}
####################################################################################
sub concrete_type {
   my ($self, $obj_proto)=@_;
   $self->abstract->($obj_proto->descend_to_generic($self->context_pkg) or
                     defined($self->context_pkg)
                     ? croak( "Parameterized type ", $self->full_name, " used in wrong context: ",
                              $obj_proto->full_name, " instead of ", $self->context_pkg->typeof_gen->full_name )
                     : croak( "Wrong use of pure abstract type ", $self->full_name ));
}

sub create_type_instance : method {
   my ($self, $arglist)=@_;
   $self->pkg->typeof(map { $_->abstract->($arglist) } @{$self->params});
}

sub context_sensitive_typecheck : method {
   my ($self, $args, $given)=@_;
   concrete_type($self, $args->[0]->type)->perform_typecheck->($args, $given);
}

sub context_sensitive_deducing_typecheck : method {
   my ($self, $args, $given, $backtrack)=@_;
   deducing_typecheck(concrete_type($self, $args->[0]->type), $args, $given, $backtrack);
}

sub deducing_typecheck : method {
   my ($self, $args, $given, $backtrack)=@_;
   if (defined (my $comparable=$given->descend_to_generic($self->pkg))) {
      my $i=0;
      foreach my $param (@{$self->params}) {
         $param->perform_typecheck->($args, $comparable->params->[$i++], $backtrack) or return 0;
      }
      $self
   }
}

sub performs_deduction {
   my ($self)=@_;
   $self->abstract && defined($self->perform_typecheck) && $self->perform_typecheck != \&context_sensitive_typecheck;
}

sub new_generic {
   my ($self_pkg, $name, $pkg, $app, $params, $super, $subpkg)=@_;
   my $self;
   my $n_defaults=shift @$params;
   my $min_params=@$params-$n_defaults;
   my $param_index=-1;
   my @param_holders=map { new ClassTypeParam($_, $pkg, $app, ++$param_index) } @$params;
   my $symtab=get_symtab($pkg);
   if (!defined($subpkg)) {
      $self=_new_generic($self_pkg, $name, $pkg, $app, $super, \@param_holders);
      if ($super) {
         $self->dimension=$super->dimension;
         $self->operators=$super->operators;
         if ($self->construct_node=$super->construct_node) {
            $self->create_method_new();
         }
         push @{$super->derived_abstract}, $self;
         weak($super->derived_abstract->[-1]);
      }
      $subpkg="props";
   }
   define_function($symtab, "_min_params", sub { $min_params });

   no strict 'refs';
   *{$app->pkg."::$subpkg\::$name\::"}=$symtab;
   $self // \@param_holders;
}

#################################################################################
sub mangle_param_names {
   @_==1 ? "__$_[0]" : "_A_" . join("_I_", @_) . "_Z";
}

sub mangle_paramed_type_name {
   my ($pkg, $params)=@_;
   $pkg . mangle_param_names(split /,/, $params)
}

sub scan_params {
   my ($self)=@_;
   my $deduction_involved;
   foreach (@{$self->params}) {
      if ($_->abstract) {
         $self->abstract=\&create_type_instance;
         $deduction_involved ||= $_->performs_deduction;
         if ($_->context_pkg) {
            if ($self->context_pkg) {
               $self->context_pkg eq $_->context_pkg
                 or croak( "invalid parameterized type: confilicting conext packages ", $self->context_pkg, " and ", $_->context_pkg );
            } else {
               $self->context_pkg=$_->context_pkg;
            }
         }
      } else {
         set_extension($self->extension, $_->extension);
      }
   }
   if ($self->abstract) {
      $self->perform_typecheck=
        $deduction_involved
        ? ($self->context_pkg ? \&context_sensitive_deducing_typecheck : \&deducing_typecheck)
        : \&context_sensitive_typecheck;
   } else {
      $self->context_pkg=$self->pkg;
      $self->pkg .= mangle_param_names(map { $_->mangled_name } @{$self->params});
   }
}
#################################################################################
sub find_super_type_param {
   my ($self, $name) = @_;
   my $result;
   while (defined ($self = $self->super)) {
      if (defined($self->params) and my ($param) = grep { $_->name eq $name } @{$self->params}) {
         $result = $param;
      }
   }
   $result;
}
#################################################################################
sub full_name {
   my ($self)=@_;
   if ($self->abstract ? !$self->performs_deduction : !defined($self->params)) {
      $self->name
   } else {
      $self->name . "<" . join(", ", map { $_->full_name } @{$self->params}) . ">"
   }
}

sub mangled_name {
   $_[0]->pkg =~ /::(\w+)$/;
   $1
}

# produce a name adhering to the XML name syntax rule, e.g. as a schema element identifier
sub xml_name {
   my ($self)=@_;
   join(".", $self->name, map { $_->xml_name } @{$self->params})
}

# produce a name sufficient for reconstruction from a data file
sub qualified_name {
   my ($self, $in_app) = @_;
   $in_app //= $self->application;
   &PropertyType::qualified_name . "<" . join(", ", map { $_->qualified_name($in_app) } @{$self->params}) . ">"
}

sub required_extensions {
   my ($self, $include_bundled) = @_;
   map { PropertyType::required_extensions($_, $include_bundled) } $self, @{$self->params}
}
#################################################################################
# Update the list of independent extensions in $_[0].
# Both arguments may be undef (equivalent to an empty list) or single Extension objects (equivalent to a one-element list)
sub set_extension {
   my ($curlist, $newlist)=@_;
   defined($newlist) or return;
   if (defined $curlist) {
      return if $curlist == $newlist;
      my @curlist= is_object($curlist) ? ($curlist) : @$curlist;
      my $last_cur=$#curlist;
      my $changed;
      foreach my $ext (is_object($newlist) ? $newlist : @$newlist) {
         my $add=1;
         for (my $i=$last_cur; $i>=0; --$i) {
            if ($curlist[$i] == $ext || list_index($curlist[$i]->requires, $ext)>=0) {
               # extension to be added is already contained in the current list or is among prerequisites of one of its members
               $add=0; last;
            }
            if (list_index($ext->requires, $curlist[$i])>=0) {
               # extension to be added superposes one of the current list members
               splice @curlist, $i, 1;
               --$last_cur;
            }
         }
         if ($add) {
            push @curlist, $ext;
            $changed=1;
         }
      }
      if ($changed) {
         $_[0]= @curlist>1 ? \@curlist : $curlist[0];
      }
   } else {
      $_[0]=is_object($newlist) ? $newlist : [ @$newlist ];
   }
}
#################################################################################
package Polymake::Core::PropertyTypeInstance;

# for pure C++ types 'derived' from an abstract perl base class

use Polymake::Struct (
   [ '@ISA' => 'PropertyType' ],
   [ new => '$$$;$' ],
   [ '$name' => '#1->name' ],
   [ '$pkg' => '#1->pkg' ],
   [ '$application' => '#1->application' ],
   [ '$super' => '#2 || #4' ],
   [ '$generic' => '#1' ],
   [ '$param' => '#3' ],
);

use Polymake::Struct (
   [ 'alt.constructor' => '_new_generic' ],
   [ new => '$$$;$' ],
   [ '$name' => '#1' ],
   [ '$pkg' => '#2' ],
   [ '$application' => '#3' ],
   [ '$extension' => '$Application::extension' ],
   [ '$super' => '#4' ],
   [ '$generic' => 'undef' ],
   [ '$param' => 'undef' ],
   [ '&abstract' => '\&type' ],
);

sub new {
   my $self=&_new;
   $self->param =~ s/[.\$<>]/_/g;
   $self->pkg.="::".$self->param;
   my $self_sub=sub { $self };
   define_function($self->pkg, "type", $self_sub, 1);
   define_function($self->pkg, ".type", $self_sub);
   establish_inheritance($self, $self->generic, $self->super);
   if (defined($self->generic->cppoptions)) {
      $self->application->cpp->add_template_instance($self, $self->generic, $nested_instantiation);
   }
   $self;
}

sub new_generic {
   my $self=&_new_generic;
   establish_inheritance($self, $self->super) if $self->super;
   $self;
}

sub full_name {
   my ($self)=@_;
   $self->name . "<" . $self->param . ">";
}

sub mangled_name {
   my ($self)=@_;
   $self->name . "__" . $self->param;
}

#################################################################################
package Polymake::Core::ClassTypeParam;

use Polymake::Struct (
   [ '@ISA' => 'PropertyType' ],
   [ '$super' => 'undef' ],
   [ '&abstract' => '\&extract_type' ],
   [ '&perform_typecheck' => '\&no_typecheck' ],
   [ '$pkg' => '"UNIVERSAL"' ],
   [ '$context_pkg' => '#2' ],
   [ '$type_param_index' => '#4' ],
);

*new=\&_new;

# the context owner object type can be either passed directly or as the leading argument in a function arglist
sub extract_type : method {
   (is_object($_[1]) ? $_[1] : $_[1]->[0])->params->[$_[0]->type_param_index]
}

sub typecheck : method {
   my ($self, $full_args, $args, $arg_index)=@_;
   &extract_type->isa->($args->[$arg_index]) && $self;
}

*concrete_type=\&PropertyParamedType::concrete_type;

sub full_name {
   my ($self)=@_;
   $self->context_pkg->typeof_gen->name . "::" . $self->name
}

sub help_topic { undef }

#################################################################################
package Polymake::Core::FunctionTypeParam;

use Polymake::Struct (
   [ '@ISA' => 'PropertyType' ],
   [ new => '$' ],
   [ '$name' => '"__PARAM__" . #1' ],
   [ '$pkg' => '"UNIVERSAL"' ],
   [ '$application' => 'undef' ],
   [ '$extension' => 'undef' ],
   [ '$super' => 'undef' ],
   [ '&abstract' => '\&extract_type' ],
   [ '&perform_typecheck' => '\&deduce_type' ],
   [ '$type_param_index' => '#1' ],
);

*new=\&_new;

sub extract_type : method {
   namespaces::fetch_explicit_typelist($_[1])->[$_[0]->type_param_index]
}

sub deduce_type : method {
   my ($self, $args, $given, $backtrack, $deduce_func)=@_;
   my ($typelist, $explicit_size)=namespaces::fetch_explicit_typelist($args);
   if (my $settled=$typelist->[$self->type_param_index]) {
      # if already deduced rather than explicitly specified, may be reduced to the common least derived type or updgraded
      $deduce_func //= \&deduce_least_derived;
      if (my $deduced=$deduce_func->($settled, $given)) {
         if ($settled != $deduced) {
            return if $self->type_param_index < $explicit_size;
            $typelist->[$self->type_param_index]=$deduced;
            push @$backtrack, $self->type_param_index, $settled, \&Overload::restore_type_param;
         }
         $self
      }
   } else {
      $typelist->[$self->type_param_index]=$given;
      push @$backtrack, $self->type_param_index, undef, \&Overload::restore_type_param;
      $self
   }
}

sub performs_deduction { 1 }

# For RuleFilter:
# the instances are shared among all overloaded functions
declare @instances=(new(__PACKAGE__, undef));

# The 0th parameter is the anonymous placeholder.
# It does not deduce anything, but simply accepts any argument.

$instances[0]->name="__ANON__";
$instances[0]->perform_typecheck=sub { $_[1] };

sub create {
   my ($self_pkg, $num)=@_;
   for my $index (@instances..$num) {
      $instances[$index]=new($self_pkg, $index-1);
   }
}

#################################################################################
package Polymake::Core::PropertyType::Upgrade;

use Polymake::Struct (
   [ '@ISA' => 'PropertyParamedType' ],
   [ new => '$' ],
   [ '$name' => '"type_upgrade"' ],
   [ '$pkg' => '"UNIVERSAL"' ],
   [ '$super' => 'undef' ],
   [ '$generic' => 'undef' ],
   [ '$params' => '[ #1 ]' ],
   [ '$context_pkg' => 'undef' ],
   [ '&abstract' => '\&type' ],
   [ '&perform_typecheck' => '\&check_and_upgrade' ],
);

*new=\&_new;

sub typeof {
   if (@_==2) {
      # requested an upgrading type deduction from a function argument

      instanceof FunctionTypeParam($_[1])
        or croak( "type_upgrade is only applicable to function type parameters" );
      state %inst_cache;
      $inst_cache{ $_[1] } //= &new;

   } elsif (@_==3) {
      # final typecheck clause

      if (!is_readonly($_[1]) and
          !defined($_[1]) || deduce_most_enhanced($_[1], $_[2]) == $_[2]) {
         # no value deduced so far or upgrade successful
         $_[1]=$_[2];
      } else {
         # the parameter has been specified explicitly, or upgrade failed: no changes
         $_[1]
      }
   } else {
      croak( "type_upgrade requires one or two type parameters" );
   }
}

sub check_and_upgrade : method {
   my $self=shift;
   $self->params->[0]->deduce_type(@_, \&deduce_most_enhanced) && $self
}

sub type_param_index : method { $_[0]->params->[0]->type_param_index }

#################################################################################
package Polymake::Core::PropertyType::UpgradesTo;

use Polymake::Struct (
   [ '@ISA' => 'PropertyParamedType' ],
   [ new => '$' ],
   [ '$name' => '"type_upgrades_to"' ],
   [ '$pkg' => '"UNIVERSAL"' ],
   [ '$super' => 'undef' ],
   [ '$generic' => 'undef' ],
   [ '$params' => '[ #1 ]' ],
   [ '$context_pkg' => 'undef' ],
   [ '&perform_typecheck' => '\&check_upgradable' ],
);

sub new {
   my $self=&_new;
   if ($self->params->[0]->abstract) {
      $self->abstract = sub : method { $_[0]->params->[0]->abstract->($_[1]) };
      $self->context_pkg = $self->params->[0]->context_pkg;
   } else {
      $self->abstract = sub : method { $_[0]->params->[0] };
   }
   $self;
}

sub typeof {
   @_==2 or croak( "type_upgrades_to requires exactly one type parameter" );
   state %inst_cache;
   $inst_cache{ $_[1] } //= &new;
}

sub check_upgradable : method {
   my ($self, $args, $given)=@_;
   if (defined (my $expected=$self->abstract->($args))) {
      if (deduce_most_enhanced($expected, $given)==$expected) {
         return $self;
      }
   }
   undef
}

sub type_param_index : method { $_[0]->params->[0]->type_param_index }

#################################################################################
package Polymake::Core::PropertyType::ConvertTo;

use Polymake::Struct (
   [ '@ISA' => 'UpgradesTo' ],
   [ new => '$' ],
   [ '$name' => '"can_convert_to"' ],
   [ '&perform_typecheck' => '\&no_typecheck' ],
);

sub typeof {
   @_==2 or croak( "can_convert_to requires exactly one type parameter" );
   state %inst_cache;
   $inst_cache{ $_[1] } //= &new;
}

sub typecheck : method {
   my ($self, $full_args, $args, $arg_index, $backtrack)=@_;
   if (defined (my $target_proto=$self->abstract->($full_args))) {
      my $given=$args->[$arg_index];
      if ($target_proto->isa->($given)) {
         return $self;
      }
      if (defined (my $converted=eval { $target_proto->construct->($given) })) {
         push @$backtrack, sub { $args->[$arg_index]=$given };
         $args->[$arg_index]=$converted;
         return $self;
      }
   }
   undef
}

#################################################################################
package main;

# fallback for normal packages without prototype objects
sub UNIVERSAL::typeof_gen { @_==1 && $_[0] }

*type_upgrades_to::=\%Polymake::Core::PropertyType::UpgradesTo::;
*type_upgrade::=\%Polymake::Core::PropertyType::Upgrade::;
*can_convert_to::=\%Polymake::Core::PropertyType::ConvertTo::;

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
