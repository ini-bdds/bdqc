package BDQC::FileSignature::Generic;

###############################################################################
# Class       : BDQC::FileSignature::Generic
#
# Description : This class is autogenerated via generatePerlClasses.pl and
#
###############################################################################

use strict;
use warnings;

use BDQC::Response qw(processParameters);

my $CLASS = 'BDQC::FileSignature::Generic';
my $DEBUG = 0;
my $VERBOSE = 0;
my $TESTONLY = 0;

my $VERSION = '0.0.1';

#### BEGIN CUSTOMIZED CLASS-LEVEL VARIABLES AND CODE




#### END CUSTOMIZED CLASS-LEVEL VARIABLES AND CODE


sub new {
###############################################################################
# Constructor
###############################################################################
  my $METHOD = 'new';
  print "DEBUG: Entering $CLASS.$METHOD\n" if ( $DEBUG );
  my $self = shift;
  my %parameters = @_;
  my $class = ref($self) || $self;

  #### Create the object with any default attributes
  $self = {
  };
  bless $self => $class;

  #### Process constructor object parameters
  my $filePath = processParameters( name=>'filePath', required=>0, allowUndef=>0, parameters=>\%parameters, caller=>$METHOD );
  $self->{_filePath} = $filePath;

  #### BEGIN CUSTOMIZATION. DO NOT EDIT MANUALLY ABOVE THIS. EDIT MANUALLY ONLY BELOW THIS.




  #### END CUSTOMIZATION. DO NOT EDIT MANUALLY BELOW THIS. EDIT MANUALLY ONLY ABOVE THIS.

  #### Complain about any unexpected parameters
  my $unexpectedParameters = '';
  foreach my $parameter ( keys(%parameters) ) { $unexpectedParameters .= "ERROR: unexpected parameter '$parameter'\n"; }
  die("CALLING ERROR [$METHOD]: $unexpectedParameters") if ($unexpectedParameters);

  print "DEBUG: Exiting $CLASS.$METHOD\n" if ( $DEBUG );
  return($self);
}


sub getFilePath {
###############################################################################
# getFilePath
###############################################################################
  my $METHOD = 'getFilePath';
  print "DEBUG: Entering $CLASS.$METHOD\n" if ( $DEBUG );
  my $self = shift || die("parameter self not passed");

  print "DEBUG: Exiting $CLASS.$METHOD\n" if ( $DEBUG );
  return($self->{_filePath});
}


sub setFilePath {
###############################################################################
# setFilePath
###############################################################################
  my $METHOD = 'setFilePath';
  print "DEBUG: Entering $CLASS.$METHOD\n" if ( $DEBUG );
  my $self = shift || die("parameter self not passed");
  my $value = shift;


  $self->{_filePath} = $value;
  print "DEBUG: Exiting $CLASS.$METHOD\n" if ( $DEBUG );
  return 1;
}


