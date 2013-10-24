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

sub help($)
{
	print <<EOF;

Generate stub functions for library symbols

Usage: $0 OPTIONS LIBRARY

OPTIONS:
	-x <SYM1[,SYM2[,...]]>      exclude symbols from list
	-v <FILE>                   write version map into FILE
	-o <FILE>                   write C code into FILE
	--help                      this screen
EOF
	exit(shift);
}

Getopt::Long::Configure("no_ignore_case");
GetOptions (
    "x=s@"   => $except,
    "v=s"   => \$versfile,
    "o=s"   => \$outfile,
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
print OUT "#include <stdlib.h>\n";
print OUT "static void segv(void) { exit(0); }\n";
print OUT "static char array[1];\n";

srand(42);

foreach my $file (@ARGV)
{
    my %sectlist;
    open (IN, "/usr/bin/readelf -W --sections $file|") or die;
    while(<IN>)
    {
	next unless /^ +\[ *(\d+)\] *(\.rodata|\.data.rel.ro|\.data|\.bss)/;
	$sectlist{$1} = $2;
    }
    close IN;

    open (IN, "/usr/bin/readelf -W --symbols $file|") or die;
    while(<IN>)
    {
	my ($size, $type, $bind, $visib, $sect, $sym, $ver, $at);
	next unless s/^ +\d+: *//;
	chomp;
	my @f = split(/ +/);

	next unless $#f >= 6;
	$size = $f[1];
	next unless $f[2] eq 'FUNC' or $f[2] eq 'OBJECT';
	$type = $f[2];
	next unless $f[3] eq 'GLOBAL' or $f[3] eq 'WEAK';
	$bind = $f[3];
	$visib = $f[4];
	next unless $f[5] =~ /\d+/;
	$sect = $f[5];
	$sym = $f[6];

	if($sym =~ /(.*?)(\@\@?)(.*)/)
	{
	    $sym = $1;
	    $at = $2;
	    $ver = $3;
	}

	next if exists $except->{$sym};

	if($visib eq 'PROTECTED') {
	    print OUT "__attribute__ ((visibility (\"protected\"))) ";
	}
	if($ver)
	{
	    my $vsym = sprintf "SEGV_%08X_%s",int(rand(0xffffffff)), $sym;
	    push @{$versions{$ver}}, $vsym;
	    print OUT "__asm__(\".symver $vsym,$sym$at$ver\");\n";
	    $sym=$vsym;
	}
	if($type eq 'FUNC')
	{
	    print OUT "void $sym() { segv(); }\n" if $bind eq 'GLOBAL';;
	    print OUT "void $sym() __attribute__ ((weak, alias (\"segv\")));\n" if $bind eq 'WEAK';
	}
	elsif($type eq 'OBJECT')
	{
	    print OUT "__attribute__ ((weak)) " if $bind eq 'WEAK';

	    if($sectlist{$sect} eq '.rodata')
	    {
		print OUT "const char $sym\[$size\]={1};\n";
	    }
	    elsif($sectlist{$sect} eq '.data.rel.ro')
	    {
		print OUT "char* const $sym\[$size/sizeof(void*)\]={array};\n";
	    }
	    elsif($sectlist{$sect} eq '.data')
	    {
		print OUT "char $sym\[$size\]={1};\n";
	    }
	    elsif($sectlist{$sect} eq '.bss')
	    {
		print OUT "char $sym\[$size\];\n";
	    }
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
