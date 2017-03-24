# Copyright (c) 2013-2016 Silke Horn, Andreas Paffenholz
# http://solros.de/polymake/poly_db
# http://www.mathematik.tu-darmstadt.de/~paffenholz
# 
# This file is part of the polymake extension polyDB.
# 
# polyDB is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# polyDB is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with polyDB.  If not, see <http://www.gnu.org/licenses/>.

package PolyDB::Shell;

require Exporter;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw(poly_db_tabcompletion);


sub poly_db_tab_completion {
	my $self = shift;
	my $line = shift;
    pos($line)=$self->term->Attribs->{point};
	
	# col name
	if ($line =~
       m{(?: \b db \s*=>\s* ["'](?'db' [^"']*)["']\s*,\s*) (:? \b $id_re \s*=>\s* .*?\s*,\s*)* (?: \b collection \s*=>\s* )
         (?:(?'quote' ['"]) (?'prefix' [^"']*)? )? \G}xogc) {

		my ($db, $quote, $prefix) = @+{qw(db quote prefix)};
		if (defined $quote) {
			$self->completion_words = [ list_col_completions($db, $prefix) ];
			$self->term->Attribs->{completion_append_character}=$quote;
			
			return 1;
		} else {
			$self->completion_words = ['"'];
			return 1;
		}	
	}

	# db name
	elsif ($line =~
       m{(?: \b db \s*=>\s* )
         (?:(?'quote' ['"]) (?'prefix' [^"']*)? )? \G}xogc) {
		my ($quote, $prefix) = @+{qw(quote prefix)};
		if (defined $quote) {
			$self->completion_words = [ list_db_completions($prefix) ];
			$self->term->Attribs->{completion_append_character}=$quote;
			
			return 1;
		} else {
			$self->completion_words = ['"'];
			return 1;
		}
	}
	
	
	return 0;
}


sub list_db_completions {
	my $prefix = shift;
	grep { /^\Q$prefix\E/ } @{common::db_get_list_db()};
}
sub list_col_completions {
	my $db = shift;
	my $prefix = shift;
	grep { /^\Q$prefix\E/ } @{common::db_get_list_col_for_db($db)};
}

if ($INC{"Polymake/Core/Shell.pm"}) {
   push @Core::Shell::custom_tab_completion,  \&PolyDB::Shell::poly_db_tab_completion;
}

1;
