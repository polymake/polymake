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

package Polymake::Core::PropertyType;

declare @delimiters;
declare $nesting_level;
declare $trusted_value;

sub parse_fallback : method {
   my $p=pos;
   if (substr($_,$p) =~ $_[1]) {
      pos=$p+$+[0];
      substr($_,$p,$-[0]);
   } else {
      &parse_error;
   }
}
sub construct_complain : method {
   shift;
   croak( "Don't know how to construct an object of type ", original_object()->full_name,
          " from given arguments (", join(", ", map { ref($_) || "'$_'" } @_), ")" );
}

sub canonical_fallback { }
sub equal_fallback { $_[0] == $_[1] }
sub isa_fallback : method { UNIVERSAL::isa($_[1], $_[0]->pkg) }
sub toString_fallback { "$_[0]" }
sub toXML_fallback {
   my ($value, $writer, @attrs)=@_;
   $writer->dataElement("e", "$value", @attrs);
}
sub init_fallback { }
sub type : method { shift }
*prototype=\&type;
*resolve_abstract=\&type;

use Polymake::Struct (
   [ new => '$$$;$@' ],
   [ '$name | full_name | generic_name | mangled_name' => '#1' ],
   [ '$pkg' => '#2' ],
   [ '$application' => '#3' ],
   [ '$extension' => '$Application::extension' ],
   [ '$super' => '#4' ],
   [ '$dimension' => '0' ],
   [ '&parse'    => '->super || \&parse_fallback' ],            # delimiter_re => object; text in $_
   [ '&canonical' => '->super || \&canonical_fallback' ],       # object => void
   [ '&equal'  => '->super || \&equal_fallback' ],              # object1, object2 => bool
   [ '&isa' => '->super || \&isa_fallback' ],                   # object => bool
   [ '&toString' => '->super || \&toString_fallback' ],         # object => "string"
   [ '&toXML' => '->super || \&toXML_fallback' ],               # object, writer =>
   [ '&construct' => '->super || \&construct_complain' ],       # args ... => object
   [ '&parse_string' => '\&parse_whole' ],
   '&abstract',
   [ '$context_pkg' => 'undef' ],
   [ '$cppoptions' => 'undef' ],
   [ '&init' => '\&init_fallback' ],
   [ '$help' => 'undef' ],      # InteractiveHelp (in interactive mode, when comments are supplied for user methods)
);

my @override_methods=qw( init parse parse_string canonical equal isa toString toXML construct );

declare @string_ops=map { $_ => eval <<"." } qw( . cmp eq ne lt le gt ge );
sub { \$_[2] ? "\$_[1]" $_ "\$_[0]" : "\$_[0]" $_ "\$_[1]" }
.

####################################################################################
#
#  Constructor
#
#  new PropertyType('name', 'package', Application, super PropertyType, [ template params ])
#
sub new {
   my $self=&_new;
   my $pkg=$self->pkg;
   if ($self->super) {
      $self->dimension=$self->super->dimension;
   }
   my $proto_sub=sub { $self };

   if (ref($_[4]) eq "ARRAY") {
      $self->extension=$pkg->self->extension;
      PropertyParamedType::init_params($self,$_[4]);
      return $self if $self->abstract;
      my $set=store_methods($self, $pkg, @override_methods);
      if ($self->init != \&init_fallback) {
         $self->init->();
      } else {
         define_basic_operators($self) if $set->{equal} || $set->{toString};
      }
   } else {
      no strict 'refs';
      *{$self->application->pkg."::props::".$self->name."::"}=get_pkg($pkg,1);
      define_function($self->pkg, "typeof", $proto_sub, 1);
   }
   define_function($self->pkg, "type", $proto_sub, 1);
   define_function($self->pkg, "prototype", $proto_sub);
   define_function($self->pkg, "self",  # for the rule parser
                   sub {
                      shift or croak( "This declaration is only allowed in the top-level (application) scope" );
                      $self
                   }, 1);

   if ($self->pkg eq $pkg) {
      # usual (not parameterized) type
      define_function($self->pkg, "new", sub { shift; new_object($self,@_) });
   } else {
      # non-abstract template instance
      my @isa=($pkg);
      if ($self->super) {
         push @isa, $self->super->pkg;
      }
      namespaces::using($self->pkg, @isa);
      no strict 'refs';
      @{$self->pkg."::ISA"}=@isa;
   }

   $self;
}

