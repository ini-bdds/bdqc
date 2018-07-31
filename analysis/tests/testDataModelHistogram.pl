#!/usr/bin/perl
#
###############################################################################
# Program    testDataModelHistogram.pl
# Author     Eric Deutsch <edeutsch@systemsbiology.org>
# Date       2017-12-08
#
# Description : This program tests different types of vectors through
#               DataModel::Histogram
#
#  testDataModelHistogram.pl --help
#
###############################################################################

use strict;
use warnings;
$| = 1;

use Getopt::Long;
use FindBin;
use JSON;

use lib "$FindBin::Bin/../lib";
use BDQC::DataModel;
use BDQC::DataModel::Histogram;

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

 e.g.:  $PROG_NAME --vectorTag test1

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

  #### Create vectorTag test1
  if ( $vectorTag eq 'test1' ) {
    @values = ( 
                     undef,
                     undef,
                     {
                        "5" => 538
                     },
                     {
                        "5" => 644
                     },
                     undef,
                     {
                        "10" => 4493
                     },
                     {
                        "2" => 6062
                     },
                     {
                        "5" => 811
                     },
                     {
                        "5" => 570
                     },
                     undef,
                     {
                        "5" => 559
                     },
                     {
                        "5" => 550
                     },
                     undef,
                     undef,
                     undef,
                     {
                        "5" => 830
                     },
                     undef,
                     {
                        "5" => 1015
                     },
                     undef,
                     {
                        "5" => 742
                     },
                     undef,
                     undef
     );

  } elsif ( $vectorTag eq 'test2' ) {
    @values = ( 
                     undef,
                     undef,
                     {
                        "10" => 4493
                     },
                     {
                        "2" => 6062
                     },
                     {
                        "5" => 830
                     },
                     undef,
                     undef,
                     undef,
                     {
                        "5" => 811
                     },
                     {
                        "5" => 559
                     },
                     undef,
                     undef,
                     {
                        "5" => 538
                     },
                     {
                        "5" => 570
                     },
                     undef,
                     {
                        "5" => 742
                     },
                     undef,
                     undef,
                     {
                        "5" => 1015
                     },
                     undef,
                     {
                        "5" => 644
                     },
                     {
                        "5" => 550
                     }
     );

  #### Else not known
  } else {
    print "ERROR: --vectorTag = $vectorTag is not yet supported.\n\n";
    return;
  }

  my $nValues = scalar(@values);

  #my $model = BDQC::DataModel::Histogram->new( vector=>\@values );
  my $model = BDQC::DataModel->new( vector=>\@values );
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




