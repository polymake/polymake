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

package Polymake::polytope::LPparser;

# The LPparser can be called in two ways:
#
# (1) LPparser($filename)
#     parses the file $filename and prepares the (arrays of) hashes @A, @P, %C, etc, detailed below
#
# (2) LPparser($filename, $testvec, $prefix)
#     additionally, whenever a new constraint is read in, it is checked whether $testvec satisfies it.
#
#     **Precondition**: The variable names in $filename **MUST** all be of the from $prefix.$i, with $i a number.
#
#     The reason for this is that since the rows are read in on the fly, the total number and ordering
#     of the variables are not known at the time this test takes place. Therefore, the variable names
#     that are read in are assumed to be directly parseable to an index in the test vector.

use Polymake::Struct (
   [ new => '$;$$' ],
   [ '$LPfile' => '#1' ],       # the filename of the .lp file to be read in
   [ '$testvec' => '#2' ],      # optionally, a reference to a test vector
   [ '$prefix' => '#3' ],       # optionally, a string with which all variables in the LP file are supposed to start;
                                #   defaults to 'x'
   '$name',                     # the name of the entire object, dervied from the filename
   '$lp_name',                  # the name of the LP, if specified as a comment in the input file
   '$ct',                       # counts how many relations have been read in
   '@Ineq',                     # inequalities Ax+B>=0, in the form (B, A)
   '@Eq',                       # equations Px+Q=0,     in the form (Q, P)
   '%Obj',                      # objective function Cx -> min
   '@L', '@U',                  # variable bounds  l <= x <= u
   '@X',                        # variable index => name
   '%Xi',                       # variable name => index
   '@Int',                      # variable index => is_integer (boolean)
   '$objsense',                 # 'max' or 'min'
);