# for types with partially deferred declarations (e.g. embedded in clients)
sub add_super {
   my ($self, $super)=@_;
   $self->super=$super;
   $self->dimension=$self->super->dimension;
   namespaces::using($self->pkg, $super->pkg);
   no strict 'refs';
   push @{$self->pkg."::ISA"}, $super->pkg;
}

sub covering_app { $_[0]->application }

##################################################################################
sub store_methods {
   my $self=shift;
   my $symtab=get_pkg(shift);
   my $set= defined(wantarray) ? { } : undef;
   foreach (@_) {
      if (exists &{$symtab->{$_}}) {
         $self->$_=my $method=\&{$symtab->{$_}};
         Struct::pass_original_object($method) if $_ eq "construct";
         $set->{$_}=1 if defined($set);
      }
   }
   $set
}
##################################################################################
sub parse_whole : method {
   (my $self, local $_)=@_;
   my $obj;
   if ($self->cppoptions && !$self->cppoptions->builtin) {
      $obj=eval { $self->parse->($_, $trusted_value) };
      if (!defined($obj)) {
         if ($@ =~ /^(\d+)\t/) {
            pos($_)=$1;
            &parse_error;
         } else {
            die $@;
         }
      }
   } else {
      pos($_)=0;
      $obj=$self->parse->(qr/\Z/m);
      if (!$trusted_value && defined($self->canonical)) {
         $self->canonical->($obj);
      }
   }
   $obj;
}

Struct::pass_original_object(\&parse_whole);

sub parse_error {
   my $start=pos;
   die "invalid ", $_[0]->full_name, " value starting at \"",
       length($_)>$start+30 ? substr($_,$start,30)."..." : substr($_,$start),
       "\"\n";
}
##################################################################################
sub define_basic_operators {
   my ($self)=@_;
   overload::OVERLOAD($self->pkg, fallback => 0,
                      '""' => sub { $self->toString->(@_) },
                      '==' => sub { $self->equal->(@_) },
                      '!=' => sub { !$self->equal->(@_) },
                      @string_ops,
                     );
}
##################################################################################
# PropertyType, arg, ... =>
sub new_object {
   my $self=shift;
   if (@_==1 && is_string($_[0])) {
      $self->parse_string->(@_);
   } else {
      $self->construct->(@_);
   }
}
##################################################################################
sub trivialArray_toXML {
   my ($elem_proto, $value, $writer, @attr)=@_;
   if (@$value) {
      if (defined ($elem_proto->toString)) {
         $writer->dataElement("v", join(" ", map { $elem_proto->toString->($_) } @$value), @attr);
      } else {
         $writer->dataElement("v", "@$value", @attr);
      }
   } else {
      $writer->emptyTag("v",@attr);
   }
}

sub nontrivialArray_toXML {
   my ($elem_proto, $value, $writer, @attr)=@_;
   my $tag= $elem_proto->dimension ? "m" : "v";
   if (@$value) {
      $writer->startTag($tag, @attr);
      foreach my $elem (@$value) {
         $elem_proto->toXML->($elem,$writer);
      }
      $writer->endTag($tag);
   } else {
      $writer->emptyTag($tag, @attr);
   }
}

sub sparseArray_toXML {
   my ($elem_proto, $value, $writer, @attr)=@_;
   push @attr, dim => $value->dim unless $writer->{"!dim"};
   $writer->startTag("v",@attr);
   $writer->setDataMode(0);
   for (my $it=args::entire($value); $it; ++$it) {
      $writer->characters(" ");
      $elem_proto->toXML->($it->deref, $writer, i => $it->index);
   }
   $writer->characters(" ");
   $writer->setDataMode(1);
   $writer->endTag("v");
}

