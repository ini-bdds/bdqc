#!/usr/bin/perl -w
#
###############################################################################
# Program    bdqc.cgi
# Author     Eric Deutsch <edeutsch@systemsbiology.org>
# Date       2017-12-11
#
# Description : Simple web interface to BDQC software
#
#  bdqc.cgi --help
#
###############################################################################

use strict;
use Getopt::Long;
use FindBin;
use Data::Dumper;
use JSON;
use File::Basename;

use lib "$FindBin::Bin/../lib";
use BDQC::KB;
use BDQC::UI;

# Globals
my $json = JSON->new();
my $kb = new BDQC::KB;
my $ui = new BDQC::UI;

main();

sub main {

  print $ui->getHeader();
  print $ui->startPage();
  my $params = $ui->parseParameters();

  my $rpath = $params->{kbRootPath} || $ENV{kbRootPath} || $ARGV[0];
  $rpath ||= "/net/db/projects/PeptideAtlas/MRMAtlas/analysis/2017/BDQC/testdata/log";

  unless ( -e "${rpath}.qckb.json" ) {
    print "Error: kbRootPath ${rpath}.qckb.json not found\n";
  }
  unless ( -e "${rpath}.qckb.storable" ) {
    print "Error: kbRootPath storable data not found\n";
  }

  my $result = $kb->createKb();
  $result = $kb->loadKb( kbRootPath => $rpath, quiet => 1 );

#  open JSN, "${rpath}.qckb.json";
#  my $json_string;
#  { 
#    undef local $/;
#    $json_string = <JSN>;
#  }
#  close JSN;

#  my $qckb = decode_json( $json_string );
  my $qckb = $kb->{_qckb};
  my %models;

  # Loop over data structure.  File type
  for my $ft ( keys( %{$qckb->{fileTypes}} ) ) {

    my @files;
    # Under type read list of files
    for my $tag ( @{$qckb->{fileTypes}->{$ft}->{fileTagList}} ) {
      push @files, basename( $tag );
    }

    # Then Signature
    for my $sig ( keys( %{$qckb->{fileTypes}->{$ft}->{signatures}} ) ) {
      my $sig_obj = $qckb->{fileTypes}->{$ft}->{signatures}->{$sig};

      # Then individual 'Elements'
      for my $sigelem ( keys( %{$sig_obj} ) ) {
        my $has_outliers = 0;
        my $has_deviation = 0;
        my @dev;

        next unless $sig_obj->{$sigelem}->{model}->{deviations};

        my $fidx = 0;
        for my $dev ( @{$sig_obj->{$sigelem}->{model}->{deviations}} ) {
          last unless defined ( $dev->{deviationFlag} );
          $has_deviation++;
          if ( $dev->{deviationFlag} !~ /^normal$/i ) {
            $has_outliers++;
          }
          $dev->{filename} = $files[$fidx];
          push @dev, $dev;
          $fidx++;
        }

        my $mkey = $sig . '__' . $sigelem;
        $mkey =~ s/FileSignature::/FS/;

#        next unless $has_outliers;
        next unless $has_deviation;

        $models{$mkey} = { hasout => $has_outliers,
                             data => \@dev };
      }
    }
  }
  my $msel = $ui->getModelSelect( \%models );

  # If we have some models to plot, get page
  if ( $msel ) {
    my $plotHTML = $ui->getPlotHTML( models => \%models,
                                     select => $msel,
                                     params => $params );
    print $plotHTML;
  }
  print $ui->endPage();

  exit;
#  my $out = $kb->getOutliers( quiet => 1, verbose => 0, asObject => 1 );
}

__DATA__