sub new {
   my $self=&_new;
   replace_special_paths($self->LPfile);
   if ($self->LPfile =~ m{([^/]+)\.lp$}i) {
      $self->name = $1;
   }
   $self->ct = 0;
   if (!defined($self->prefix)) { $self->prefix = 'x'; }

   # the first variable represents the inhomogeneous part, so push initial values onto some stacks
   push @{$self->L}, undef;
   push @{$self->U}, undef;
   push @{$self->X}, "inhomog_var";

   open my $LP, $self->LPfile
     or die "Can't read ", $self->LPfile, ": $!\n";
   local $/;
   my $lp = <$LP>;
   close $LP;

   ($self->lp_name) = $lp =~ /\\problem name:\s*(\S+)/im
     and
   $self->lp_name =~ s/\.lp$//i;

   $lp =~ s/\\.*?\n//mg;        # comments start with '\' and go up to the end of line
   $lp =~ /^\s*(maximize|minimize|maximum|minimum|max|min)\s+/im
      or die "no objective function found\n";
   $lp=$'; #'
   $self->objsense= lc(substr($1,0,3)) eq "max" ? "+" : "-";

   $lp =~ /^\s*(?:subject\s+to|such\s+that|s\.?t\.?)\s+/im
      or die "no constraints found\n";
   $lp=$'; #'
   $self->Obj = $self->make_vector($`, "+");

   my ($constraints, @optionals)=split /^(bounds?|bin|binar(?:y|ies)|gen|generals?|end)\s*$/im, $lp;

   while ($constraints =~ /\S/) {
      $constraints =~ /[<>]\s*=?|=\s*[<>]?/
         or die "invalid constraint\n";
      $constraints=$';  #'
      my ($line, $rel)=($`, $&);
      my $label = '';
      if ($line =~ /^(.*):/) { $label = $1; }
      my $vecref = $self->make_vector($line, $rel=~/</ ? "-" : "+");
      my $rhs = parse_number($constraints, $rel=~/</ ? "+" : "-");
      die "invalid right hand side in input line '$line'\n" if $rhs eq "" or $rhs eq "-";
      $$vecref{0} = $rhs;

      if ($rel=~/[<>]/) {
         push @{$self->Ineq}, $vecref;
      } else {
         push @{$self->Eq}, $vecref;
      }
      if (defined $self->testvec) {
         my $unpermuted_vecref = $self->make_vector($line, $rel=~/</ ? "-" : "+", 1, $self->prefix);
         $$unpermuted_vecref{0} = $rhs;
         $self->livecheck($unpermuted_vecref, $rel, $label);
      }
   }

   if ($optionals[0] =~ /bounds?/i) {
      my $bounds=splice @optionals, 0, 2;
      while ($bounds =~ /\S/) {
         $bounds =~ s/^\s+//s;
         my ($x, $rel, $b, $inf);
         if ($bounds =~ /^([^\n]*?)[ \t]+free\s*/i) {
            $bounds=$';  #'
            my $name=$1;
            $x=$self->parse_name($name);
            if (defined $x) {
               undef $self->L->[$x];  undef $self->U->[$x];
            } else {
               die "invalid free variable declaration: $name\n";
            }
            next;
         }

         if ($bounds =~ /^([^\n]*?)([<>][ \t]*=?|=[ \t]*[<>]?)([^\n]*?)([<>][ \t]*=?|=[ \t]*[<>]?)/) {
            $bounds="$1$2$3\n$3$4$'";
         }

         my $xleft=0;
         $x=$self->parse_name($bounds);
         if (defined $x) {
            $xleft=1;
            $bounds =~ /[<>]\s*=?|=\s*[<>]?/
               or die "invalid bound variable declaration: $bounds\n";
            $bounds=$'; #'
            $rel=$&;
         }

         if ($bounds =~ /^\s*([+-])[ \t]*inf(inity)?\s*/) {
            $inf=$1;
            $bounds=$'; #'
         } else {
            $b=parse_number($bounds,"+");
            die "invalid variable bound value\n" if $b eq "" or $b eq "-";
         }

         if (!$xleft) {
            $bounds =~ /[<>]\s*=?|=\s*[<>]?/
               or die "invalid bound variable declaration\n";
            $rel=$&;
            $bounds=$';  #'
            $x=$self->parse_name($bounds);
            die "invalid bound variable declaration\n" unless defined $x;
         }

         if (defined $inf) {
            die "invalid infinite bound\n"
               if $inf eq "+" && $xleft != $rel=~/</
               or $inf eq "-" && $xleft != $rel=~/>/;
            if ($inf eq "+") {
               undef $self->U->[$x];
            } else {
               undef $self->L->[$x];
            }
         } else {
            if ($rel!~/[<>]/) {
               $self->L->[$x]=$self->U->[$x]=$b;
            } elsif ($xleft == $rel=~/</) {
               $self->U->[$x]=$b;
            } else {
               $self->L->[$x]=$b;
            }
         }
      }
   }

   while (@optionals>1) {
      @{$self->Int}=(0) x @{$self->X} unless @{$self->Int};
      my $binary= shift(@optionals) =~ /bin|binar(?:y|ies)/i;
      my $names=shift(@optionals);
      foreach my $name ($names =~ /\S+/g) {
         if (defined (my $x=$self->Xi->{$name})) {
            if ($binary) {
               $self->L->[$x]=0; $self->U->[$x]=1;
            }
            $self->Int->[$x]=1;
         } else {
            die "unknown variable name '$name' occured in ", ($binary ? "BINARY" : "GENERAL"), " section\n";
         }
      }
   }

   if (@optionals>1 || $optionals[0] !~ /end/i) {
      $optionals[-1] =~ /\S+/;
      die "unrecognized section '$&' near the end of file\n";
   }

   my $dim = @{$self->X};
   for (my $x = 1; $x < $dim; ++$x) {
      my $lo = $self->L->[$x];
      my $hi = $self->U->[$x];
      if (defined($lo) and defined($hi) and $lo == $hi) {
         my %vec;
         $vec{0} = $lo if $lo != 0;
         $vec{$x} = -1;
         push @{$self->Eq}, \%vec;
         next;
      }
      if (defined($lo)) {
         my %vec;
         $vec{0} = -$lo if $lo != 0;
         $vec{$x} = 1;
         push @{$self->Ineq}, \%vec;
      }
      if (defined($hi)) {
         my %vec;
         $vec{0} = $hi if $hi != 0;
         $vec{$x} = -1;
         push @{$self->Ineq}, \%vec;
      }
   }

   # don't permute the 0-th, homogeneous coordinate
   my @xorder = (0, sort {
      # do some clever sorting in case we have prefixed but not padded numbers
      my ($x, $y) = @{$self->X}[$a, $b];
      if (my ($xprefix, $xnumbers) = $x =~ /(\D*)(\d+)/) {
         if ($y =~ /(\D*)(\d+)/) {
            return $xprefix cmp $1 || $xnumbers <=> $2;
         }
      }
      $x cmp $y;
   } 1..$dim-1);

   my @invxorder;
   $#invxorder = $dim-1;  # reserve memory for @invxorder up front to prevent repeated reallocation
   foreach (0..$dim-1) {
       $invxorder[$xorder[$_]] = $_;
   }

   foreach my $line (@{$self->Ineq}, @{$self->Eq}, $self->Obj) {
      my %permline;
      while (my ($x, $coeff) = each %$line) {
         $permline{$invxorder[$x]} = $coeff;
      }
      $line = \%permline;
   }
   delete $self->Obj->{0};

   @{$self->X}=@{$self->X}[@xorder];
   @{$self->Int}=@{$self->Int}[@xorder] if @{$self->Int};
   shift(@{$self->Int}) if @{$self->Int}; # 0-th coordinate not required in convertation for the INTEGER_VARIABLES
   $self;
}

