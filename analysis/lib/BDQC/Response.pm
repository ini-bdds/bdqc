package BDQC::Response;

###############################################################################
# Class       : BDQC:Response
# Author      : Eric Deutsch <edeutsch@systemsbiology.org>
#
# Description : This class represents a standardized mechanism for a
#               method call to provide a rich response to the caller.
#
# Methods:
#   new()
#   logEvent()
#   setState()
#   testEvent()
#   processParameters()
#   mergeResponse()
#   serialize()
#
# Scalar Attributes:
#   status
#   message
#   fullMessage
#   errorCode
#   isValid
#   nErrors
#   nWarnings
#   messageLimit
#
# Complex Attributes:
#   errors
#   warnings
#   messages
#   validationErrors
#   validationWarnings
#   errorMessageCount
#   warningMessageCount
#
###############################################################################

###############################################################################
#### Special export of the processParameters function
use Exporter;
@ISA = qw(Exporter);
@EXPORT_OK = qw(processParameters);

use strict;
use warnings;
eval { require JSON };

use vars qw( $CLASS $DEBUG $VERBOSE $TESTONLY );

$CLASS = 'BDQC::Response';
$DEBUG = 0;
$VERBOSE = 0;
$TESTONLY = 0;

my $VERSION = '0.0.5';

###############################################################################
#### Other needed classes

###############################################################################
#### Class variables and setup
my $defaultMessageLimit = 3;

###############################################################################
# Constructor
###############################################################################
sub new {
  my $METHOD = 'new';
  print "DEBUG: Entering $CLASS.$METHOD\n" if ($DEBUG);
  my $self = shift;
  my %parameters = @_;
  my $class = ref($self) || $self;

  #### Create the object with any default attributes
  $self = {
    status => 'OK',
    message => 'No problems to report',
    fullMessage => '',
    errorCode => '',
    isValid => '',
    nErrors => 0,
    nWarnings => 0,
    nMessages => 0,
    nValidationErrors => 0,
    nValidationWarnings => 0,
    messageLimit => $defaultMessageLimit,
    errors => [],
    warnings => [],
    messages => [],
    validationErrors => [],
    validationWarnings => [],
    validationErrorCount => [],
    validationWarningCount => [],
  };
  bless $self => $class;

  #### Die if any unexpected parameters are passed
  my $unexpectedParameters = '';
  foreach my $parameter ( keys(%parameters) ) { $unexpectedParameters .= "ERROR: unexpected parameter '$parameter'\n"; }
  die("CALLING ERROR [$METHOD]: $unexpectedParameters") if ($unexpectedParameters);

  print "DEBUG: Exiting $CLASS.$METHOD\n" if ($DEBUG);
  return $self;
}


