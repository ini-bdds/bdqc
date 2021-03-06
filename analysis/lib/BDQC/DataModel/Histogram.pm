package BDQC::DataModel::Histogram;

###############################################################################
# Class       : BDQC::DataModel::Histogram
#
# Description : This class is autogenerated via generatePerlClasses.pl and
#
###############################################################################

use strict;
use warnings;

use BDQC::Response qw(processParameters);

my $CLASS = 'BDQC::DataModel::Histogram';
my $DEBUG = 0;
my $VERBOSE = 0;
my $TESTONLY = 0;

my $VERSION = '0.0.1';

#### BEGIN CUSTOMIZED CLASS-LEVEL VARIABLES AND CODE

#use BDQC::DataModel::Scalar::Number;       # FIXME
use BDQC::DataModel::Scalar;



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
  my $vector = processParameters( name=>'vector', required=>0, allowUndef=>0, parameters=>\%parameters, caller=>$METHOD );
  $self->{_vector} = $vector;

  #### BEGIN CUSTOMIZATION. DO NOT EDIT MANUALLY ABOVE THIS. EDIT MANUALLY ONLY BELOW THIS.




  #### END CUSTOMIZATION. DO NOT EDIT MANUALLY BELOW THIS. EDIT MANUALLY ONLY ABOVE THIS.

  #### Complain about any unexpected parameters
  my $unexpectedParameters = '';
  foreach my $parameter ( keys(%parameters) ) { $unexpectedParameters .= "ERROR: unexpected parameter '$parameter'\n"; }
  die("CALLING ERROR [$METHOD]: $unexpectedParameters") if ($unexpectedParameters);

  print "DEBUG: Exiting $CLASS.$METHOD\n" if ( $DEBUG );
  return($self);
}


sub getVector {
###############################################################################
# getVector
###############################################################################
  my $METHOD = 'getVector';
  print "DEBUG: Entering $CLASS.$METHOD\n" if ( $DEBUG );
  my $self = shift || die("parameter self not passed");

  print "DEBUG: Exiting $CLASS.$METHOD\n" if ( $DEBUG );
  return($self->{_vector});
}


sub setVector {
###############################################################################
# setVector
###############################################################################
  my $METHOD = 'setVector';
  print "DEBUG: Entering $CLASS.$METHOD\n" if ( $DEBUG );
  my $self = shift || die("parameter self not passed");
  my $value = shift;


  $self->{_vector} = $value;
  print "DEBUG: Exiting $CLASS.$METHOD\n" if ( $DEBUG );
  return 1;
}


sub create {
###############################################################################
# create
###############################################################################
  my $METHOD = 'create';
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
  my $vector = processParameters( name=>'vector', required=>0, allowUndef=>0, parameters=>\%parameters, caller=>$METHOD, response=>$response );
  if ( ! defined($vector) ) {
    $vector = $self->getVector();
  } else {
    $self->setVector($vector);
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
  my $nElements = scalar(@{$vector}) || 1;
  #print "INFO: Input to $CLASS has $nElements elements\n";
  my $masterHistogram = {};
  my @sums = ();
  my $iHistogram = 0;
  foreach my $histogram ( @{$vector} ) {
    next if ( !defined($histogram) );
    my $sum = 0;
    foreach my $key ( keys(%{$histogram}) ) {
      $sum += $histogram->{$key};
    }
    $sum = 1 unless ( $sum );
    $sums[$iHistogram] = $sum;
    foreach my $key ( keys(%{$histogram}) ) {
      $masterHistogram->{$key} += $histogram->{$key}/$sum/$nElements;
    }
    $iHistogram++;
  }

  #### Now compute a variance of each histogram from the masterHistogram to create a simple vector
  $iHistogram = 0;
  my @numericalVector = ();
  foreach my $histogram ( @{$vector} ) {
    if ( !defined($histogram) ) {
      push(@numericalVector,undef);
      next;
    }
    my $sum = $sums[$iHistogram];
    my $summedVariance = 0;
    foreach my $key ( keys(%{$histogram}) ) {
      my $value = $histogram->{$key}/$sum;
      my $masterValue = $masterHistogram->{$key};
      my $variance = abs($value-$masterValue);
      $summedVariance += $variance;
      #print "==\t$value\t$masterValue\t$variance\t$summedVariance\n";
    }
    #print "--\t$summedVariance\n";
    push(@numericalVector,$summedVariance);
    $iHistogram++;
  }

  #### Then run that vector through the usual BDQC::DataModel::Scalar::Number
  #print "My resulting vector has ".scalar(@vector)." elements\n";
  #my $model = BDQC::DataModel::Scalar::Number->new( vector=>\@numericalVector );       # FIXME
  my $model = BDQC::DataModel::Scalar->new( vector=>\@numericalVector );
  my $result = $model->create();
  $response->{model} = $result->{model};

  $response->{model}->{masterHistogram} = $masterHistogram;




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
  my $vector = $self->getVector() || '';
  $buffer .= "  vector=$vector\n";

  print "DEBUG: Exiting $CLASS.$METHOD\n" if ( $DEBUG );
  return $buffer;
}



###############################################################################
1;