sub sparseMatrix_toXML {
   my ($elem_proto, $value, $writer)=@_;
   if ($value->rows) {
      local $writer->{"!dim"}=1;
      nontrivialArray_toXML(@_,  cols => $value->cols);
   } else {
      $writer->emptyTag("m",@_[3..$#_]);
   }
}

sub trivialComposite_toXML {
   my ($value, $writer, @attr)=@_;
   $writer->dataElement("t", "@$value", @attr);
}

sub nontrivialComposite_toXML {
   my ($elem_protos, $value, $writer, @attr)=@_;
   $writer->startTag("t",@attr);
   my $i=0;
   foreach my $elem (@$value) {
      $elem_protos->[$i++]->toXML->($elem,$writer);
   }
   $writer->endTag("t");
}

sub assocContainer_toXML {
   my ($pair_proto, $value, $writer, @attr)=@_;
   if (keys %$value) {
      $writer->startTag("m",@attr);
      while (my ($k, $v)=each %$value) {
         $pair_proto->toXML->([$k,$v],$writer);
      }
      $writer->endTag("m");
   } else {
      $writer->emptyTag("m");
   }
}
#################################################################################
sub locate_own_help_topic {
   my ($self, $whence, $force)=@_;
   my $full_name=$self->full_name;
   my $topic=$self->application->help->find($whence, $full_name);
   if (!defined($topic)) {
      if ($self->name ne $full_name) {
         # help node of the generic type
         $topic=$self->application->help->find($whence, $self->name);
      }
      if ($force) {
         # hang the specialized topic as a sibling of the generic one
         $topic= defined($topic) ? $topic->parent->add([$full_name])
                                 : $self->application->help->add([$whence, $full_name]);
      } else {
         return $topic;
      }
   }
   push @{$topic->related},
        uniq( map { ($_, @{$_->related}) } grep { defined and $_ != $topic } map { $_->help_topic }
              $whence eq "property_types" ? defined($self->super) ? $self->super : () : @{$self->super} );
   $self->help=$topic;
}

sub help_topic {
   my $self=shift;
   $self->help || locate_own_help_topic($self, "property_types", @_);
}
#################################################################################
package _::Analyzer;

sub DESTROY {
   my $proto=${(shift)};
   store_methods($proto, $proto->pkg, @override_methods)
}
#################################################################################
package Polymake::Core::HasType4Deduction;

sub type4deduction { shift }

package Polymake::Core::PropertyParamedType;

use Polymake::Struct (
   [ '@ISA' => 'PropertyType', 'HasType4Deduction' ],
   [ '$param' => 'undef' ],
);

sub descend_to_generic {
   my ($self, $pkg)=@_;
   do {
      return $self if $self->context_pkg eq $pkg || $self->pkg eq $pkg;
   } while ($self=$self->super);
   undef
}

sub concrete_type {
   my ($self, $obj_proto)=@_;
   $self->abstract->($obj_proto->descend_to_generic($self->context_pkg) ||
                     croak( $obj_proto->full_name, " is not the parameterization context of the abstract type ", $self->full_name));
}

sub resolve_abstract {
   my $self=shift;
   if ($self->abstract) {
      if (defined ($self->context_pkg)) {
         if (my ($obj,$kind)=caller_object($self->context_pkg, "Polymake::Core::HasType4Deduction")) {
            $self->concrete_type($kind ? $obj->type4deduction : $obj->type);
         } else {
            croak( "Can't determine the concrete type for ", $proto->full_name, " from the context" );
         }
      } else {
         $self->abstract->();
      }
   } else {
      $self;
   }
}

sub covering_app {
   my $self=shift;
   if (is_object($self->param)) {
      $self->application->common($self->param->covering_app);
   } else {
      my $app=$self->application;
      foreach my $param (@{$self->param}) {
         $app=$app->common($param->covering_app) or last;
      }
      $app;
   }
}

sub new_abstract {
   my $param_index=pop @_;
   my $self=&_new;
   $self->param=$param_index;
   $self->abstract= $param_index>=0 ? sub : method { $_[1]->param->[$_[0]->param] } : sub { $_[0]->param };
   $self->context_pkg=$self->pkg;
   $self->pkg .= "::".$self->name;
   define_function($self->pkg, "new",
                   sub {
                      if (my ($obj,$kind)=caller_object($self->context_pkg, "Polymake::Core::HasType4Deduction")) {
                         shift;
                         new_object($self->abstract->($kind ? $obj->type4deduction : $obj->type),@_);
                      } else {
                         croak( "Can't determine the concrete type for ", $self->name, " from the context" );
                      }
                   }, 1);
   define_function($self->pkg, "type", sub { $self });
   $self;
}

sub get_typeof {
   if ($#_>0 && instanceof namespaces::TemplateExpression($_[1])) {
      shift;   # drop the package
      eval_type_expr(shift)->resolve_abstract;
   } else {
      # either an object with a ready-to-use concrete type, or a naked package name supposing default parameters
      (shift)->type->resolve_abstract;
   }
}

sub check_instanceof : method {
   (undef, my ($expr, $obj))=@_;
   UNIVERSAL::isa($obj, eval_type_expr($expr)->pkg);
}

sub new_generic {
   my ($self_pkg, $name, $pkg, $app, $params, $subpkg)=@_;
   my $n_defaults=shift @$params;
   my $min_params=@$params-$n_defaults;
   my $param_index=@$params>1 ? 0 : -1;
   foreach my $param_name (@$params) {
      my $type_proto=new_abstract PropertyParamedType($param_name, $pkg, $app, $param_index);
      ++$param_index;
   }
   get_pkg($name,1);                    # enforce a top-level package to trick out the perl parser
   get_pkg($app->name."::$name",1);     # and the same for qualifications via application name
   my $symtab=get_pkg($pkg);
   define_function($symtab, "new", sub { new_object(&get_typeof,@_) });
   if (!defined($subpkg)) {
      my $self=_new($self_pkg, $name, $pkg, $app);
      define_function($symtab, "self",  # for the rule parser
                      sub {
                         shift or croak( "This declaration is only allowed in the top-level (application) scope" );
                         $self
                      });
      $subpkg="props";
   }
   define_function($symtab, "instanceof", \&check_instanceof);
   define_function($symtab, "typeof", \&get_typeof);
   define_function($symtab, "_min_params", sub { $min_params });
   
   no strict 'refs';
   *{$app->pkg."::$subpkg\::$name\::"}=$symtab;
}

#################################################################################
sub init_params {
   my ($self, $params)=@_;
   if (@$params==1) {
      $self->param=$params->[0];
      if ($self->param->abstract) {
         $self->abstract=sub : method {
            my ($self, $obj_proto)=@_;
            $self->pkg->generic_type($self->param->abstract->($obj_proto));
         };
         $self->context_pkg=$self->param->context_pkg;
      } else {
         $self->context_pkg=$self->pkg;
         $self->pkg.="__" . $self->param->mangled_name;
         set_application($self, $self->param->application);
         set_extension($self->extension, $self->param->extension);
      }
   } else {
      $self->param=$params;
      foreach (@$params) {
         if ($_->abstract) {
            $self->abstract=sub : method {
               my ($self, $obj_proto)=@_;
               $self->pkg->generic_type(map { $_->abstract ? $_->abstract->($obj_proto) : $_ } @{$self->param});
            };
            $self->context_pkg=$_->context_pkg;
            last;
         } else {
            set_application($self, $_->application);
            set_extension($self->extension, $_->extension);
         }
      }
      if (! $self->abstract) {
         $self->context_pkg=$self->pkg;
         $self->pkg.="_A_" . join("_I_", map { $_->mangled_name } @$params) . "_Z";
      }
   }
}
#################################################################################
sub set_field_names {
   my $self=shift;
   if ($#_ != $#{$self->param}) {
      croak( "mismatch between type parameters and field names" );
   }
   my $symtab=get_pkg($self->pkg);
   my $i=0;
   foreach (@_) {
      define_function($symtab, $_, Struct::create_accessor($i, \&Struct::access_field));
      my $param=$self->param->[$i];
      if (my $set_filter=
          defined($param->construct)
          ? sub {
               my $arg=shift;
               if (is_object($arg) && UNIVERSAL::isa($arg, $param->pkg)) {
                  $arg
               } else {
                  new_object($param,$arg);
               }
            }
          : defined($param->canonical) &&
            sub {
               my $arg=shift;
               $param->canonical->($arg);
               $arg
            }
         ) {
         ${$symtab->{$_}}=$set_filter;
      }
      ++$i;
   }
}
#################################################################################
# self: abstract XxxType with placeholders;  proto: concrete XxxType
sub match_type : method {
   my ($self, $proto)=@_;
   if (is_object($self->param)) {
      $proto=$proto->descend_to_generic($self->pkg) or die "no match\n";
      match_type($self->param, $proto->param);

   } elsif (is_ARRAY($self->param)) {
      $proto=$proto->descend_to_generic($self->pkg) or die "no match\n";
      my $i=-1;
      map { ++$i; $_->abstract ? match_type($_, $proto->param->[$i]) : () } @{$self->param};

   } else {
      [ $self, $proto ];
   }
}
#################################################################################
sub full_name {
   my $self=$_[0];
   if (is_object($self->param)) {
      $self->name . "<" . $self->param->full_name . ">"
   } elsif (is_ARRAY($self->param)) {
      $self->name . "<" . join(", ", map { $_->full_name } @{$self->param}) . ">"
   } else {
      $self->name
   }
}

sub mangled_name {
   $_[0]->pkg =~ /::(\w+)$/;
   $1
}
#################################################################################
sub prepare_rebind {
   my ($pkg, $generic_type_sub, $super_type, @param_names)=@_;
   my $type_eval_pkg="Polymake::Core::PropertyParamedType::_type_eval";
   my $type_eval_symtab=get_pkg($type_eval_pkg,1);
   namespaces::using($type_eval_pkg,$pkg);

   my $tpcount=0;
   my @placeholders=map {
      my $placeholder=_new PropertyParamedType($_,undef,undef);
      $placeholder->cppoptions=new Overload::TemplateParam("T$tpcount", $tpcount, undef);
      ++$tpcount;
      $placeholder->abstract=sub { 1 };
      define_function($type_eval_symtab, $_, sub { $placeholder->cppoptions->deduced=1; $placeholder });
      $placeholder;
   } @param_names;
   --$tpcount;
   translate_type($super_type);
   my $super_proto=eval "package $type_eval_pkg; $super_type";
   my @deduce_from_original= map { $_->cppoptions->deduced ? () : $_->cppoptions->index } @placeholders;

   Symbol::delete_package($type_eval_pkg);

   sub {
      my ($object, $src_proto)=@_;
      my @params;  $#params=$tpcount;
      foreach ($super_proto->match_type($src_proto)) {
         $params[$_->[0]->cppoptions->index]=$_->[1];
      }
      if (@deduce_from_original) {
         my $obj_proto=$object->type;
         $params[$_]=$obj_proto->param->[$_] for @deduce_from_original;
      }
      $generic_type_sub->(@params)
   }
}
#################################################################################
sub set_application {
   my ($self, $new_app)=@_;
   if (defined (my $app=$self->application)) {
      if ($new_app != $app) {
         $self->application=$app->common($new_app)
                            || croak( $self->complain_source_conflict, ": conflicting applications ", $app->name, " and ", $new_app->name );
      }
   } else {
      $self->application=$new_app;
   }
}

sub complain_source_conflict {
   "Don't know where to place the instantiation of ", $_[0]->full_name
}

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
package Polymake::Core::PropertyTypePlaceholder;
use Polymake::Struct (
   [ '@ISA' => 'PropertyParamedType' ],
   '@locals',
);

sub _undef { croak( "undefined template parameter ", $_[0]=~/($id_re)$/o ) }

sub new {
   my $self=&_new;
   $self->abstract=sub : method { $_[0]->param };
   $self->pkg .= "::".$self->name;
   get_pkg($self->name,1);
   define_function($self->pkg,"new",\&_undef,3);
   define_function($self->pkg,"type",\&_undef,3);
   define_function($self->pkg,"typeof",\&_undef,3);
   no strict 'refs';
   @{$self->locals}=( \*{$self->pkg."::new"}, sub { shift; new_object($self->param,@_); },
                      \*{$self->pkg."::type"}, sub { $self },
                      \*{$self->pkg."::typeof"}, sub { $self->param },
                    );
   $self
}

sub resolve_abstract { (shift)->param }

sub full_name { my $self=shift; $self->name."=".(defined($self->param) ? $self->param->full_name : "UNDEF") }

#################################################################################
package Polymake::Core::PropertyTypeInstance;

# for pure C++ types 'derived' from an abstract perl base class

use Polymake::Struct (
   [ '@ISA' => 'PropertyType' ],
   [ new => '$$$$$;$' ],
   [ '$super' => '#4 || #6' ],
   [ '$param' => '#5' ],
);

sub new {
   my $self=&_new;
   my $generic_pkg=$self->pkg;
   $self->param =~ s/[.\$<>]/_/g;
   $self->pkg.="::".$self->param;
   $self->mangled_name=$self->name . "__" . $self->param;
   define_function($self->pkg, "prototype", define_function($self->pkg, "type", sub { $self }, 1));

   my @isa=($generic_pkg);
   if ($self->super) {
      push @isa, $self->super->pkg;
   }
   namespaces::using($self->pkg, @isa);
   no strict 'refs';
   @{$self->pkg."::ISA"}=@isa;
   $self;
}

sub full_name {
   my ($self)=@_;
   $self->name . "<" . $self->param . ">";
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