###############################################################################
# logEvent
###############################################################################
sub logEvent {
  my $METHOD = 'logEvent';
  print "DEBUG: Entering $CLASS.$METHOD\n" if ($DEBUG);
  my $self = shift || die ("self not passed");
  my %parameters = @_;

  #### Process standard parameters
  my $debug = processParameters( name=>'debug', required=>0, allowUndef=>0, default=>0, overrideIfFalse=>$DEBUG, parameters=>\%parameters, caller=>$METHOD, response=>$self );
  my $verbose = processParameters( name=>'verbose', required=>0, allowUndef=>0, default=>0, overrideIfFalse=>$VERBOSE, parameters=>\%parameters, caller=>$METHOD, response=>$self );
  my $quiet = processParameters( name=>'quiet', required=>0, allowUndef=>0, default=>0, parameters=>\%parameters, caller=>$METHOD, response=>$self );
  my $outputDestination = processParameters( name=>'outputDestination', required=>0, allowUndef=>0, default=>'STDERR', parameters=>\%parameters, caller=>$METHOD, response=>$self );
  print "DEBUG: Entering $CLASS.$METHOD\n" if ( $debug && !$DEBUG );

  #### Process specific parameters
  my $status = processParameters( name=>'status', required=>0, allowUndef=>1, parameters=>\%parameters, caller=>$METHOD, response=>$self );
  my $message = processParameters( name=>'message', required=>1, allowUndef=>0, parameters=>\%parameters, caller=>$METHOD, response=>$self );
  my $errorCode = processParameters( name=>'errorCode', required=>0, allowUndef=>1, parameters=>\%parameters, caller=>$METHOD, response=>$self );

  my $level = processParameters( name=>'level', required=>1, allowUndef=>0, parameters=>\%parameters, caller=>$METHOD, response=>$self );
  my $isValidationMessage = processParameters( name=>'isValidationMessage', required=>0, allowUndef=>1, parameters=>\%parameters, caller=>$METHOD, response=>$self );
  my $line = processParameters( name=>'line', required=>0, allowUndef=>1, parameters=>\%parameters, caller=>$METHOD, response=>$self );
  my $location = processParameters( name=>'location', required=>0, allowUndef=>1, parameters=>\%parameters, caller=>$METHOD, response=>$self );

  my $minimumVerbosity = processParameters( name=>'minimumVerbosity', required=>0, allowUndef=>1, default=>1, parameters=>\%parameters, caller=>$METHOD, response=>$self );

  #### Die if any unexpected parameters are passed
  my $unexpectedParameters = '';
  foreach my $parameter ( keys(%parameters) ) { $unexpectedParameters .= "ERROR: unexpected parameter '$parameter'\n"; }
  die("CALLING ERROR [$METHOD]: $unexpectedParameters") if ($unexpectedParameters);

  #### Allow both INFO and MESSAGE to be synonymous
  $level = uc($level);
  $level = 'INFO' if ( $level eq 'MESSAGE' );

  #### If the specified level is not valid, report this
  if ( $level ne 'ERROR' && $level ne 'INFO' && $level ne 'WARNING' ) {
    $message = "INTERNAL CODE ERROR: [$METHOD] specified message level cannot be '$level'. Forcing it to ERROR. Problem message: ".$message;
    $level = 'ERROR';
  }

  #### Check the outputDestination
  $outputDestination = uc($outputDestination);
  if ( $outputDestination ne 'STDERR' && $outputDestination ne 'STDOUT'
       && $outputDestination ne 'NONE' && $outputDestination ne 'BOTH' ) {
    $message = "INTERNAL CODE ERROR: [$METHOD] specified outputDestination cannot be '$outputDestination'. Forcing it to BOTH for maximum noisiness. Problem message: ".$message;
    $outputDestination = 'BOTH';
  }

  #### Build a full error message
  my $fullMessage = "$level";
  $fullMessage .= " ($errorCode)" if ( defined($errorCode) );
  $fullMessage .= ": ";
  $fullMessage .= "[line $line] " if ( defined($line) );
  $fullMessage .= $message;
  $fullMessage .= " $location" if ( defined($location) );

  #### If this is a validation event, then things are handled differently
  if ( $isValidationMessage ) {

    #### If this is an error, push it to the error list
    if ( $level eq 'ERROR' ) {
      $self->{nValidationErrors}++;
      $self->{validationErrorCount}->{$message}++;
      if ( $self->{validationErrorCount}->{$message} <= $self->{messageLimit} ) {
        push( @{$self->{validationErrors}}, $fullMessage );
      }
    }

    #### If this is a warning, push it to the warning list
    if ( $level eq 'WARNING' ) {
      $self->{nValidationWarnings}++;
      $self->{validationWarningCount}->{$message}++;
      if ( $self->{validationWarningCount}->{$message} <= $self->{messageLimit} ) {
        push( @{$self->{validationWarnings}}, $fullMessage );
      }
    }

    #### Set up validation attribute to 0 if there is an error
    if ( $status && $status eq 'ERROR' ) {
      $self->{isValid} = 0;
    }
    if ( $level && $level eq 'ERROR' ) {
      $self->{isValid} = 0;
    }

  #### Otherwise this is just an ordinary event
  } else {
  
    #### Copy the status attribute only if it is supplied
    $self->{status} = $status if ( $status );

    #### Copy the errorCode attribute only if it is supplied
    $self->{errorCode} = $errorCode if ( $errorCode );

    #### Always copy the message attribute
    $self->{message} = $message;

    #### Always copy the fullMessage attribute
    $self->{fullMessage} = $fullMessage;

    #### If this is an error, push it to the error list
    if ( $level eq 'ERROR' ) {
      $self->{nErrors}++;
      push( @{$self->{errors}}, $fullMessage );
      if ( $outputDestination eq 'STDOUT' || $outputDestination eq 'BOTH' ) {
        print "$fullMessage\n" unless ( $quiet );
      }
      if ( $outputDestination eq 'STDERR' || $outputDestination eq 'BOTH' ) {
        print STDERR "$fullMessage\n" unless ( $quiet );
      }
    }

    #### If this is a warning, push it to the warning list
    if ( $level eq 'WARNING' ) {
      $self->{nWarnings}++;
      push( @{$self->{warnings}}, $fullMessage );
      if ( $outputDestination eq 'STDOUT' || $outputDestination eq 'BOTH' ) {
        print "$fullMessage\n" unless ( $quiet );
      }
      if ( $outputDestination eq 'STDERR' || $outputDestination eq 'BOTH' ) {
        print STDERR "$fullMessage\n" unless ( $quiet );
      }
    }

    #### If this is just a message, push it to the message list
    if ( $level eq 'INFO' ) {
      if ( defined($minimumVerbosity) && $verbose >= $minimumVerbosity ) {
        $self->{nMessages}++;
        push( @{$self->{messages}}, $fullMessage );
        if ( $outputDestination eq 'STDOUT' || $outputDestination eq 'BOTH' ) {
          print "$fullMessage\n" unless ( $quiet );
        }
        if ( $outputDestination eq 'STDERR' || $outputDestination eq 'BOTH' ) {
          print STDERR "$fullMessage\n" unless ( $quiet );
        }
      }
    }

  }

  print "DEBUG: Exiting $CLASS.$METHOD\n" if ($debug);
  return $fullMessage;
}


