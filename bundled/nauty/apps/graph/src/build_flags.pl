# input for generate_ninja_targets.pl

my $foreign_src = $ConfigFlags{'bundled.nauty.NautySrc'};
my $nauty_src = $foreign_src ? '${bundled.nauty.NautySrc}' : '${root}/bundled/nauty/external/nauty';
my $generated_dir = '${builddir}/staticlib/nauty';
my @generated_headers = qw( nauty.h naututil.h gtools.h );
my @generated_in = map { /\.h$/; "$nauty_src/$`-h.in" } @generated_headers;
my @generated_out = map { "$generated_dir/$_" } @generated_headers;
my $include_generated = ($foreign_src && grep { -f "$foreign_src/$_" } @generated_headers)
                        ? join(" ", map { "-include $_" } @generated_out)
                        : "-I$generated_dir";

( CXXFLAGS => "-DBIGNAUTY -I$generated_dir -I$nauty_src",

  GENERATED => {
    out => "@generated_out", in => "@generated_in",
    command => "cd $generated_dir; CC=\"\${CC}\" CFLAGS=\"\${CFLAGS}\" $nauty_src/configure --quiet >/dev/null 2>/dev/null; rm -f makefile",
  },

  staticlib => {
    SOURCEDIR => $nauty_src,
    SOURCES => [ qw(naugraph.c naurng.c nausparse.c nautaux.c nautil.c nautinv.c naututil.c nauty.c rng.c schreier.c) ],
    CFLAGS => "-DBIGNAUTY $include_generated -I$nauty_src",
  }
)
