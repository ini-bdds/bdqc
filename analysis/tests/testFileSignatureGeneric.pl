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
use Time::HiRes qw(gettimeofday tv_interval);

use lib "$FindBin::Bin/../lib";
use BDQC::FileSignature::Generic;

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
  --filename x        Filename to put through FileSignature::Generic

 e.g.:  $PROG_NAME --vectorTag 5int

EOU

#### Process options and print usage if an illegal options is provided
my %OPTIONS;
unless (GetOptions(\%OPTIONS,"help","verbose:i","quiet","debug:i","testonly",
                   "filename:s",
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
  my $filename = $OPTIONS{'filename'};
  unless ( $filename ) {
    print "ERROR: Please specify a --filename, or use --help for more information\n\n";
    return;
  }
  unless ( -f $filename ) {
    print "ERROR: Filename '$filename' not found\n\n";
    return;
  }

  my $signature = BDQC::FileSignature::Generic->new( filePath=>$filename );

  my $t0 = [gettimeofday];
  my $result = $signature->calcSignature();
  my $t1 = [gettimeofday];
  my $elapsed = tv_interval($t0,$t1);
  print "Time elapsed=$elapsed\n";


  my $json = JSON->new->allow_nonref;
  $json->canonical();

  print "===========================================================\n";
  print "Filename=$filename\n";
  print $result->show();
  print "-------------------------\n";
  print $json->pretty->encode($result->{signature});

  return;
}