###############################################################################
# testLogEvent
###############################################################################
sub testLogEvent {
  my $METHOD = 'testLogEvent';
  print "DEBUG: Entering $CLASS.$METHOD\n" if ($DEBUG);
  my $self = shift || die ("self not passed");
  my %parameters = @_;

  #### Set up a response object and a default error state
  my $response = BDQC::Response->new();

  #### Process standard parameters
  my $debug = processParameters( name=>'debug', required=>0, allowUndef=>0, default=>0, overrideIfFalse=>$DEBUG, parameters=>\%parameters, caller=>$METHOD, response=>$response );
  my $verbose = processParameters( name=>'verbose', required=>0, allowUndef=>0, default=>0, overrideIfFalse=>$VERBOSE, parameters=>\%parameters, caller=>$METHOD, response=>$response );
  my $quiet = processParameters( name=>'quiet', required=>0, allowUndef=>0, default=>0, parameters=>\%parameters, caller=>$METHOD, response=>$response );
  my $outputDestination = processParameters( name=>'outputDestination', required=>0, allowUndef=>0, default=>'STDERR', parameters=>\%parameters, caller=>$METHOD, response=>$response );
  print "DEBUG: Entering $CLASS.$METHOD\n" if ( $debug && !$DEBUG );

  #### Process specific parameters
  my $type = processParameters( name=>'type', required=>0, allowUndef=>1, parameters=>\%parameters, caller=>$METHOD, response=>$response );

  #### Die if any unexpected parameters are passed
  my $unexpectedParameters = '';
  foreach my $parameter ( keys(%parameters) ) { $unexpectedParameters .= "ERROR: unexpected parameter '$parameter'\n"; }
  die("CALLING ERROR [$METHOD]: $unexpectedParameters") if ($unexpectedParameters);

  #### Return if there was a problem with the required parameters
  return $response if ( $response->{errorCode} =~ /MissingParameter/i );

  #### Generate some example events
  $response->logEvent( level=>'INFO', verbose=>$verbose, debug=>$debug, quiet=>$quiet, outputDestination=>$outputDestination,
    message=>"First event generated in $METHOD. Type is INFO");

  $response->logEvent( level=>'WARNING', verbose=>$verbose, debug=>$debug, quiet=>$quiet, outputDestination=>$outputDestination,
    message=>"Second event generated in $METHOD. Type is WARNING");

  if ( 1 == 1 ) {
    $response->logEvent( status=>'ERROR', level=>'ERROR', errorCode=>'ExampleError', verbose=>$verbose, debug=>$debug, quiet=>$quiet, outputDestination=>$outputDestination,
      message=>"Third event generated in $METHOD. Type is ERROR");
    return $response;
  }

  $response->setState( status=>'OK', message=>"Method $METHOD completed normally");
  
  print "DEBUG: Exiting $CLASS.$METHOD\n" if ($debug);
  return $response;
}


