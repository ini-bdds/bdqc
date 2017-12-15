#!/usr/bin/perl
#
###############################################################################
# Program        bdqc.pl
# Author        Eric Deutsch <edeutsch@systemsbiology.org>
# Date            2017-11-06
#
# Description : This program is a simple interface to the BDQC software
#
#  bdqc.pl --help
#
###############################################################################

use strict;
use warnings;
$| = 1;

use Getopt::Long;
use FindBin;

use lib "$FindBin::Bin/../lib";

use BDQC::KB;

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
  --kbRootPath x      Full or relative root path QC KB results files (file extension will be added)
  --dataDirectory x   Full or relative directory path of the data to be scanned
  --calcSignatures    Calculate file signatures for all new files in the QC KB
  --importSignatures x Import .bdqc signatures from the specified external file
  --importLimit N     Limit the number of signatures during import to N (for testing only)
  --collateData       Collate all the data from individual files in preparation for modeling
  --calcModels        Calculate file signature models for all files in the QC KB
  --pluginModels      Substitute the default internal models and outlier detection with an external program
                        (not yet implemented)
  --pluginSignatures  Substitute or add the default signature calculators with an external program
                      Use format "<fileType>:<operation>=<command>;..."
                        <fileType> can be any one fileType (e.g. tsv) or "*all" to apply to all
                        <operation> is either add (append another sig calc) or set (cancel default/previous and set)
                        <command> is a shell command that will be run to calculate a signature. Response must be correct JSON to STDOUT
                        e.g. "*all:add=perl ../bin/customBinarySignatureExample.pl"
  --skipAttributes x  Semi-colon separated list of signatures/attributes to skip in the collation,
                      modeling, or outlier reporting (e.g. "extrinsic.mtime;tracking.dataDirectory" to skip
                      collation, model, or outlier reporting of the file modification times and directory names.
  --showOutliers      Show all outliers in the QC KB
  --sensitivity x     Set the sensitivity of the outlier detection. Can either be low, medium, high or a numerical
                      value specifying the number of deviations from typical to flag as an outlier.
                      low=10, medium=5 (default), and high=3 deviations

 e.g.:  $PROG_NAME --kbRootPath testqc --dataDirectory test1

EOU

#### Process options and print usage if an illegal options is provided
my %OPTIONS;
unless (GetOptions(\%OPTIONS,"help","verbose:i","quiet","debug:i","testonly",
                   "kbRootPath:s", "dataDirectory:s", "calcSignatures", "collateData",
                   "calcModels", "showOutliers", "importSignatures:s", "importLimit:i",
                   "pluginModels:s", "pluginSignatures:s", "skipAttributes:s", 
                   "sensitivity:s", 
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

  my $response = BDQC::Response->new();
  $response->setState( status=>'OK', message=>"Starting BDQC");

  #### Create the Quality Control Knowledge Base object
  my $qckb = BDQC::KB->new();
  my $result = $qckb->createKb();
  $response->mergeResponse( sourceResponse=>$result );

  #### If a KC QC file parameter was provided, see if there is already one to warm start with
  if ( $OPTIONS{kbRootPath} ) {
    $result = $qckb->loadKb( kbRootPath=>$OPTIONS{kbRootPath}, skipIfFileNotFound=>1 );
    $response->mergeResponse( sourceResponse=>$result );
  }

  #### Parse the plugins options
  if ( $result->{status} eq 'OK' && ( $OPTIONS{pluginSignatures} || $OPTIONS{pluginModels} ) ) {
    my $result = $qckb->parsePlugins( pluginSignatures=>$OPTIONS{pluginSignatures}, pluginModels=>$OPTIONS{pluginModels}, verbose => $verbose, quiet=>$quiet, debug=>$debug );
    $response->mergeResponse( sourceResponse=>$result );
  }

  #### Perform the scan of the dataPath
  if ( $result->{status} eq 'OK' && $OPTIONS{dataDirectory} ) {
    my $result = $qckb->scanDataPath( dataDirectory=>$OPTIONS{dataDirectory}, verbose => $verbose, quiet=>$quiet, debug=>$debug );
    $response->mergeResponse( sourceResponse=>$result );
  }

  #### Calculate signatures for all files in the KB
  if ( $result->{status} eq 'OK' && $OPTIONS{calcSignatures} ) {
    my $result = $qckb->calcSignatures( verbose => $verbose, quiet=>$quiet, debug=>$debug );
    $response->mergeResponse( sourceResponse=>$result );
  }

  #### Import signatures from an external file into the KB
  if ( $result->{status} eq 'OK' && $OPTIONS{importSignatures} ) {
    my $result = $qckb->importSignatures( inputFile=>$OPTIONS{importSignatures}, importLimit=>$OPTIONS{importLimit}, verbose => $verbose, quiet=>$quiet, debug=>$debug );
    $response->mergeResponse( sourceResponse=>$result );
  }

  #### Collate all attributes for all files in the KB by filetype into a form ready to model
  if ( $result->{status} eq 'OK' && $OPTIONS{collateData} ) {
    my $result = $qckb->collateData( skipAttributes=>$OPTIONS{skipAttributes}, verbose => $verbose, quiet=>$quiet, debug=>$debug );
    $response->mergeResponse( sourceResponse=>$result );
  }

  #### Calculate models and outliers for all files in the KB by filetype
  if ( $result->{status} eq 'OK' && $OPTIONS{calcModels} ) {
    my $result = $qckb->calcModels( skipAttributes=>$OPTIONS{skipAttributes}, verbose => $verbose, quiet=>$quiet, debug=>$debug );
    $response->mergeResponse( sourceResponse=>$result );
  }

  #### Show the deviations found in the data
  if ( $result->{status} eq 'OK' && $OPTIONS{showOutliers} ) {
    my $result = $qckb->getOutliers( skipAttributes=>$OPTIONS{skipAttributes}, sensitivity=>$OPTIONS{sensitivity}, verbose => $verbose, quiet=>$quiet, debug=>$debug );
    $response->mergeResponse( sourceResponse=>$result );
  }

  #### If a KC QC file parameter was provided, write out the KB
  if ( $result->{status} eq 'OK' && $OPTIONS{kbRootPath} ) {
    my $result = $qckb->saveKb( kbRootPath=>$OPTIONS{kbRootPath}, verbose => $verbose, quiet=>$quiet, debug=>$debug, testonly=>$testonly );
    $response->mergeResponse( sourceResponse=>$result );
  }

  #### If we're in an error state, show the results and exit abnormally
  if ( $response->{status} ne 'OK' ) {
    print "==============================\n";
    print "bdqc.pl terminated with errors:\n";
    print $response->show();
    exit 12;
  }

  return;
}

