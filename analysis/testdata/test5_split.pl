#!/bin/perl

use strict;
use warnings;

my $filename = 'Homo_sapiens.GRCh38.90.uniprot.tsv';
open(INFILE,$filename) || die("ERROR: Cannot open '$filename'");
my $header = <INFILE>;
my %firstLetters;

while ( my $line = <INFILE> ) {
  my @columns = split(/\t/,$line);
  my $firstLetter = substr($columns[3],0,1);
  unless ( $firstLetters{$firstLetter} ) {
    local *FH;
    open(FH,">$firstLetter.tsv") || die("ERROR: Cannot open '$firstLetter.tsv'");
    $firstLetters{$firstLetter} = *FH;
    print FH $header;
  }
  local *FILE = $firstLetters{$firstLetter};
  print FILE $line;
}