###############################################################################
# setState
###############################################################################
sub setState {
  my $METHOD = 'setState';
  print "DEBUG: Entering $CLASS.$METHOD\n" if ($DEBUG);
  my $self = shift || die ("self not passed");
  my %parameters = @_;

  #### Process standard parameters
  my $debug = processParameters( name=>'debug', required=>0, allowUndef=>0, default=>0, overrideIfFalse=>$DEBUG, parameters=>\%parameters, caller=>$METHOD, response=>$self );
  my $verbose = processParameters( name=>'verbose', required=>0, allowUndef=>0, default=>0, overrideIfFalse=>$VERBOSE, parameters=>\%parameters, caller=>$METHOD, response=>$self );
  my $quiet = processParameters( name=>'quiet', required=>0, allowUndef=>0, default=>0, parameters=>\%parameters, caller=>$METHOD, response=>$self );
  my $outputDestination = processParameters( name=>'outputDestination', required=>0, allowUndef=>0, default=>'STDERR', parameters=>\%parameters, caller=>$METHOD, response=>$self );
  print "DEBUG: Entering $CLASS.$METHOD\n" if ( $debug && !$DEBUG );

  #### Process specific parameters
  my $status = processParameters( name=>'status', required=>0, allowUndef=>1, parameters=>\%parameters, caller=>$METHOD, response=>$self );
  my $errorCode = processParameters( name=>'errorCode', required=>0, allowUndef=>1, parameters=>\%parameters, caller=>$METHOD, response=>$self );
  my $message = processParameters( name=>'message', required=>1, allowUndef=>0, parameters=>\%parameters, caller=>$METHOD, response=>$self );

  #### Die if any unexpected parameters are passed
  my $unexpectedParameters = '';
  foreach my $parameter ( keys(%parameters) ) { $unexpectedParameters .= "ERROR: unexpected parameter '$parameter'\n"; }
  die("CALLING ERROR [$METHOD]: $unexpectedParameters") if ($unexpectedParameters);

  #### Store the passed values
  $self->{status} = $status if ( $status);
  $self->{errorCode} = $errorCode if ( $errorCode );
  $self->{message} = $message if ( $message );

  #### If the caller set the status to OK and there is no errorCode or message provided, clear them
  if ( $status && $status eq 'OK' ) {
    $self->{errorCode} = '' unless ( $errorCode );
    $self->{message} = '' unless ( $message );
    $self->{fullMessage} = '' unless ( $message );
  }

  print "DEBUG: Exiting $CLASS.$METHOD\n" if ($DEBUG);
  return;
}


