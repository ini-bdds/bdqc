#!/usr/bin/perl
#
###############################################################################
# Program    testDataModelScalar.pl
# Author     Eric Deutsch <edeutsch@systemsbiology.org>
# Date       2017-12-08
#
# Description : This program tests different types of vectors through
#               DataModel::Scalar
#
#  testDataModelScalar.pl --help
#
###############################################################################

use strict;
use warnings;
$| = 1;

use Getopt::Long;
use FindBin;
use JSON;

use lib "$FindBin::Bin/../lib";
use BDQC::DataModel::Scalar;

#### Set program name and usage banner
my $PROG_NAME = $FindBin::Script;
my $USAGE = <<EOU;
Usage: $PROG_NAME [OPTIONS]
Options:
  --help              This message
  --verbose n         Set verbosity level.  default is 0
  --quiet             Set flag to print nothing at all except errors
  --debug n           Set debug flag
  --testonly          If set, nothing is actually altered
  --debug n           Set debug level. The default is 0
  --vectorTag x       Tag (name) of the vector to test

 e.g.:  $PROG_NAME --vectorTag 5int

EOU

#### Process options and print usage if an illegal options is provided
my %OPTIONS;
unless (GetOptions(\%OPTIONS,"help","verbose:i","quiet","debug:i","testonly",
                   "vectorTag:s",
  )) {
  print "$USAGE";
  exit 2;
}

#### Print usage on --help
if ( $OPTIONS{help} ) {
  print "$USAGE";
  exit 1;
}

main();
exit 0;


###############################################################################
sub main {

  #### Extract the run control options
  my $verbose = $OPTIONS{"verbose"} || 0;
  my $quiet = $OPTIONS{"quiet"} || 0;
  my $debug = $OPTIONS{"debug"} || 0;
  my $testonly = $OPTIONS{'testonly'} || 0;

  
  #### Get the distribution value
  my $vectorTag = $OPTIONS{'vectorTag'};
  unless ( $vectorTag ) {
    print "ERROR: Please specify a --vectorTag, or use --help for more information\n\n";
    return;
  }

  my ($comment, $suffix, @values);

  #### Create vectorTag 5int
  if ( $vectorTag eq '5int' ) {
    $comment = "Five integers, no outliers";
    $suffix = "5pts_flat_noOutliers";
    @values = ( 6,20,13,9,17 );

  #### Create vectorTag 4int
  } elsif ( $vectorTag eq '4int' ) {
    $comment = "Four integers, no outliers";
    $suffix = "4pts_flat_noOutliers";
    @values = ( 6,20,13,9 );

  #### Create vectorTag 3int
  } elsif ( $vectorTag eq '3int' ) {
    $comment = "Three integers, no outliers";
    $suffix = "3pts_flat_noOutliers";
    @values = ( 6,20,13 );

  #### Create vectorTag 2int
  } elsif ( $vectorTag eq '2int' ) {
    $comment = "Two integers";
    $suffix = "2pts";
    @values = ( 6,20 );

    #### Create vectorTag 1int
  } elsif ( $vectorTag eq '1int' ) {
    $comment = "One integer";
    $suffix = "1pts";
    @values = ( 6 );

  #### Create vectorTag 3int_2identical
  } elsif ( $vectorTag eq '3int_2identical' ) {
    $comment = "Three integers, two are identical";
    $suffix = "3pts_2identical";
    @values = ( 6,6,13 );

  #### Create vectorTag 5int_1outlier
  } elsif ( $vectorTag eq '5int_1outlier' ) {
    $comment = "Five integers, one outliers";
    $suffix = "5pts_1outlier";
    @values = ( 6,20,130,9,17 );

  #### Create vectorTag 5int_1null
  } elsif ( $vectorTag eq '5int_1null' ) {
    $comment = "Five integers, one null";
    $suffix = "5pts_1null";
    @values = ( 6,20,'',9,17 );

  #### Create vectorTag 5int_2null
  } elsif ( $vectorTag eq '5int_2null' ) {
    $comment = "Five integers, two null";
    $suffix = "5pts_1null";
    @values = ( 6,'','',9,17 );

  #### Create vectorTag 5int_2null
  } elsif ( $vectorTag eq '5int_2null' ) {
    $comment = "Five integers, two null";
    $suffix = "5pts_1null";
    @values = ( 6,'','',9,17 );

  #### Else not known
  } else {
    print "ERROR: --vectorTag = $vectorTag is not yet supported.\n\n";
    return;
  }

  my $nValues = scalar(@values);

  my $model = BDQC::DataModel::Scalar->new( vector=>\@values );
  my $result = $model->create();
  my $json = JSON->new->allow_nonref;

  print "===========================================================\n";
  print "Test=$vectorTag\n";
  print "Vector=".join(",",@values)."\n";
  print $result->show();
  print "-------------------------\n";
  print $json->pretty->encode($result->{model});

  return;
}




