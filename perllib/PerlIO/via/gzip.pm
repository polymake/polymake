#  Simplified and adapted copy of the publicly available CPAN module.
#  Please refer to the original author copyright note in the pod section at the end of this file.
#
#  Significant changes to the original module are:
#
#  - Removed FLUSH method because it is called not only before CLOSE as assumed by the author,
#    but also during the write process, as soon as the internal buffer of utf8 layer needs a drainage.
#    The problem of premature closing of the output handle by PerlIO::via is remedied now
#    by keeping a private dup-ed copy of the file handle.
#
#  - Removed FILL method because it has (wrongly) delivered the next line only; replaced it with READ
#    which delivers as much input data as available.
#
#  - Removed dependency on PerlIO::Utils module because it does not belong to the standard set of modules
#    distributed with perl or usual Linux systems.
#
#  - Removed some consistency checks from main working methods because this module
#    is not exposed in polymake to the broad public, thus we can always assume its correct usage.
#
#-------------------------------------------------------------------------------

package PerlIO::via::gzip;
use strict;
use warnings;
use IO::Compress::Gzip qw(:constants);
use IO::Uncompress::Gunzip;
use Carp;
our $VERSION = '0.022';
our $COMPRESSION_LEVEL = 9;
our $COMPRESSION_STRATEGY = Z_DEFAULT_STRATEGY;
our $BLOCK_SIZE = 4096;

sub PUSHED { 
    my ($class, $mode) = @_;
    my $stat;
    my $self = {};
    $mode =~ s/\+//;
    $self->{mode} = $mode;
    bless $self, $_[0];
}


# open hook
sub FILENO {
    my ($self, $fh) = @_;
    if ( !defined $self->{inited} ) {
	my $compress = $self->{mode} =~ /w|a/;
	$self->{fileno} = fileno($fh); # nec. to kick fileno hooks
	$self->{inited} = 1;
	if ($compress) {
	    open my $duped, ">&", $fh;
	    $self->{duped} = $duped;
	    $self->{gzip} = IO::Compress::Gzip->new( 
		$duped, 
		AutoClose => 1,
		Level => $COMPRESSION_LEVEL,
		Strategy => $COMPRESSION_STRATEGY,
		);
	    croak "via(gzip) [OPEN]: Couldn't create compression stream" unless ($self->{gzip});
	    $self->{gzip}->autoflush(1);
	}
	else {
	    $self->{gunzip} = IO::Uncompress::Gunzip->new(
		$fh,
		BlockSize => $BLOCK_SIZE
		);

	    croak "via(gzip) [OPEN]: Couldn't create decompression stream" unless ($self->{gunzip});
	}

    }
    $self->{fileno};
}

sub READ {
    my $self = shift;
    pop;
    return $self->{gunzip}->read(@_);
}

sub WRITE {
    my $self = shift;
    pop;
    return $self->{gzip}->write(@_);
}

sub CLOSE {
    my ($self, $fh) = @_;
    return -1 unless $self->{inited}; # not open yet
    if ($self->{gzip}) {
        $self->{gzip}->flush;
	$self->{gzip}->close;

	return $fh ? $fh->close : 0;
    }
    else {
	$self->{gunzip}->close;
	return $fh->close if $fh;
    }
}

1;
__END__

=pod 

=head1 NAME

PerlIO::via::gzip - PerlIO layer for gzip (de)compression

=head1 SYNOPSIS

 # compress
 open( $cfh, ">:via(gzip)", 'stdout.gz' );
 print $cfh @stuff;

 # decompress
 open( $fh, "<:via(gzip)", "stuff.gz" );
 while (<$fh>) {
    ...
 }

=head1 DESCRIPTION

This module provides a PerlIO layer for transparent gzip de/compression,
using L<IO::Compress::Gzip> and L<IO::Uncompress::Gunzip>. 

=head1 Changing compression parameters

On write, compression level and strategy default to the defaults specified in 
L<IO::Compress::Gzip>. To hack these, set

 $PerlIO::via::gzip::COMPRESSION_LEVEL

and

 $PerlIO::via::gzip::COMPRESSION_STRATEGY

to the desired constants, as imported from L<IO::Compress::Gzip>.

=head1 SEE ALSO

L<PerlIO|perlio>, L<PerlIO::via>, L<IO::Compress::Gzip>, L<IO::Uncompress::Gunzip>

=head1 AUTHOR - Mark A. Jensen

 Email maj -at- fortinbras -dot- us
 http://fortinbras.us
 http://bioperl.org/wiki/Mark_Jensen

=cut