###############################################################################
# show
###############################################################################
sub show {
  my $METHOD = 'show';
  print "DEBUG: Entering $CLASS.$METHOD\n" if ($DEBUG);
  my $self = shift || die ("self not passed");
  my %parameters = @_;

  #### Process standard parameters
  my $debug = processParameters( parameters=>\%parameters, caller=>$METHOD, name=>'debug', required=>0, allowUndef=>0, default=>0, overrideIfFalse=>$DEBUG );
  my $verbose = processParameters( parameters=>\%parameters, caller=>$METHOD, name=>'verbose', required=>0, allowUndef=>0, default=>0, overrideIfFalse=>$VERBOSE );
  my $quiet = processParameters( parameters=>\%parameters, caller=>$METHOD, name=>'quiet', required=>0, allowUndef=>0, default=>0 );
  my $outputDestination = processParameters( parameters=>\%parameters, caller=>$METHOD, name=>'outputDestination', required=>0, allowUndef=>0, default=>'STDERR' );
  print "DEBUG: Entering $CLASS.$METHOD\n" if ( $debug && !$DEBUG );

  #### Die if any unexpected parameters are passed
  my $unexpectedParameters = '';
  foreach my $parameter ( keys(%parameters) ) { $unexpectedParameters .= "ERROR: unexpected parameter '$parameter'\n"; }
  die("CALLING ERROR [$METHOD]: $unexpectedParameters") if ($unexpectedParameters);

  #### Generate a summary of the state of myself
  my $buffer = '';
  $buffer .= "status=$self->{status}\n";
  $buffer .= "errorCode=$self->{errorCode}\n" if ( $self->{errorCode} );
  $buffer .= "message=$self->{message}\n" if ( $self->{message} );
  $buffer .= "fullMessage=$self->{fullMessage}\n" if ( $self->{fullMessage} );
  $buffer .= "isValid=$self->{isValid}\n" if ( $self->{isValid} );

  #### Print error count and any errors
  $buffer .= "nErrors=$self->{nErrors}\n" if ( $self->{nErrors} );
  if ( $self->{nErrors} ) {
    foreach my $errorMessage ( @{$self->{errors}} ) {
      $buffer .= "  -$errorMessage\n";
    }
  }

  #### Print warning count and any warnings
  $buffer .= "nWarnings=$self->{nWarnings}\n" if ( $self->{nWarnings} );
  if ( $self->{nWarnings} ) {
    foreach my $warningMessage ( @{$self->{warnings}} ) {
      $buffer .= "  -$warningMessage\n";
    }
  }

  #### Print info count and any info messages
  $buffer .= "nMessages=$self->{nMessages}\n" if ( $self->{nMessages} );
  if ( $self->{nMessages} ) {
    foreach my $infoMessage ( @{$self->{messages}} ) {
      $buffer .= "  -$infoMessage\n";
    }
  }

  print "DEBUG: Exiting $CLASS.$METHOD\n" if ($debug);
  return $buffer;
}