sub calcSignature {
###############################################################################
# calcSignature
###############################################################################
  my $METHOD = 'calcSignature';
  print "DEBUG: Entering $CLASS.$METHOD\n" if ( $DEBUG );
  my $self = shift || die ("self not passed");
  my %parameters = @_;

  #### Define standard parameters
  my ( $response, $debug, $verbose, $quiet, $testonly, $outputDestination, $rmiServer );

  {
  #### Set up a response object
  $response = BDQC::Response->new();
  $response->setState( status=>'NOTSET', message=>"Status not set in method $METHOD");

  #### Process standard parameters
  $debug = processParameters( name=>'debug', required=>0, allowUndef=>0, default=>0, overrideIfFalse=>$DEBUG, parameters=>\%parameters, caller=>$METHOD, response=>$response );
  $verbose = processParameters( name=>'verbose', required=>0, allowUndef=>0, default=>0, overrideIfFalse=>$VERBOSE, parameters=>\%parameters, caller=>$METHOD, response=>$response );
  $quiet = processParameters( name=>'quiet', required=>0, allowUndef=>0, default=>0, parameters=>\%parameters, caller=>$METHOD, response=>$response );
  $testonly = processParameters( name=>'testonly', required=>0, allowUndef=>0, default=>0, parameters=>\%parameters, caller=>$METHOD, response=>$response );
  $outputDestination = processParameters( name=>'outputDestination', required=>0, allowUndef=>0, default=>'STDERR', parameters=>\%parameters, caller=>$METHOD, response=>$response );
  $rmiServer = processParameters( name=>'rmiServer', required=>0, allowUndef=>0, parameters=>\%parameters, caller=>$METHOD, response=>$response );
  print "DEBUG: Entering $CLASS.$METHOD\n" if ( $debug && !$DEBUG );
  }
  #### Process specific parameters
  my $filePath = processParameters( name=>'filePath', required=>0, allowUndef=>1, parameters=>\%parameters, caller=>$METHOD, response=>$response );
  if ( ! defined($filePath) ) {
    $filePath = $self->getFilePath();
  } else {
    $self->setFilePath($filePath);
  }

  #### Die if any unexpected parameters are passed
  my $unexpectedParameters = '';
  foreach my $parameter ( keys(%parameters) ) { $unexpectedParameters .= "ERROR: unexpected parameter '$parameter'\n"; }
  die("CALLING ERROR [$METHOD]: $unexpectedParameters") if ($unexpectedParameters);

  #### Return if there was a problem with the required parameters
  return $response if ( $response->{errorCode} =~ /MissingParameter/i );

  #### Set the default state to not implemented. Do not change this. Override later
  my $isImplemented = 0;

  #### BEGIN CUSTOMIZATION. DO NOT EDIT MANUALLY ABOVE THIS. EDIT MANUALLY ONLY BELOW THIS.


  $isImplemented = 1;
  my $signature = {};

  unless ( open(INFILE,'<:raw',$filePath) ) {
    $response->logEvent( status=>'ERROR', level=>'ERROR', errorCode=>"UnableToOpenFile", verbose=>$verbose, debug=>$debug, quiet=>$quiet, outputDestination=>$outputDestination, 
      message=>"Unable to open file '$filePath'");
    return $response;
  }

  my $charHistogram = {};
  my @charList = ();
  
  my $chunk;
  my $bytesRead = read(INFILE,$chunk,1024*50);
  my @ascii = unpack("C*",$chunk);
  map { $charList[$_]++; } @ascii;

  #### Find a median character count and ascii value
  my ( @nonZeroAsciiValues, @nonZeroCharacterCounts );

  #### Transform from an array to a hash and collect some stats along the way
  my %stats = ( fractionBelow128=>0, fractionAbove127=>0, meanAsciiValue=>0, nDifferentCharacters=>0 );
  for (my $i=0; $i<256; $i++) {
    if ( $charList[$i] ) {
      $charHistogram->{$i} = $charList[$i];
      $stats{fractionBelow128} += $charList[$i] if ( $i<128 );
      $stats{fractionAbove127} += $charList[$i] if ( $i>127 );
      $stats{meanCharacterCount} += $charList[$i];
      $stats{meanAsciiValue} += $i;
      push(@nonZeroAsciiValues,$i);
      push(@nonZeroCharacterCounts,$charList[$i]);
      $stats{nDifferentCharacters}++;
    }
  }

  #### Normalize
  if ( $bytesRead ) {
    $stats{fractionBelow128} = $stats{fractionBelow128} / $bytesRead;
    $stats{fractionAbove127} = $stats{fractionAbove127} / $bytesRead;
  }

  my @sortedNonZeroAsciiValues = sort numerically @nonZeroAsciiValues;
  my @sortedNonZeroCharacterCounts = sort numerically @nonZeroCharacterCounts;

  $stats{meanCharacterCount} = $stats{meanCharacterCount}/$stats{nDifferentCharacters} if ( $stats{nDifferentCharacters} );
  $stats{meanAsciiValue} = $stats{meanAsciiValue}/$stats{nDifferentCharacters} if ( $stats{nDifferentCharacters} );
  $stats{medianAsciiValue} = $sortedNonZeroAsciiValues[$stats{nDifferentCharacters}/2];
  $stats{medianCharacterCount} = $sortedNonZeroCharacterCounts[$stats{nDifferentCharacters}/2];

  #### Some heuristics of what kind of file this is
  if ( $bytesRead == 0 ) {
    $stats{fileType} = 'zeroLength';
  } elsif ( $stats{fractionBelow128} == 0 ) {
    $stats{fileType} = 'binary';
  } elsif ( $stats{fractionAbove127} < 0.1) {
    $stats{fileType} = 'text';
  } else {
    $stats{fileType} = 'binary';
  }

  #### If text, what subtype
  if ( $stats{fileType} eq 'text' ) {
    my $lineNumbers = $charHistogram->{10};
    $lineNumbers = ($charHistogram->{13}||0) if ( ($charHistogram->{13}||0) > $lineNumbers );
    if ( ($charHistogram->{9}||0) > .9 * $lineNumbers ) {
      $stats{subFileType} = 'tsv';
    } elsif ( ( ($charHistogram->{60}||0) + ($charHistogram->{62}||0) ) > $stats{meanAsciiValue} ) {
      $stats{subFileType} = 'xml';
    } elsif ( ( ($charHistogram->{34}||0) + ($charHistogram->{58}||0) ) > $stats{meanAsciiValue} ) {
      $stats{subFileType} = 'json';
    } elsif ( ($charHistogram->{44}||0) > .9 * $lineNumbers ) {
      $stats{subFileType} = 'csv';
    } else {
      $stats{subFileType} = 'plain';
    }
  }


  #### Store the results in the response object
  #$signature->{charHistogram} = $charHistogram;
  #$signature->{stats} = \%stats;
  #$response->{signature} = $signature;
  $response->{signature} = \%stats;



  #### END CUSTOMIZATION. DO NOT EDIT MANUALLY BELOW THIS. EDIT MANUALLY ONLY ABOVE THIS.
  {
  if ( ! $isImplemented ) {
    $response->logEvent( status=>'ERROR', level=>'ERROR', errorCode=>"Method${METHOD}NotImplemented", message=>"Method $METHOD has not yet be implemented", verbose=>$verbose, debug=>$debug, quiet=>$quiet, outputDestination=>$outputDestination );
  }

  #### Update the status codes and return
  $response->setState( status=>'OK', message=>"Method $METHOD completed normally") if ( $response->{status} eq 'NOTSET' );
  print "DEBUG: Exiting $CLASS.$METHOD\n" if ( $debug );
  }
  return $response;
}


sub show {
###############################################################################
# show
###############################################################################
  my $METHOD = 'show';
  print "DEBUG: Entering $CLASS.$METHOD\n" if ( $DEBUG );
  my $self = shift || die ("self not passed");
  my %parameters = @_;

  #### Create a simple text representation of the data in the object
  my $buffer = '';
  $buffer .= "$self\n";
  my $filePath = $self->getFilePath() || '';
  $buffer .= "  filePath=$filePath\n";

  print "DEBUG: Exiting $CLASS.$METHOD\n" if ( $DEBUG );
  return $buffer;
}


sub numerically {
###############################################################################
# numerically
# sorting routine to sort numbers but allow for missing values, which sort first
###############################################################################
  if ( ( (! defined($a)) || $a =~ /^\s*$/) && ( (! defined($b)) || $b =~ /^\s*$/) ) {
    return 0;
  } elsif ( (! defined($a)) || $a =~ /^\s*$/ ) {
    return -1;
  } elsif ( ! defined($b) || $b =~ /^\s*$/ ) {
    return 1;
  }
  return $a <=> $b;
}



###############################################################################
1;
