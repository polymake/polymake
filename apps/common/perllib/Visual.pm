#  Copyright (c) 1997-2021
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


package Polymake::common;

sub prepare_visual_objects {
   my @args=@_;
   for (my $i=0; $i<=$#args; ++$i) {
      if (instanceof Visual::Container($args[$i])) {
         $args[$i]->propagate_defaults;
         splice @args, $i, 1, @{$args[$i]->elements};
         redo;
      }
   }
   @args;
}

#
#  the central dispatching function
#
sub visualize($) {
   my $first=shift;
   return $first if defined(wantarray) or $INC{"Polymake/Test.pm"} and $Polymake::Test::disable_viewers;
   my @args=prepare_visual_objects($first);

   my $title=$first->Title // $first->Name;
   my ($method, $obj, $viewer);

   if (@args==1) {
      eval {
         if (is_object($args[0])) {
            # a single object to draw
            $method=Overload::Global::draw($args[0]);
            $viewer=method_owner($method)->new;
            $method->($viewer->new_drawing($title, $first), $args[0]);
         } else {
            return undef if !@{$args[0]};
            # should be a homogeneous array of drawables - resolve only once
            $method=Overload::Global::draw($args[0]->[0]);
            $viewer=method_owner($method)->new;
            my $drawing=$viewer->new_drawing($title, $first);
            foreach $obj (@{$args[0]}) {
               $method->($drawing, $obj, undef);
            }
         }
      };
      if ($@) {
         if ($@ =~ /no matching overloaded instance of Polymake::Overload::Global::draw/) {
            die "do not know how to visualize ", ref($args[0]) eq "ARRAY" ? ref($args[0]->[0]) : ref($args[0]),
                "\nprobably you should install some missing visualization packages\n";
         }
         if ($@ =~ /Undefined subroutine &Polymake::Overload::Global::draw/) {
            die "cannot visualize anything: no visualization packages installed\n";
         }
         die $@;
      }

   } else {
      # several drawable objects:
      # first obtain all viable visualizers, even if not preferred
      $obj=$args[0];
      my ($viewer_pkg, @methods);
      if (defined (my $control_list=Overload::resolve_global("draw",
                      [ is_object($obj) ? $obj : $obj->[0], undef ]))) {
         # must try all visualizers in the preference order until find such a one
         # that can cope with all drawable objects
       TRY:
         foreach my $method (@{$control_list->items}) {
            @methods=($method);
            $viewer_pkg=method_owner($method);
            foreach $obj (@args[1..$#args]) {
               if (my $method_next=Overload::resolve_method($viewer_pkg, "draw", is_object($obj) ? $obj : $obj->[0], undef)) {
                  push @methods, $method_next;
               } else {
                  @methods=();
                  next TRY;
               }
            }
            last;
         }
      }
      if (@methods) {
         $viewer=$viewer_pkg->new;
         my $drawing=$viewer->new_drawing($title, $first);
         foreach (@args) {
            $method=shift @methods;
            if (is_object($_)) {
               $method->($drawing, $_, undef);

            } else {
               foreach my $elem (@$_) {
                  $method->($drawing, $elem, undef);
               }
            }
         }
      } else {
         croak( "do not know how to visualize (",
                join(", ", map { ref($_) eq "ARRAY" ? "ARRAY<".ref($_->[0]).">" : ref($_) || $_ } @args), ") together" );
      }
   }

   $viewer->run;
}

############################################################################

#  The common part of explicit visualization functions
#  [ Visual::Object ... ], { options }, "Package" =>

sub visualize_explicit {
   my ($vis_objects, $opts, $Package)=@_;
   my $to_file=$opts->{File};
   my $viewer=do {
      if (defined $to_file) {
         my $file_package="$Package\::File::Writer";
         if (!ref($to_file) && $to_file eq "AUTO") {
            Visual::FileWriter::Auto->new($file_package);
         } else {
            if (@$vis_objects > 1 && ! $file_package->multiple) {
               my $caller_sub=(caller(1))[3];
               $caller_sub=~s/.*::([^:]+)$/$1/;
               $caller_sub=~s/^__(\w+)__OV__.*/$1/;
               die << ".";
The file format for $Package does not support multiple independent scenes.
If you really want to create several pictures, you should either specify
File => "AUTO", or call $caller_sub with each object (and different file names)
separately.

If you intended to put the objects together in one drawing instead,
bundle them with compose() like this:  $caller_sub(compose(VISUAL1, VISUAL2, ...))
.
            }
            if (!ref($to_file)) {
               replace_special_paths($to_file);
               if ($to_file !~ /^[-&|]/ && $to_file !~ /\.\w+$/) {
                  $to_file.=$file_package->file_suffix;
               }
            }
            $file_package->new($to_file);
         }
      } else {
         "$Package\::Viewer"->new;
      }
   };

   foreach my $vis_obj (@$vis_objects) {
      my $drawing=$viewer->new_drawing($vis_obj->Title // $vis_obj->Name, $vis_obj);
      foreach (prepare_visual_objects($vis_obj)) {
         if (is_object($_)) {
            $drawing->draw($_);
         } elsif (@$_) {
            my $method=Overload::resolve_method($drawing, "draw", $_->[0], undef)
              or die "no matching method ", ref($drawing), "::draw(", ref($_->[0]), ")\n";
            foreach my $elem (@$_) {
               $method->($drawing,$elem);
            }
         }
      }
   }

   if (defined(wantarray) && !defined($to_file)) {
      return $viewer;
   }

   $viewer->run
      unless $INC{"Polymake/Test.pm"} and $Polymake::Test::disable_viewers;

   if (defined $to_file) {
      $viewer->file
   } else {
      () # empty return
   }
}


############################################################################
package Visual;

# a few basic colors from the X11 color names (rgb.txt)
# plus some used directly in the code or the testsuite
my %backupcolors = (
      black    => "000000",
      white    => "ffffff",
      gray     => "bebebe",
      red      => "ff0000",
      green    => "00ff00",
      blue     => "0000ff",
      yellow   => "ffff00",
      magenta  => "ff00ff",
      cyan     => "00ffff",
      pink     => "ffc0cb",
      purple   => "a020f0",
      orange   => "ffa500",
      lavenderblush  => "fff0f5",
      indianred      => "cd5c5c",
      darkolivegreen => "556b2f",
      midnightblue   => "191970",
      lightslategrey => "778899",
      azure          => "f0ffff",
      lightgreen     => "90ee90",
      plum1          => "ffbbff",
      salmon1        => "ff8c69",
      chocolate1     => "ff7f24",
);

my %X11colornames;

sub loadX11names {
   %X11colornames or eval {
      require Graphics::ColorNames;
      tie %X11colornames, 'Graphics::ColorNames', (qw( X ));
   };
}

sub get_sym_color {
   if (defined (my $c=$Visual::Color::symbolicnames{$_[0]})) {
      return new RGB($c);
   }
   my $colorname = lc($_[0]);
   loadX11names;
   if (defined (my $c = $X11colornames{$colorname} // $backupcolors{$colorname})) {
      return new RGB("#".$c);
   }
   warn "Only basic color names available, check TAB-completion for a list.\n",
        "To use the full set of X11 color names please install the perl module Graphics::ColorNames.\n",
        "You can also define custom names using the custom variable \%Visual::Color::symbolicnames.\n"
      unless %X11colornames;

   undef;
}

sub list_color_completions {
   my $prefix=lc shift;
   loadX11names;
   grep { lc =~ /^\Q$prefix\E/ } (keys %Visual::Color::symbolicnames, %X11colornames ? keys %X11colornames : keys %backupcolors );
}

sub get_RGB {
   my $c=pop;
   instanceof RGB($c) ? $c : new RGB($c)
}

declare $hidden_re=qr{\bhidden\b};

# restrict the precision by 6 significant digits
sub print_coords {
   join($_[1] || " ", map { abs($_)>=1e-5 ? sprintf("%.6g", $_) : "0" } @{$_[0]})
}

sub print3dcoords {
   my $c=$_[1] || " ";
   sprintf "%.6g$c%.6g$c%.6g", @{$_[0]}, 0;
}

sub transform_float {
    my ($MM,$T,$v) = @_; # Matrix<Float> (dehomogenized), Matrix<Float>, Vector<Float>
    if (defined($T)) {
        die "transformation invalid" unless $MM->cols()==$T->rows() && $T->cols()<=3;
        $MM = $MM*$T;
    }
    if (defined($v)) {
        die "offset invalid" unless $MM->cols()==$v->dim() && $v->dim()<=3;
        for (my $i=0; $i<$MM->rows(); ++$i) { $MM->[$i] = $MM->[$i]+$v }
    }
    return $MM;
}

sub transform_float_facets {
    my ($FF,$T,$v) = @_; # Matrix<Float>, Matrix<Float>, Vector<Float>
    my $vv = defined($v) ? new Vector<Float>(1|$v) : unit_vector<Float>($FF->cols(),0);
    my $TT = defined($T) ? $vv/(zero_vector<Float>()|$T) : unit_matrix<Float>($FF->cols());
    $FF = $FF*transpose(inv($TT));
    return $FF;
}

sub get_code_decor_keys {
   # only considers keys that contain the name
   my ($decor,$name) = @_;
   my @codekeys;
   foreach my $key (keys %$decor) {
      my $deco = $decor->{$key};
      push @codekeys, $key if ($key =~ $name && (is_code($deco) || is_like_array($deco) || is_like_hash($deco)));
   }
   return @codekeys;
}

sub decor_subset {
   # TODO: this only works for vertex decor and NOT for vertexlabels that are separated by whitespaces, 
   # this should be somehow combined with subs like "unify_labels" and "unify_decor" that are called later (when the Visual::Object is
   # instantiated). Unification of decoration would be more useful before VISUAL* methods process it further. 
   
   # no need to be called if there is no vertex code decor
   my ($decor,$subset,$codedecor) = @_;
   my $d = { %$decor };
   foreach my $key (@$codedecor) {
      my $deco = $decor->{$key};
      if (is_code($deco)) {
         my @array = map { $deco->($_) } @$subset;
         $d->{$key} = sub { $array[$_[0]] };
      } elsif (is_like_array($deco)) {
         my @array = map { $deco->[$_] } @$subset;
         $d->{$key} = sub { $array[$_[0]] };
      } elsif (is_like_hash($deco)) {
         my @array = map { $deco->{$_} } @$subset;
         $d->{$key} = sub { $array[$_[0]] };
      }
   }
   return $d; 
}


###############################################################################
#
#  Basic visual object.
#  All visualization primitives and compound types must be derived thereof.
#
package Visual::Object;

use Polymake::Struct (
   [ new => '%' ],
   [ '$Name' => '#%' ],
   [ '$Title' => '#%', default => 'undef' ],
);

use overload '""' => sub { visualize(shift); undef },
             '==' => \&refcmp,
             '!=' => sub { !&refcmp };

sub representative { undef }

sub check_points {
   my ($name, $pts)=@_;
   if (is_object($pts) && $pts->isa("Visual::DynamicCoord")
       || is_like_array($pts)) {
      $pts;
   } else {
      croak( "$name neither an array nor an interactive object" );
   }
}

###############################################################################
#
#  Filter function for constructors of Visual objects checking and unifying arguments
#  for various label attributes (vertex labels, facet labels, etc.)
#
#  The result is either 'undef' if no labels should be displayed at all
#  or a reference to a sub taking the item (vertex, facet, etc.) index and returning a string.
#  This way the visualization back-ends can be significantly simplified,
#  as they have to distinguish between just these two cases.
#
#  As input following data can be passed:
#  - a keyword "hidden" => no labels are displayed
#   
#  - a keyword "show"   => the item index itself serves as a label 
#    (this is also the default behavior if nothing or an "undef" is passed)
#  - an array reference => the elements are assumed to contain the label strings
#  - a hash map reference => the keys are item indexes, the corresponding values are label strings
#  - a string => assumed to consist of labels separated with white spaces;
#                single labels may be enclosed in single or double quotes.
#  - a code reference => accepted as is
#
#  @param $name   name of the attribute, e.g. "VertexLabels"
#  @param $labels value passed to the constructor of the Visual object

sub unify_labels {
   my ($name, $labels)=@_;
   if (defined $labels) {
      if (is_code($labels)) {
         $labels
      } elsif (is_like_array($labels)) {
         sub { $labels->[$_[0]] }
      } elsif (is_like_hash($labels)) {
         sub { $labels->{$_[0]} }
      } elsif ($labels eq "hidden") {
         undef
      } elsif ($labels eq "show") {
         sub { shift }
      } else {
         my @labels;
         while ($labels =~ /\G\s* (?: (['"]) (.*?) (?!< \\) \1 | (\S+) ) \s*/xg) {
            push @labels, $2 // ($3 eq "_" ? " " : $3);
         }
         sub { @labels[$_[0]] }
      }
   } else {
      sub { shift }
   }
}

###############################################################################
#
#  Filter function for constructors of Visual objects checking and unifying arguments
#  for various item attributes like vertex/facet colors, vertex/edge thickness,
#  facet transparency, etc.  It can be used in the Visual object constructor as well as
#  in its merge method, which is usually automatically generated alongside the constructor.
#
#  The result is either a constant value of an appropriate type (or an "undef"
#  if nothing at all has been passed) which is then to be applied to all items of
#  the visual object, or a reference to a sub taking the item (vertex, facet, etc.)
#  index and returning a value specific for this item.
#  This way the visualization back-ends can be significantly simplified,
#  as they have to distinguish between just these two cases.
#
#  As input following data can be passed:
#  - a single value => is applied to all items
#  - an array reference => the elements are assumed to be the attribute values 
#  - a hash map reference => the keys are item indexes, the corresponding values are the attributes
#  - a code reference => accepted as is
#
#  For color attributes (which are recognized by their names ending with "Color")
#  the conversion to the RGB class is guaranteed, so that as input attributes values
#  all possible kinds of specifying a color are accepted, such as strings with color names,
#  three-element R-G-B arrays, HSV objects, etc.
#
#  @param $name  name of the attribute, e.g. "VertexColor"; used to distinguish color attributes
#  @param $decor value of the attribute as passed to the constructor of the Visual object
#                or to the merge() method
#  @param $default default value of the attribute as specified in the Visual object Struct definition
#                  (when called from the constructor) or the previous value stored in the object (from the merge method).
#
#  Merging with the default (or previous) values is done according to the following rules:
#  - passed value is a constant or an array reference => completely overrides the default value
#  - passed value is a hash map reference => default values apply for items not contained in the map
#  - passed value is a code reference => if it returns a defined value, it is applied, otherwise the default value
#  - passed value is "undef" => default values apply for all items

sub unify_decor {
   my ($name, $decor, $default)=@_;
   if ($name =~ /color(?:s)?$/i) {
      !defined($decor)
      ?  $default :
      is_like_array($decor)
      ?  (is_code($default)
          ?  sub { my $c=$decor->[$_[0]]; defined($c) ? get_RGB($c) : &$default }
          :  sub { my $c=$decor->[$_[0]]; defined($c) ? get_RGB($c) : $default } ) :
      is_like_hash($decor)
      ?  (is_code($default)
          ?  sub { my $c=$decor->{$_[0]}; defined($c) ? get_RGB($c) : &$default }
          :  sub { my $c=$decor->{$_[0]}; defined($c) ? get_RGB($c) : $default } ) :
      is_code($decor)
      ?  (is_code($default)
          ?  sub { my $c=&$decor; defined($c) ? get_RGB($c) : &$default }
          :  sub { my $c=&$decor; defined($c) ? get_RGB($c) : $default } )
      :  get_RGB($decor)

   } else {
      is_like_array($decor)
      ?  sub { $decor->[$_[0]] } :
      is_like_hash($decor)
      ?  (is_code($default)
          ?  sub { $decor->{$_[0]} // &$default } :
          defined($default)
          ?  sub { $decor->{$_[0]} // $default }
          :  sub { $decor->{$_[0]} } ) :
      is_code($decor)
      ?  (is_code($default)
          ?  sub { &$decor // &$default } :
          defined($default)
          ?  sub { &$decor // $default }
          :  $decor )
      :  $decor // $default
   }
}

###############################################################################
#
#  Container for several visualization objects - to be derived from
#
package Visual::Container;

use Polymake::Struct (
   [ new => '%@' ],
   [ '@ISA' => 'Visual::Object' ],
   [ '@elements' => '@' ],
   [ '%defaults' => '#%' ],
);

sub propagate_defaults {
   my $self=shift;
   while (my ($name, $val)=each %{$self->defaults}) {
      my $applied;
      foreach my $c (@{$self->elements}) {
         my $obj=is_object($c) ? $c : $c->[0];
         if (my $access_method=UNIVERSAL::can($obj,$name)) {
            my $filter=Struct::get_field_filter($access_method);
            foreach my $vis ($c==$obj ? ($obj) : @$c) {
               if (Struct::is_default($access_method->($vis))) {
                  if ($filter) {
                     $val=select_method($filter,$obj)->($name,$val);
                     undef $filter;
                  }
                  $access_method->($vis)=$val;
               }
            }
            $applied=1;
         } elsif (instanceof Container($obj)) {
            foreach my $vis ($c==$obj ? ($obj) : @$c) {
               $vis->defaults->{$name} ||= $val;
            }
            $applied=1;
         }
      }
      croak( "default attribute $name is not applicable to any element of the ", ref($self) ) unless $applied;
   }
}

# Object to store viewer feedback in
sub representative : method {
   my $self=shift;
   my $rep;
   foreach my $vis (@{$self->elements}) {
      defined($rep=$vis->representative) and return $rep;
   }
   undef
}

###########################################################################################
#
#  Direct writing to a file without starting a GUI
#

package Visual::FileWriter;

sub import {
   (undef, my %params)=@_;
   my $pkg = caller;
   my ($top_pkg) = $pkg =~ /^([^:]+)/;
   my $multiple = $params{multiple};
   eval <<".";
package $pkg;
use Polymake::Struct (
   [ '\@ISA' => "${top_pkg}::File", "${top_pkg}::Viewer" ],
   [ new => ';\$' ],
   [ '\$file' => '#1 // &Visual::FileWriter::prompt_filename' ],
   [ '\$multiple' => '$multiple' ],
);

*graphics = \\&Visual::FileWriter::self;
*run = \\&Visual::FileWriter::run;
.
}

sub self : method { $_[0] }

sub run : method {
   my ($self)=@_;
   if (!ref($self->file)) {
      open my $handle, ">", $self->file
         or die "can't create output file ", $self->file, ": $!\n";
      $self->file=$handle;
   }
   print { $self->file } $self->toString;
}

sub prompt_filename {
   my ($pkg)=@_;
   if ($Shell->interactive) {
      print "Please specify the output file name; the suffix ", $pkg->file_suffix, " may be omitted.\n";
      my $response=$Shell->enter_filename("", { prompt => $pkg->file_suffix . " file" });
      if ($response !~ /\.\w+$/) {
         $response.=$pkg->file_suffix;
      }
      $response
   } else {
      croak('missing argument File=>"filename"');
   }
}

###########################################################################################
#
#  Generator of file names
#

package Visual::FileWriter::Auto;

use Polymake::Struct (
   [ new => '$' ],
   [ '$Package' => '#1' ],
   [ '$viewer' => 'undef' ],
);

sub new_drawing {
   my ($self, $title)=@_;
   if ($self->viewer) {
      if ($self->Package->multiple) {
         return $self->viewer->new_drawing($title);
      }
      # write previous file
      $self->viewer->run;
   }
   # eliminate dangerous characters in the file name
   (my $filename=$title) =~ s'[ /.&(){}<>:;|]'_'g;
   $filename .= $self->Package->file_suffix;
   warn_print( "writing to file $filename" ) if $Verbose::files;
   ($self->viewer=$self->Package->new($filename))->new_drawing($title);
}

sub file : method { (shift)->viewer->file }

sub run : method { (shift)->viewer->run }

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