###############################################################################
# mergeResponse
###############################################################################
sub mergeResponse {
  my $METHOD = 'mergeResponse';
  print "DEBUG: Entering $CLASS.$METHOD\n" if ($DEBUG);
  my $self = shift || die ("self not passed");
  my %parameters = @_;

  #### The response of this method is just this object
  my $response = $self;

  #### Process standard parameters
  my $debug = processParameters( name=>'debug', required=>0, allowUndef=>0, default=>0, overrideIfFalse=>$DEBUG, parameters=>\%parameters, caller=>$METHOD, response=>$response );
  my $verbose = processParameters( name=>'verbose', required=>0, allowUndef=>0, default=>0, overrideIfFalse=>$VERBOSE, parameters=>\%parameters, caller=>$METHOD, response=>$response );
  my $quiet = processParameters( name=>'quiet', required=>0, allowUndef=>0, default=>0, parameters=>\%parameters, caller=>$METHOD, response=>$response );
  my $outputDestination = processParameters( name=>'outputDestination', required=>0, allowUndef=>0, default=>'STDERR', parameters=>\%parameters, caller=>$METHOD, response=>$response );
  print "DEBUG: Entering $CLASS.$METHOD\n" if ( $debug && !$DEBUG );

  #### Process specific parameters
  my $sourceResponse = processParameters( name=>'sourceResponse', required=>1, allowUndef=>1, parameters=>\%parameters, caller=>$METHOD, response=>$response );

  #### Die if any unexpected parameters are passed
  my $unexpectedParameters = '';
  foreach my $parameter ( keys(%parameters) ) { $unexpectedParameters .= "ERROR: unexpected parameter '$parameter'\n"; }
  die("CALLING ERROR [$METHOD]: $unexpectedParameters") if ($unexpectedParameters);

  #### Return if there was a problem with the required parameters
  return $response if ( $response->{errorCode} =~ /MissingParameter/i );

  #### Check to make sure that the sourceResponse is really a Response object
  unless ( $sourceResponse =~ /BDQC::Response=HASH/ ) {
    $self->logEvent( level=>'ERROR', errorCode=>'sourceResponseNotObject', verbose=>$verbose, debug=>$debug, quiet=>$quiet,
        message=>"Parameter sourceResponse is not a Response object");
    return $self;
  }

  #### Transfer all the state information and data from the source response to self
  if ( $self->{status} eq 'OK' && $sourceResponse->{status} eq 'ERROR' ) {
    $self->{status} = $sourceResponse->{status};
    $self->{errorCode} = $sourceResponse->{errorCode};
    $self->{message} = $sourceResponse->{message};
    $self->{fullMessage} = $sourceResponse->{fullMessage};
  }

  $self->{nErrors} += $sourceResponse->{nErrors};
  $self->{nWarnings} += $sourceResponse->{nWarnings};
  $self->{nMessages} += $sourceResponse->{nMessages};

  #### Copy over errors, warnings, and messages
  if ( $sourceResponse->{nErrors} ) {
    push(@{$self->{errors}},@{$sourceResponse->{errors}});
  }
  if ( $sourceResponse->{nWarnings} ) {
    push(@{$self->{warnings}},@{$sourceResponse->{warnings}});
  }
  if ( $sourceResponse->{nMessages} ) {
    push(@{$self->{messages}},@{$sourceResponse->{messages}});
  }

  print "DEBUG: Exiting $CLASS.$METHOD\n" if ($debug);
  return $self;
}


###############################################################################
# serialize
###############################################################################
sub serialize {
  my $METHOD = 'serialize';
  print "DEBUG: Entering $CLASS.$METHOD\n" if ( $DEBUG );
  my $self = shift || die ("self not passed");
  my %parameters = @_;

  #### Create a temporary hash to serialize to JSON
  my $package = $self->package( envelopeData=>$parameters{envelopeData} );

  my @objectArray = ( $package );

  if ( $self->{respondingObject} ) {
    my %respondingObject = ();
    if ( $self->{respondingObject} =~ /^(.+)=/ ) {
      $respondingObject{class} = $1;
      $self->{respondingObject} =~ /(0x[\da-m]+)/;
      my $id = $1;
      $respondingObject{id} = $id;
      $respondingObject{version} = $VERSION;
      $respondingObject{content} = { name=>$self->{respondingObject}->{_name}, id=>$self->{respondingObject}->{_id} } ;
    }
    push( @objectArray, \%respondingObject );
  }

  my $buffer = '';
  $buffer = encode_json(\@objectArray);

  print "DEBUG: Exiting $CLASS.$METHOD\n" if ( $DEBUG );
  return $buffer;
}


###############################################################################
# package
###############################################################################
sub package {
  my $METHOD = 'package';
  print "DEBUG: Entering $CLASS.$METHOD\n" if ( $DEBUG );
  my $self = shift || die ("self not passed");
  my %parameters = @_;

  #### Create a temporary hash to serialize to JSON
  my %objectData = ();
  $objectData{status} = $self->{status} || '';
  $objectData{message} = $self->{message} || '';
  $objectData{fullMessage} = $self->{fullMessage} || '';
  $objectData{isValid} = $self->{isValid} || '';
  $objectData{nMessages} = $self->{nMessages} || '';
  $objectData{nWarnings} = $self->{nWarnings} || '';
  $objectData{nErrors} = $self->{nErrors} || '';
  $objectData{messageLimit} = $self->{messageLimit} || '';

  $objectData{messages} = $self->{messages};
  $objectData{warnings} = $self->{warnings};
  $objectData{errors} = $self->{errors};

  $self =~ /(0x[da-m]+)/;
  my $id = $1;

  my %object = ();
  $object{class} = $CLASS;
  $object{id} = $id;
  $object{version} = $VERSION;
  $object{content} = \%objectData;

  if ( $parameters{envelopeData} ) {
    foreach my $item ( keys(%{$parameters{envelopeData}}) ) {
      $object{$item} = $parameters{envelopeData}->{$item};
    }
  }

  print "DEBUG: Exiting $CLASS.$METHOD\n" if ( $DEBUG );
  return \%object;
}



