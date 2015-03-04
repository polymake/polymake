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

package Polymake::polytope::PortaParser;

use Polymake::Struct (
   [ new => '$' ],
   [ '$portaFile' => '#1' ],
   '$name',
   '$has_points',
   '$computed',
   '$dim',
   '@Points',
   '@Ineq',
   '@Eq',
);

sub new {
   my $self=&_new;
   replace_special_paths($self->portaFile);
   $self->portaFile =~ m{([^/]+)\.(?:(poi)|ieq)(\.(?(2)ieq|poi))?$}
      or die "only *.poi or *.ieq files allowed\n";
   $self->name=$1;
   $self->computed=defined($3);
   $self->has_points=defined($2) != $self->computed;

   open my $in, $self->portaFile
      or die "Can't read ", $self->portaFile, ": $!\n";
   local $_;
   while (<$in>) {
      if (/^DIM\s*=\s*(\d+)$/) {
         $self->dim=$1;
      } elsif (/^CON([VE])_SECTION$/) {
         $self->parse_points($in, $1 eq "V");
      } elsif (/^INEQUALITIES_SECTION$/) {
         $self->parse_facets($in);
      } elsif (/^(?:VALID|ELIMINATION_ORDER|(?:LOWER|UPPER)_BOUNDS)$/) {
         do { $_=<$in> } until (/\S/);          # skip it
      } elsif (/^END$/) {
         last;
      } elsif (/\S/) {
         die "file ", $self->portaFile, ", line $.: unrecognized input: $_\n";
      }
   }

   if ($self->has_points) {
      @{$self->Points} or die "file ", $self->portaFile, " lacking CONV_SECTION\n";
   } else {
      @{$self->Ineq} or die "file ", $self->portaFile, " lacking INEQUALITIES_SECTION\n";
   }

   $self;
}

sub parse_points {
   my ($self, $P, $CONV_flag)=@_;
   my $first= $CONV_flag ? 1 : 0;
   while (<$P>) {
      next unless /\S/;
      unless (/\d|\(hex\)/) {
         seek $P, -length, 1;
         last;
      }
      s/^\s*\([\s\d]+\)\s*//;
      s"\s*/\s*"/"g;
      push @{$self->Points}, [ $first, map { parse_number($_) } split ];
   }
}

sub parse_facets {
   my ($self, $P)=@_;
   while (<$P>) {
      next unless /\S/;
      unless (/^(?:\s*\([\s\d]+\))?(.*)([<=>])=/) {
         seek $P, -length, 1;
         last;
      }
      $_=$1;
      my $eq= $2 eq "=";
      my $gt= $2 eq ">";
      my $neg_sign= $gt ? "-" : "+";
      my $right=$';
      $right=~s/\s//g;
      my @x=(parse_number($right), (0) x $self->dim);
      if ($gt) {
         $x[0] =~ s/^-// or $x[0] =~ s/^/-/;
      }
      s/^\s*(?=[\dx])/+/;
      while (/\G\s* ([+-]) (.*?) x(\d+)/gx) {
         my $neg= $1 eq $neg_sign && "-";
         my $coef=$2;
         my $i=$3;
         $coef =~ s/\s//g;
         if (length($coef)) {
            $x[$i]=$neg.parse_number($coef);
         } else {
            $x[$i]=$neg."1";
         }
      }
      push @{$eq ? $self->Eq : $self->Ineq}, \@x;
   }
}

sub hextodec {
   my @dec=(0);
   foreach my $h (split(//,shift)) {
      $h=~tr/abcdef/012345/ and $h+=10;
      for (my $i=$#dec; $i>=0; $i--) {
         my $d=$dec[$i]*16+$h;  $dec[$i]=$d%10;  $h=$d/10;
      }
      unshift (@dec,$h%10), $h/=10  if $h;
      unshift (@dec,$h)             if $h;
   }
   join "", @dec
}

sub parse_number {
   my $n=shift;
   $n =~ s/\(hex\)([\da-f]+)/&hextodec($1)/eg;
   $n
}

##############################################################################
package Polymake::polytope::PortaConverter;

use Polymake::Struct (
   [ new => '$' ],
   [ '$out' => '#1' ],
);

sub new {
   my $self=&_new;
   open my $out, ">", $self->out
      or die "Can't create output file ", $self->out, "\n";
   $self->out=$out;
   $self;
}

sub print_dim {
   my ($self, $dim)=@_;
   my $out=$self->out;
   print $out "DIM=$dim\n";
}

sub print_valid_point {
   my ($self, $V)=@_;
   my $out=$self->out;
   print $out "VALID\n", $V->slice(1), "\n";
}

sub print_points {
   my ($self, $Points)=@_;
   my (@conv, @cone);
   my $out=$self->out;
   foreach my $p (@$Points) {
      if (!$p->[0]) {
         push @cone, join(" ", $p->slice(1))."\n";
      } else {
         push @conv, join(" ", $p->slice(1))."\n";
      }
   }
   print $out "CONV_SECTION\n", @conv if @conv;
   print $out "CONE_SECTION\n", @cone if @cone;
}

sub print_ineq_matrix {
   my ($Matrix, $out, $sign)=@_;
   foreach my $h (@$Matrix) {
      my $n=0;
      my @lhs=map { ++$n; !$_ ? () : ($_>0 && "+").$_."x".$n } @{$h->slice(1)};
      # skip the far hyperplane
      if (@lhs) {
        print $out @lhs, $sign, -$h->[0], "\n";
      }
   }
}

sub print_inequalities {
   my ($self, $Ineq, $Eq)=@_;
   my $out=$self->out;
   print $out "INEQUALITIES_SECTION\n";
   print_ineq_matrix($Ineq, $out, ">=");
   print_ineq_matrix($Eq, $out, "==") if defined($Eq);
}

sub DESTROY {
   my $self=shift;
   my $out=$self->out;
   print $out "END\n";
   close $out;
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