sub parse_number {              # "line", positive_sign => "advanced line", number
   my ($line,$positive)=@_;
   $line =~ m'^\s*([+-])?(?:[ \t]*\n)?([ \t\d]*\.?[ \t\d]*(e[ \t]*[+-]?[ \t\d]+)?)?\s*'si;
   $_[0]=$';  #'
   my ($sign, $number)=($1, lc($2));
   $number=~s/\s//g;                                    # in exact powers of 10
   $number="1$number" if substr($number,0,1) eq "e";    #   the leading 1 can be omitted
   $sign= ($sign or "+") eq $positive ? "" : "-";
   "$sign$number"
}

sub varindex {                  # "name" => index
   my ($self, $name)=@_;
   if ($name eq "inhomog_var") { die "tried to access dummy inhomogeneous variable\n"; }
   if (exists $self->Xi->{$name}) {
      $self->Xi->{$name}
   } else {
      push @{$self->X}, $name;
      push @{$self->L}, 0;
      $self->Xi->{$name}=$#{$self->X}
   }
}

sub parse_name ($;$$) {                # "line" => index or undef
   my ($self, $line, $unpermuted, $prefix)=@_;
   if ($line =~ m'^\s*([A-Za-z!"#$%&()/,;?@_\'`{}|~][A-Za-z0-9.!"#$%&()/,;?@_\'`{}|~ \t]*)\s*') {
      $_[1]=$'; #'
      my $name=$&;
      $name=~s/\s//g;
      if (!defined($unpermuted)) {
         $self->varindex($name)             # if we don't ask for the "unpermuted" version, give back the index in the hash
      } else {
         if ($name =~ /^$prefix(\d+)/) {    # else, the $name is supposed to be of the form $prefix.$i, and we give back $i
            $1
         } else {
            die "illegal name '$name' for prefix '$prefix'";
         }
      }
   } else {
      undef
   }
}

sub make_vector($$$;$$) {   # "linear expression", positive_sign => (coefficients)
                            # optional: $unpermuted, $prefix.
                            # if $unpermuted is defined, assign the coefficient of x$i in the relation directly to $vec{$i}
                            # where we assume $prefix=='x'
   my %vec;
   my ($self, $line, $positive, $unpermuted, $prefix)=@_;
   my $iline = $line;
   my ($coef, $i);

   $line =~ s/^.*://;     # remove a possible label
   if ($line eq " ") {    # trivial inequality
      $vec{0} = 1;
      return \%vec;
   }
   while ($line) {
      $coef = parse_number($line, $positive);
      $coef.="1" if $coef eq "" or $coef eq "-";
      $i = $self->parse_name($line, $unpermuted, $prefix);
      die "$0: invalid expression in input line '$iline'" unless defined $i;
      $vec{$i} = $coef;
   }
   \%vec
}

# If testvec is defined, the following function will called with each new linear relation that is read in.
# It tests testvec against the relation, and screams if testvec violates it.
#
# @param $vec Hash a perl hash with the entries of the linear relation
# @param $rel String the (in)equality relation read in
# @param $label the label of the relation
sub livecheck($$$) {
    my ($self, $vec, $rel, $label) = @_;
    if ($self->ct % 10000 == 0 && $self->ct > 0) {
        print "checked " , $self->ct, " relations\n";
    }
    $self->ct++;
    my $relvec = new SparseVector<Rational>($self->testvec->dim());
    keys %{$vec};
    while (my ($k, $v) = each %{$vec}) {
        $relvec->[$k] = $v;
    }
    my $val = $relvec * $self->testvec;
    my $is_ineq = ($rel =~ /[<>]/);
    if ( $is_ineq && $val < 0 ||
         !$is_ineq && $val != 0) {
        print "violated: relation ", $self->ct, "; $label; $rel: ", $relvec, " * ", $self->testvec, " = $val\n";
    }
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