###############################################################################
# serializeold
###############################################################################
sub serializeold {
  my $METHOD = 'serializeold';
  print "DEBUG: Entering $CLASS.$METHOD\n" if ( $DEBUG );
  my $self = shift || die ("self not passed");
  my %parameters = @_;

  #### Create a temporary hash to serialize to JSON
  my %objectData = ();
  $objectData{status} = $self->{status} || '';
  $objectData{message} = $self->{message} || '';
  $objectData{fullMessage} = $self->{fullMessage} || '';
  $objectData{isValid} = $self->{isValid} || '';
  $objectData{nErrors} = $self->{nErrors} || '';
  $objectData{nWarnings} = $self->{nWarnings} || '';
  $objectData{messageLimit} = $self->{messageLimit} || '';

  $self =~ /(0x[\da-m]+)/;
  my $id = $1;

  my %object = ();
  $object{class} = $CLASS;
  $object{id} = $id;
  $object{version} = $VERSION;
  $object{content} = \%objectData;

  if ( $parameters{envelopeData} ) {
    foreach my $item ( keys(%{$parameters{envelopeData}}) ) {
      $object{$item} = $parameters{envelopeData}->{$item};
    }
  }

  my @objectArray = ( \%object );

  if ( $self->{respondingObject} ) {
    my %respondingObject = ();
    if ( $self->{respondingObject} =~ /^(.+)=/ ) {
      $respondingObject{class} = $1;
      $self->{respondingObject} =~ /(0x[\da-m]+)/;
      my $id = $1;
      $respondingObject{id} = $id;
      $respondingObject{version} = $VERSION;
      $respondingObject{content} = { name=>$self->{respondingObject}->{_name}, id=>$self->{respondingObject}->{_id} } ;
    }
    push( @objectArray, \%respondingObject );
  }

  my $buffer = '';
  $buffer = encode_json(\@objectArray);

  print "DEBUG: Exiting $CLASS.$METHOD\n" if ( $DEBUG );
  return $buffer;
}


###############################################################################
# deserialize
###############################################################################
sub deserialize {
  my $METHOD = 'deserialize';
  print "DEBUG: Entering $CLASS.$METHOD\n" if ( $DEBUG );
  my $self = shift || die ("self not passed");
  my %parameters = @_;

  my $data = $parameters{data} || die("ERROR: ($CLASS.$METHOD) No data parameter passed");

  my $package = decode_json($data);

  $self->unpackage( package=>$package );

  print "DEBUG: Exiting $CLASS.$METHOD\n" if ( $DEBUG );
  return 1;
}


###############################################################################
# unpackage
###############################################################################
sub unpackage {
  my $METHOD = 'unpackage';
  print "DEBUG: Entering $CLASS.$METHOD\n" if ( $DEBUG );
  my $self = shift || die ("self not passed");
  my %parameters = @_;

  my $package = $parameters{package} || die("ERROR: ($CLASS.$METHOD) No package parameter passed");

  my $class = $package->{class};
  my $id = $package->{id};
  my $version = $package->{version};
  my $content = $package->{content};

  $self->{status} = $content->{status} || '';
  $self->{message} = $content->{message} || '';
  $self->{fullMessage} = $content->{fullMessage} || '';
  $self->{isValid} = $content->{isValid} || '';
  $self->{nMessages} = $content->{nMessages} || '0';
  $self->{nWarnings} = $content->{nWarnings} || '0';
  $self->{nErrors} = $content->{nErrors} || '0';
  $self->{messageLimit} = $content->{messageLimit} || '';

  $self->{messages} = $content->{messages};
  $self->{warnings} = $content->{warnings};
  $self->{errors} = $content->{errors};

  print "DEBUG: Exiting $CLASS.$METHOD\n" if ( $DEBUG );
  return 1;
}


