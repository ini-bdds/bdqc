package BDQC::FileSignature::XML;

###############################################################################
# Class       : BDQC::FileSignature::XML
#
# Description : This class is autogenerated via generatePerlClasses.pl and
#
###############################################################################

use strict;
use warnings;

use BDQC::Response qw(processParameters);

my $CLASS = 'BDQC::FileSignature::XML';
my $DEBUG = 0;
my $VERBOSE = 0;
my $TESTONLY = 0;

my $VERSION = '0.0.1';

#### BEGIN CUSTOMIZED CLASS-LEVEL VARIABLES AND CODE

#### Potentially dangerous if we multithread signature generation
#### Also, dsize does not get initialized? Carries over from one
#### parse() to the next??
#### Better to make these object variables and pass directly to the handlers?
my %tags;
my $dsize;
my $ctag;



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
  my $filePath = processParameters( name=>'filePath', required=>0, allowUndef=>0, parameters=>\%parameters, caller=>$METHOD, response=>$response );
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

  $isImplemented++;

  use Data::Dumper;
  %tags = ();
  $ctag = '';

  require XML::Parser;
  my $xparser = new XML::Parser();
  $xparser->setHandlers( Start => \&start,
                          End  => \&end,
                         Char  => \&data,
                       Default => \&other );

  $xparser->parsefile( $filePath, ErrorContext => 3);
  my %curr_tags = %tags;
  $response->{signature} = { tags => \%curr_tags,
                             ntags => scalar(keys(%curr_tags)),
                             dsize => $dsize };

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


sub start {
###############################################################################
# start: callback for starting elements
###############################################################################
    my ( $p, $elem, %attr ) = @_;
    $ctag = $elem;
    $tags{$elem}++;
    for my $attr ( keys( %attr ) ) {
      $attr{$elem . '_' . $attr}++;
    }
}

sub data {
###############################################################################
# data: callback for data
###############################################################################
    # shift off objects
    my ( $p, $data) = @_;
    $dsize += length( $data );
    return "OK";
}

sub end {
###############################################################################
# end: callback for ending elements
###############################################################################
    my ( $p, $elem) = @_;
    $ctag = '';
}

sub other {
###############################################################################
# other: callback for anything else
###############################################################################
    my ( $p, $data) = @_;
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



###############################################################################
1;
