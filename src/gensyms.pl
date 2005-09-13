#!/usr/bin/perl -w

#  Generate stub functions for library symbols
#  Copyright (C) 2005 Ludwig Nussel <l-n@users.sourceforge.net>
# 
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#  
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#  
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA

use strict;
use Getopt::Long;

my $except = [];
my %versions;
my $versfile;
my $outfile;
my $weak;

sub help($)
{
	print <<EOF;

Generate stub functions for library symbols

Usage: $0 OPTIONS LIBRARY

OPTIONS:
	-x <SYM1[,SYM2[,...]]>      exclude symbols from list
	-v <FILE>                   write version map into FILE
	-o <FILE>                   write C code into FILE
	--weak                      generate weak symbols
	--help                      this screen
EOF
	exit(shift);
}

Getopt::Long::Configure("no_ignore_case");
GetOptions (
    "x=s@"   => $except,
    "v=s"   => \$versfile,
    "o=s"   => \$outfile,
    "weak"   => \$weak,
    "h|help"  => sub { help(0); }
    ) or exit(1);

if(!$outfile)
{
    print STDERR "no output file specified\n";
    help(1);
}

$except = { map { $_ => 1 }  split(/,/,join(',',@{$except}))};

$except->{'_init'} = 1;
$except->{'_fini'} = 1;

open (OUT, '>', $outfile) or die;
print OUT "static void segv(void) { char* die = 0; ++*die; }\n";

foreach my $file (@ARGV)
{
    my %seen;

    open (IN, "/usr/bin/readelf -W --symbols $file|") or die;
    while(<IN>)
    {
	my ($sym, $ver, $at);
	next unless s/^ +\d+: *//;
	chomp;
	my @f = split(/ +/);

	next unless $#f >= 6;
	next unless $f[2] eq 'FUNC';
	next unless $f[3] eq 'GLOBAL';
	next unless $f[5] =~ /\d+/;
	$sym = $f[6];

	next if exists $seen{$sym};
	$seen{$sym} = 1;

	if($sym =~ /(.*?)(\@\@?)(.*)/)
	{
	    $sym = $1;
	    $at = $2;
	    $ver = $3;
	}

	next if exists $except->{$sym};

	if($ver)
	{
	    my $vsym = sprintf "SEGV_%08X_%s",int(rand(0xffffffff)), $sym;
	    push @{$versions{$ver}}, $vsym;
	    print OUT "__asm__(\".symver $vsym,$sym$at$ver\");\n";
	    print OUT "void $vsym() { segv(); }\n" unless $weak;
	    print OUT "void $vsym() __attribute__ ((weak, alias (\"segv\")));\n" if $weak;
	}
	else
	{
	    print OUT "void $sym() { segv(); }\n" unless $weak;
	    print OUT "void $sym() __attribute__ ((weak, alias (\"segv\")));\n" if $weak;
	}
    }
    close IN;
}
close OUT;
    
if(scalar keys %versions)
{
    if(!$versfile)
    {
	print STDERR "no version file specified but versioned symbols found\n";
	exit(1);
    }

    open (OUT, '>', $versfile) or die;
    foreach my $ver (keys %versions)
    {
	print OUT "$ver {\n  local:\n";
	foreach my $sym (@{$versions{$ver}})
	{
	    print OUT "    $sym;\n";
	}
	print OUT "};\n";
    }
    close OUT;
}