###############################################################################
# processParameters - Standard way of processing parameters passed to a sub.
#    Note that this is not a method, but rather an exported function.
###############################################################################
sub processParameters {
  my $METHOD = 'processParameters';
  my %parameters = @_;

  #### Process optional parameter response
  my $response;
  if ( exists($parameters{response}) ) {
    $response = $parameters{response};
    delete($parameters{response});
  }

  #### Process required parameter parameters
  my $passedParameters;
  if ( exists($parameters{parameters}) ) {
    $passedParameters = $parameters{parameters};
    delete($parameters{parameters});
  }

  #### Process required parameter name
  my $caller;
  if ( exists($parameters{caller}) ) {
    $caller = $parameters{caller};
    delete($parameters{caller});
  } else {
    die("ERROR [$METHOD]: Required parameter 'caller' not specified.");
  }

  #### Process required parameter name
  my $name;
  if ( exists($parameters{name}) ) {
    $name = $parameters{name};
    delete($parameters{name});
  } else {
    die("ERROR [$METHOD]: Required parameter 'name' not specified in call from $caller.");
  }

  #### Process optional parameter required
  my $required;
  if ( exists($parameters{required}) ) {
    $required = $parameters{required};
    delete($parameters{required});
  }

  #### Process optional parameter allowUndef
  my $allowUndef;
  if ( exists($parameters{allowUndef}) ) {
    $allowUndef = $parameters{allowUndef};
    delete($parameters{allowUndef});
  }

  #### Process optional parameter default
  my $default;
  if ( exists($parameters{default}) ) {
    $default = $parameters{default};
    delete($parameters{default});
  }

  #### Process optional parameter overrideIfFalse
  my $overrideIfFalse;
  if ( exists($parameters{overrideIfFalse}) ) {
    $overrideIfFalse = $parameters{overrideIfFalse};
    delete($parameters{overrideIfFalse});
  }

  #### Die if any unexpected parameters are passed
  my $unexpectedParameters = '';
  foreach my $parameter ( keys(%parameters) ) { $unexpectedParameters .= "ERROR: unexpected parameter '$parameter'\n"; }
  die("CALLING ERROR [$METHOD]: $unexpectedParameters") if ($unexpectedParameters);

  #### Extract the value if it is there
  my $value;
  if ( exists($passedParameters->{$name}) ) {
    $value = $passedParameters->{$name};
    if ( ! defined($value) ) {
      if ( $allowUndef ) {
        #### It is undef and that is okay. Leave it undef.
      } elsif ( defined($default) ) {
        $value = $default;
      } else {
        my $message = "Required parameter '$name' was passed as undef to $caller and that is not permitted";
        if ( $response ) {
          $response->logEvent( level=>'ERROR', errorCode=>"MissingParameter_$name", message=>$message);
        } else {
          die("ERROR: $message");
        }
      }
    }

  #### Else this parameter was not specified at all, which is a code error, so die
  } else {
    if ( $required ) {
      die("ERROR: Required parameter '$name' was not passed to $caller");
    } else {
      if ( defined($default) ) {
        $value = $default;
      } else {
      #### Nothing specified, not required, no default. Leave it undef
      }
    }
  }

  #### If there is a potential override
  if ( defined($overrideIfFalse) ) {
    if ( ! $value ) {
      $value = $overrideIfFalse;
    } else {
      #### There is a value, so nothing to do
    }
  }

  #### Delete the entry after processing
  delete($passedParameters->{$name});

  return $value;
}

###############################################################################
1;
