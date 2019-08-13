
@conf_vars=qw( );

sub allowed_options {
   my ($allowed_options, $allowed_with)=@_;
   @$allowed_with{ qw( flint ) }=();
}

sub usage {
   print STDERR "  --with-flint=PATH  installation path of FLINT\n";
}

sub proceed {
   my ($options)=@_;

   my $flint_path;
   my $flint_version;
   my $cxxflags;
   my $ldflags;

   if (defined ($flint_path=$options->{flint}) and $flint_path ne ".none.") {
      my $inc="$flint_path/include";
      my $lib=Polymake::Configure::get_libdir($flint_path, "flint");
      if (-f "$inc/flint/flint.h" && -f "$lib/libflint.$Config::Config{so}" ) {
         $cxxflags .= " -I$inc";
         $ldflags .= " -L$lib -Wl,-rpath,$lib";
      } else {
         die "Invalid installation location of flint, header files and/or library not found\n";
      }
   }
   if (defined $CLANGversion) {
      $cxxflags .= " -Wno-tautological-pointer-compare";
   }

   if ($options->{prereq} ne ".none." && $flint_path ne ".none.") {
      my $error=Polymake::Configure::build_test_program(<<'---', CXXFLAGS => $cxxflags, LDFLAGS => $ldflags, LIBS => "-lflint -lgmp -lmpfr");

#include <cstddef>
#include <flint/fmpq_poly.h>
#include <iostream>

   using namespace std;

   int main (int argc, char *argv[])
   {
      fmpq_poly_t fp;
      fmpq_poly_init(fp);
      fmpq_poly_clear(fp);
      cout << "version " << FLINT_VERSION << endl;
      return 0;
   }
---
      if ($?==0) {
         my $message=Polymake::Configure::run_test_program();
         if ($?) {
               die "Could not run a test program checking for flint.\n",
                   "The complete error log follows:\n\n$message\n",
                   "Please investigate the reasons and fix the installation.\n";
         } else {
            ($flint_version) = $message =~ /version ([0-9]\.[0-9]\.[0-9])/;
            $flint_version //= "0";
            my $minversion = "2.5.2";
            if (Polymake::Configure::v_cmp($flint_version,$minversion) < 0) {
                  die "Your flint version $flint_version is too old, at least version $minversion is required.\n";
            }
            $cxxflags .= " -DPOLYMAKE_WITH_FLINT";
         }
      } else {
            die "Could not compile a test program checking for flint.\n",
                "The most probable reasons are that the library is installed at a non-standard location,\n",
                "is not configured to build a shared module, or missing at all.\n",
                "The complete error log follows:\n\n$error\n",
                "Please install the library and specify its location using --with-flint option, if needed.\n";
      }
   } elsif ($options->{flint} ne ".none.") {
      # no prereq check, assume flint is available
      $cxxflags .= " -DPOLYMAKE_WITH_FLINT";
   }
   # we need to add these flags to the core flags as FlintPolynomial lives in the core
   # which is needed to specialize UniPolynomial<Rational,Int>
   $Polymake::Configure::CXXFLAGS .= $cxxflags;
   $Polymake::Configure::LDFLAGS .= $ldflags;
   $Polymake::Configure::LIBS .= " -lflint";
   return "ok ($flint_version @ ".($flint_path//"system").")";
}

