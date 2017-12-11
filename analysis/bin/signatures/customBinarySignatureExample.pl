#!/usr/bin/perl
#
###############################################################################
# Program       customBinarySignatureExample.pl
# Author        Eric Deutsch <edeutsch@systemsbiology.org>
# Date          2017-12-09
#
# Description : This program is a simple example of an external signature that
#               users can customize to their own needs and call from bdqc.pl
#
#  customBinarySignatureExample.pl --help
#
###############################################################################

use strict;
use warnings;
$| = 1;

use Getopt::Long;
use FindBin;
use JSON;

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
  --filename x        Name of the file to process through this signature

 e.g.:  $PROG_NAME --filename IMG_100.jpg

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
  my $filename = $OPTIONS{'filename'};

  #### Set your signature name here
  my $signatureName = "customBinarySignatureExample";

  #### Create a hash for the response and assume a successful result
  my $response;
  $response->{status} = 'OK';
  $response->{message} = "File parsing completed normally";
  $response->{signatureName} = $signatureName;

  #### Create an emtpy hash for the signature data
  my $signature = {};

  #### If the file wasn't passed, set the status to ERROR and don't return a signature
  my $continue = 1;
  if ( ! $filename ) {
    $response->{status} = 'ERROR';
    $response->{message} = "Filename was not specified. Please specify a filename";
    $continue = 0;
  }

  #### If the file can't be found, set the status to ERROR and don't return a signature
  if ( $continue && ! -e $filename ) {
    $response->{status} = 'ERROR';
    $response->{message} = "File '$filename' not found";
    $continue = 0;
  }

  if ( $continue ) {
    if ( open(INFILE,$filename) ) {
      $signature->{isReadable} = "true";
    } else {
      $signature->{isReadable} = "false";
      $response->{message} = "Returning very bare bones signature that file could not be opened";
      $continue = 0;
    }
    #### Even if we couldn't open the file, still return no error and the nearly emtpy model. Kinder not to fail completely?
    $response->{signature} = $signature;
  }

  #### If the file was openable, then read through it and return a signature for it.
  #### Simple signatures can just be a series of scalar attributes and hashes.
  #### Scalars are best for things that don't vary with file size.
  #### Hashes are ideal containers for counts of things that scale with file size
  #### because hash counts are normalized for each file.
  if ( $continue ) {
    my $chunk;
    my $done = 0;
    my $firstCharacter;
    my $charHistogram = {};
    my @charList = ();

    while ( ! $done ) {
      my $bytesRead = read(INFILE,$chunk,1024);
      last unless ( $bytesRead );
      my @ascii = unpack("C*",$chunk);
      map { $charList[$_]++; } @ascii;
      $done = 1 if ( $bytesRead < 1024 );
      if ( !defined($firstCharacter) ) {
        $firstCharacter = $ascii[0];
      }
    }

    #### Transform the character array into a hash. It is faster to record as an array first, then convert to a hash
    for (my $i=0; $i<256; $i++) {
      $charHistogram->{$i} = $charList[$i] if ( $charList[$i] );
    }

    #### Store the information we got into the signature
    #### In this case, a single scalar that is the first ASCII number of the first character of the file
    #### and then a histogram of the total counts of all ASCII numbers of all characters in the file.
    $signature->{firstCharacter} = $firstCharacter;
    $signature->{charHistogram} = $charHistogram;
  }

  #### The last step is to write JSON to STDOUT for BDQC to read back
  my $json = JSON->new->allow_nonref;
  $json->canonical();
  print $json->pretty->encode($response);

  return;
}

