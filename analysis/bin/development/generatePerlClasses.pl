#!/usr/bin/perl

###############################################################################
# Program     : generatePerlClasses.pl
# Author      : Eric Deutsch <edeutsch@systemsbiology.org>
#
# Description : This program reads a classes definition file and generates the
#               classes needed for that API in Perl
#
###############################################################################

$| = 1;

my $VERSION = "0.1.0";

use strict;
use Getopt::Long;
use FindBin;
use DirHandle;

use lib "$FindBin::Bin/../../lib";

use BDQC::Response qw(processParameters);

#### Set program name and usage banner
my $CLASS = $FindBin::Script;
my $USAGE = <<EOU;
Usage: $CLASS [OPTIONS]
Options:
  --help              This message
  --verbose n         Set verbosity level.  default is 0
  --quiet             Set flag to print nothing at all except errors
  --debug n           Set debug flag
  --testonly          If set, nothing is actually altered

  --inputFile         Name of the .ooapi file that defines the API
  --destination       Directory in which to write the generated classes
  --class             If specified, only the one class will be (re)generated

 e.g.:  $CLASS --inputFile PROXI.ooapi

EOU


#### Process options
my %OPTIONS;
unless (GetOptions(\%OPTIONS,"help","verbose:s","quiet","debug:s","testonly",
    "inputFile:s", "destination:s", "class:s", 
  )) {
  print "$USAGE";
  exit;
}

#### If no parameters are given, print usage information
if ($OPTIONS{help}){
  print "$USAGE";
  exit(9);
}

my $VERBOSE = $OPTIONS{"verbose"} || 0;
my $QUIET = $OPTIONS{"quiet"} || 0;
my $DEBUG = $OPTIONS{"debug"} || 0;
my $TESTONLY = $OPTIONS{'testonly'} || 0;

if ($DEBUG) {
  print "Options settings:\n";
  print "  VERBOSE = $VERBOSE\n";
  print "  QUIET = $QUIET\n";
  print "  DEBUG = $DEBUG\n";
  print "  TESTONLY = $TESTONLY\n";
}


###############################################################################
# Set Global Variables and execute main()
###############################################################################
my $mainResult = main();
if ( $mainResult->{status} eq 'OK' ) {
  exit(0);
} else {
  exit(100);
}


###############################################################################
# Main Program:
###############################################################################
sub main {

  #### Set up default flags
  my $verbose = $VERBOSE;
  my $debug = $DEBUG;
  my $quiet = $QUIET;
  my $testOnly = $TESTONLY;
  my $outputDestination = 'STDERR';

  #### Set up a response object
  my $response = BDQC::Response->new();

  #### Progress information
  if ($VERBOSE) {
    $response->logEvent( level=>'INFO', minimumVerbosity=>1, verbose=>$verbose, debug=>$debug, quiet=>$quiet, outputDestination=>$outputDestination,
       message=>"$CLASS Start");
  }

  #### Read the inputFile if available or quit
  my $ooapiDefinition;
  if ( $OPTIONS{inputFile} ) {
    my $result = readOoapifile( inputFile=>$OPTIONS{inputFile}, verbose=>$verbose, debug=>$debug, quiet=>$quiet, outputDestination=>$outputDestination );
    if ( $result->{status} eq 'OK' ) {
      $ooapiDefinition = $result->{ooapiDefinition};
    } else {
      $response->mergeResponse( sourceResponse=>$result );
      return $response;
    }
  } else {
    $response->logEvent( status=>'ERROR', level=>'ERROR', errorCode=>'NoInputFile', verbose=>$verbose, debug=>$debug, quiet=>$quiet, outputDestination=>$outputDestination, 
       message=>"Parameter --inputFile must be provided. Use --help for more information");
    return $response;
  }

  #### Set the destination
  my $destination = './';
  if ( $OPTIONS{destination} ) {
    if ( -d $destination ) {
      $destination = $OPTIONS{destination};
    } else {
      $response->logEvent( status=>'ERROR', level=>'ERROR', errorCode=>'InvalidDestination', verbose=>$verbose, debug=>$debug, quiet=>$quiet, outputDestination=>$outputDestination, 
        message=>"Destination '$destination' is not a valid directory");
      return $response;
    }
  }
  $response->logEvent( level=>'INFO', minimumVerbosity=>1, verbose=>$verbose, debug=>$debug, quiet=>$quiet, outputDestination=>$outputDestination,
    message=>"Writing to output directory '$destination'");

  #### Generate the classes
  my $result = generateOoapi( ooapiDefinition=>$ooapiDefinition, destination=>$destination, class=>$OPTIONS{class}, verbose=>$verbose, debug=>$debug, quiet=>$quiet, outputDestination=>$outputDestination );
  if ( $result->{status} ne 'OK' ) {
    return $response;
  }


  #### Progress information
  $response->logEvent( level=>'INFO', minimumVerbosity=>1, verbose=>$verbose, debug=>$debug, quiet=>$quiet, outputDestination=>$outputDestination,
     message=>"$CLASS Finished");

  return $response;
}


###############################################################################
# readOoapifile
###############################################################################
sub readOoapifile {
  my $METHOD = 'readOoapifile';
  print "DEBUG: Entering $CLASS.$METHOD\n" if ( $DEBUG );
  #my $self = shift || die ("self not passed");
  my %parameters = @_;

  #### Set up a response object
  my $response = BDQC::Response->new();

  #### Process standard parameters
  my $debug = processParameters( name=>'debug', required=>0, allowUndef=>0, default=>0, overrideIfFalse=>$DEBUG, parameters=>\%parameters, caller=>$METHOD, response=>$response );
  my $verbose = processParameters( name=>'verbose', required=>0, allowUndef=>0, default=>0, overrideIfFalse=>$VERBOSE, parameters=>\%parameters, caller=>$METHOD, response=>$response );
  my $quiet = processParameters( name=>'quiet', required=>0, allowUndef=>0, default=>0, parameters=>\%parameters, caller=>$METHOD, response=>$response );
  my $outputDestination = processParameters( name=>'outputDestination', required=>0, allowUndef=>0, default=>'STDERR', parameters=>\%parameters, caller=>$METHOD, response=>$response );
  print "DEBUG: Entering $CLASS.$METHOD\n" if ( $debug && !$DEBUG );

  #### Process specific parameters
  my $inputFile = processParameters( name=>'inputFile', required=>1, allowUndef=>0, parameters=>\%parameters, caller=>$METHOD, response=>$response );

  #### Die if any unexpected parameters are passed
  my $unexpectedParameters = '';
  foreach my $parameter ( keys(%parameters) ) { $unexpectedParameters .= "ERROR: unexpected parameter '$parameter'\n"; }
  die("CALLING ERROR [$METHOD]: $unexpectedParameters") if ($unexpectedParameters);

  #### Return if there was a problem with the required parameters
  return $response if ( $response->{errorCode} =~ /MissingParameter/i );


  #### Check to make sure the file exists
  unless ( -f $inputFile ) {
    $response->logEvent( status=>'ERROR', level=>'ERROR', errorCode=>'InputFileNotFound', verbose=>$verbose, debug=>$debug, quiet=>$quiet, outputDestination=>$outputDestination, 
       message=>"Specified inputFile '$inputFile' is not present");
    return $response;
  }

  #### If the file can't open, return an error
  unless ( open(INFILE,$inputFile) ) {
    $response->logEvent( status=>'ERROR', level=>'ERROR', errorCode=>'InputFileNotFound', verbose=>$verbose, debug=>$debug, quiet=>$quiet, outputDestination=>$outputDestination, 
       message=>"Specified inputFile '$inputFile' is present but cannot be opened");
    return $response;
  }

  #### Example informational, warning, and error messages
  $response->logEvent( level=>'INFO', minimumVerbosity=>1, verbose=>$verbose, debug=>$debug, quiet=>$quiet, outputDestination=>$outputDestination,
     message=>"Reading file '$inputFile'");

  #### load in the file to a buffer
  my @apiDefinitionLines = ();
  my $ooapiDefinition;
  my $iLine = 0;
  my $currentClass = '';
  my $currentMethod = '';

  my $spaceCount = 0;
  my $tabCount = 0;

  while ( my $line = <INFILE> ) {
    #print $line;
    $line =~ s/[\r\n]//g;
    push(@apiDefinitionLines,$line);
    $iLine++;

    #### If this is a nothing line or a comment line, then skip it
    next if ( $line =~ /^\s*$/ );
    next if ( $line =~ /^\s*\#/ );

    #### Strip off comments at end of line
    #$line =~ s/\#.*$//;

    my $indentCount = 0;
    if ( $line =~ /^(\s+)/ ) {
      my $indentChars = $1;
      $indentCount = length($indentChars);
      $line =~ s/^\s+//;
      if ( $indentChars =~ /\t/ ) {
        $tabCount++;
        if ( $spaceCount > 0 ) {
          $response->logEvent( status=>'ERROR', level=>'ERROR', errorCode=>'MixedSpacesAndTabs', verbose=>$verbose, debug=>$debug, quiet=>$quiet, outputDestination=>$outputDestination, 
            message=>"Line $iLine has a tab whereas previous lines were spaces. Must only use one");
          return $response;
        }
      } else {
        $spaceCount++;
        if ( $tabCount > 0 ) {
          $response->logEvent( status=>'ERROR', level=>'ERROR', errorCode=>'MixedSpacesAndTabs', verbose=>$verbose, debug=>$debug, quiet=>$quiet, outputDestination=>$outputDestination, 
            message=>"Line $iLine has a space whereas previous lines were tabs. Must only use one");
          return $response;
        }
      }
    }
    my @tokens = split(/\s+/,$line);
    if ( $indentCount == 0 ) {
      $currentClass = '';
      if ( $tokens[0] eq 'apiVersion' ) {
        $ooapiDefinition->{$tokens[0]} = $tokens[1];
      } elsif ( $tokens[0] eq 'option' ) {
        $ooapiDefinition->{$tokens[1]} = $tokens[2] || 1;
      } elsif ( $tokens[0] eq 'class' ) {
        $currentClass = $tokens[1];
        $ooapiDefinition->{classes}->{$currentClass}->{name} = $currentClass;
        print "INFO: Reading definition for class $currentClass\n";
      } else {
        $response->logEvent( status=>'ERROR', level=>'ERROR', errorCode=>'UnrecognizedToken', verbose=>$verbose, debug=>$debug, quiet=>$quiet, outputDestination=>$outputDestination, 
          message=>"Unrecognized token '$tokens[0]' with indentCount=$indentCount at line $iLine (line=$line)");
        return $response;
      }
    } elsif ( $indentCount == 2 ) {
      $currentMethod = '';
      if ( $tokens[0] eq 'attribute' ) {
        $ooapiDefinition->{classes}->{$currentClass}->{attributes}->{$tokens[2]}->{datatype} = $tokens[1];
        my $iToken = 3;
        while ( $iToken <= scalar(@tokens) ) {
          $ooapiDefinition->{classes}->{$currentClass}->{attributes}->{$tokens[2]}->{$tokens[$iToken]} = 1;
          $iToken++;
        }
      } elsif ( $tokens[0] eq 'constraint' ) {
        $ooapiDefinition->{classes}->{$currentClass}->{constraints}->{$tokens[2]} = $tokens[1];
      } elsif ( $tokens[0] eq 'contains' ) {
        $ooapiDefinition->{classes}->{$currentClass}->{contains}->{$tokens[2]} = $tokens[1];
      } elsif ( $tokens[0] eq 'method' ) {
        $ooapiDefinition->{classes}->{$currentClass}->{methods}->{$tokens[1]}->{name} = $tokens[1];
        $currentMethod = $tokens[1];
      } else {
        print "Unrecognized token '$tokens[0]'\n";
      }

    } elsif ( $indentCount == 4 ) {
      if ( $tokens[0] eq 'parameter' && $currentMethod ) {
        $ooapiDefinition->{classes}->{$currentClass}->{methods}->{$currentMethod}->{parameters}->{$tokens[2]} = \@tokens;
      } elsif ( $tokens[0] eq 'returns' && $currentMethod ) {
        $ooapiDefinition->{classes}->{$currentClass}->{methods}->{$currentMethod}->{returns} = $tokens[1];
      } else {
        print "Unhandled indentcount=4 token '$tokens[0]' in line '$line'\n";
      }
    } else {
      print "Unhandled indentCount=$indentCount\n";
    }
    
    
  }
  close(INFILE);

  $response->{ooapiDefinition} = $ooapiDefinition;

  #### Update the status codes and return
  $response->setState( status=>'OK', message=>"Method $METHOD completed normally");
  print "DEBUG: Exiting $CLASS.$METHOD\n" if ( $debug );
  return $response;
}


###############################################################################
# extractCustomContent
###############################################################################
sub extractCustomContent {
  my $METHOD = 'extractCustomContent';
  print "DEBUG: Entering $CLASS.$METHOD\n" if ( $DEBUG );
  #my $self = shift || die ("self not passed");
  my %parameters = @_;

  #### Set up a response object
  my $response = BDQC::Response->new();

  #### Process standard parameters
  my $debug = processParameters( name=>'debug', required=>0, allowUndef=>0, default=>0, overrideIfFalse=>$DEBUG, parameters=>\%parameters, caller=>$METHOD, response=>$response );
  my $verbose = processParameters( name=>'verbose', required=>0, allowUndef=>0, default=>0, overrideIfFalse=>$VERBOSE, parameters=>\%parameters, caller=>$METHOD, response=>$response );
  my $quiet = processParameters( name=>'quiet', required=>0, allowUndef=>0, default=>0, parameters=>\%parameters, caller=>$METHOD, response=>$response );
  my $outputDestination = processParameters( name=>'outputDestination', required=>0, allowUndef=>0, default=>'STDERR', parameters=>\%parameters, caller=>$METHOD, response=>$response );
  print "DEBUG: Entering $CLASS.$METHOD\n" if ( $debug && !$DEBUG );

  #### Process specific parameters
  my $filename = processParameters( name=>'filename', required=>1, allowUndef=>0, parameters=>\%parameters, caller=>$METHOD, response=>$response );
  my $class = processParameters( name=>'class', required=>1, allowUndef=>0, parameters=>\%parameters, caller=>$METHOD, response=>$response );
  my $ooapiDefinition = processParameters( name=>'ooapiDefinition', required=>1, allowUndef=>0, parameters=>\%parameters, caller=>$METHOD, response=>$response );

  #### Die if any unexpected parameters are passed
  my $unexpectedParameters = '';
  foreach my $parameter ( keys(%parameters) ) { $unexpectedParameters .= "ERROR: unexpected parameter '$parameter'\n"; }
  die("CALLING ERROR [$METHOD]: $unexpectedParameters") if ($unexpectedParameters);

  #### Return if there was a problem with the required parameters
  return $response if ( $response->{errorCode} =~ /MissingParameter/i );

  #### For the moment, we only check for custom code in the appropriate block for methods.
  #### Eventually it would be nice to account for all subs and make sure there aren't extract only. FIXME.

  #### Get a list of methods that might contain data to preserve
  my %methods = ();
  my $nMethods = 0;
  my @methodsToKeep = keys %{$ooapiDefinition->{classes}->{$class}->{methods}};
  push(@methodsToKeep,"new","___CLASS-LEVEL CODE");
  foreach my $method ( @methodsToKeep ) {
    $methods{$method}->{found} = 0;
    $methods{$method}->{code} = [];
    $nMethods++;
  }

  #### If there aren't any, nothing to do
  unless ( $nMethods ) {
    $response->logEvent( level=>'INFO', verbose=>$verbose, debug=>$debug, quiet=>$quiet, outputDestination=>$outputDestination, 
      message=>"This class $class has no methods so nothing to try to preserve");
    $response->setState( status=>'OK', message=>"No methods to preserve");
    print "DEBUG: Exiting $CLASS.$METHOD\n" if ( $debug );
    return $response;
  }

  #### Check if the file is there
  unless ( -e $filename ) {
    $response->logEvent( level=>'INFO', verbose=>$verbose, debug=>$debug, quiet=>$quiet, outputDestination=>$outputDestination, 
      message=>"There is no previous instance of class $class. Wanted to try to preserve some content, but nothing to do. This is fine.");
    $response->setState( status=>'OK', message=>"No previous class data to preserve");
    print "DEBUG: Exiting $CLASS.$METHOD\n" if ( $debug );
    return $response;
  }

  #### Open file
  unless ( open(INFILE,$filename) ) {
    $response->logEvent( status=>'ERROR', level=>'ERROR', errorCode=>'CannotOpenPrev', verbose=>$verbose, debug=>$debug, quiet=>$quiet, outputDestination=>$outputDestination, 
      message=>"Although it exists, we are unable to open file '$filename' for writing");
    print "DEBUG: Exiting $CLASS.$METHOD\n" if ( $debug );
    return $response;
  }

  #### Loop over all lines looking for method data to preserve
  my $foundMethod = '';
  my $captureMode = 0;
  my $pendingBuffer = '';
  while ( my $line = <INFILE> ) {

    #### If this is the beginning of a sub the start paying attention
    if ( $line =~ /^sub (.+) \{/ ) {
      $foundMethod = $1;

      #### If this is one of the methods under autogen control, then just extract the custom section
      if ( $methods{$foundMethod} || $foundMethod eq 'new' ) {
        print "INFO: Extracting custom code in method $foundMethod\n";
        $ooapiDefinition->{classes}->{$class}->{methods}->{$foundMethod}->{customCode} = $methods{$foundMethod}->{code};
        $captureMode = 0;
        $pendingBuffer = '';

      #### But if it's not under autogen control, then keep the whole method as is
      } else {
        print "INFO: Preserving custom method $foundMethod\n";
        $methods{$foundMethod}->{code} = [];
        $ooapiDefinition->{classes}->{$class}->{extraMethods}->{$foundMethod}->{customCode} = $methods{$foundMethod}->{code};
        $captureMode = 2;
        $pendingBuffer = '';
      }
    }

    #### At the beginning of the custom section, enter mode 1 and flush the buffer
    if ( $line =~ /BEGIN CUSTOMIZATION. DO NOT EDIT MANUALLY/ ) {
      $captureMode = 1;
      $pendingBuffer = '';

    #### At the end of the custom section, back to mode 0 and flush the buffer
    } elsif ( $line =~ /END CUSTOMIZATION. DO NOT EDIT MANUALLY/ ) {
      $captureMode = 0;
      $pendingBuffer = '';

    #### At the beginning of the custom section, enter mode 1 and flush the buffer
    } elsif ( $line =~ /BEGIN CUSTOMIZED CLASS-LEVEL VARIABLES AND CODE/ ) {
      $foundMethod = "___CLASS-LEVEL CODE";
      $ooapiDefinition->{classes}->{$class}->{methods}->{$foundMethod}->{customCode} = $methods{$foundMethod}->{code};
      $captureMode = 1;
      $pendingBuffer = '';

    #### At the end of the custom section, back to mode 0 and flush the buffer
    } elsif ( $line =~ /END CUSTOMIZED CLASS-LEVEL VARIABLES AND CODE/ ) {
      $captureMode = 0;
      $pendingBuffer = '';

    #### If we're capturing information
    } elsif ( $captureMode ) {
      $line =~ s/[\r\n]//g;
      if ( $captureMode == 1 ) {
        if ( $line =~ /^\s*$/ ) {
          next;
        } else {
          $captureMode = 2;
        }
      } elsif ( $captureMode == 2 ) {
        if ( $line =~ /^\s*$/ || $line =~ /^[\s\#]+$/ || $line =~ /\s*1;\s*$/ ) {
          $pendingBuffer .= "$line\n";
        } else {
          $line = "$pendingBuffer$line";
          $pendingBuffer = '';
        }
      }
      unless ( $pendingBuffer ) {
        push(@{$methods{$foundMethod}->{code}},$line);
      }
    }
  }
  close(INFILE);

  $response->{ooapiDefinition} = $ooapiDefinition;

  #### Update the status codes and return
  $response->setState( status=>'OK', message=>"Method $METHOD completed normally");
  print "DEBUG: Exiting $CLASS.$METHOD\n" if ( $debug );
  return $response;
}


###############################################################################
# generateOoapi
###############################################################################
sub generateOoapi {
  my $METHOD = 'generateOoapi';
  print "DEBUG: Entering $CLASS.$METHOD\n" if ( $DEBUG );
  #my $self = shift || die ("self not passed");
  my %parameters = @_;

  #### Set up a response object
  my $response = BDQC::Response->new();

  #### Process standard parameters
  my $debug = processParameters( name=>'debug', required=>0, allowUndef=>0, default=>0, overrideIfFalse=>$DEBUG, parameters=>\%parameters, caller=>$METHOD, response=>$response );
  my $verbose = processParameters( name=>'verbose', required=>0, allowUndef=>0, default=>0, overrideIfFalse=>$VERBOSE, parameters=>\%parameters, caller=>$METHOD, response=>$response );
  my $quiet = processParameters( name=>'quiet', required=>0, allowUndef=>0, default=>0, parameters=>\%parameters, caller=>$METHOD, response=>$response );
  my $outputDestination = processParameters( name=>'outputDestination', required=>0, allowUndef=>0, default=>'STDERR', parameters=>\%parameters, caller=>$METHOD, response=>$response );
  print "DEBUG: Entering $CLASS.$METHOD\n" if ( $debug && !$DEBUG );

  #### Process specific parameters
  my $ooapiDefinition = processParameters( name=>'ooapiDefinition', required=>1, allowUndef=>0, parameters=>\%parameters, caller=>$METHOD, response=>$response );
  my $destination = processParameters( name=>'destination', required=>1, allowUndef=>0, parameters=>\%parameters, caller=>$METHOD, response=>$response );
  my $oneClass = processParameters( name=>'class', required=>0, allowUndef=>1, parameters=>\%parameters, caller=>$METHOD, response=>$response );

  #### Die if any unexpected parameters are passed
  my $unexpectedParameters = '';
  foreach my $parameter ( keys(%parameters) ) { $unexpectedParameters .= "ERROR: unexpected parameter '$parameter'\n"; }
  die("CALLING ERROR [$METHOD]: $unexpectedParameters") if ($unexpectedParameters);

  #### Return if there was a problem with the required parameters
  return $response if ( $response->{errorCode} =~ /MissingParameter/i );

  #### Capture the API version
  my $apiVersion = $ooapiDefinition->{apiVersion};

  #### Loop over the classes, writing them out
  foreach my $class ( sort keys %{$ooapiDefinition->{classes}} ) {

    #### If we're just processing a single class, check if it's the right one
    next if ( $oneClass && $class ne $oneClass );

    print "INFO: Generate code for $class $class\n";

    #### Generate other forms of the class name
    my $fullClassName = $class;
    $fullClassName =~ s/\./::/g;
    my $classFileName = $class;
    $classFileName =~ s/\./\//g;
    $classFileName .= ".pm";

    #### Define the output file
    my $outfile = "$destination/$classFileName";

    #### Save all the hand-rolled content in the existing file
    my $extractCustomContentResult = extractCustomContent( filename=>$outfile, class=>$class, ooapiDefinition=>$ooapiDefinition, verbose=>$verbose, debug=>$debug, quiet=>$quiet, outputDestination=>$outputDestination );
    $response->mergeResponse( sourceResponse=>$extractCustomContentResult );
    return $response if ( $response->{status} ne 'OK' );

    #### Make a list of built-in methods that we need to know about when scanning for custom content
    my %builtinMethods = ( new=>1, show=>1 );
    #### If the class definition has a show, then don't use a pregen one, treat it like a regular method
    $builtinMethods{show} = 1 unless ( $ooapiDefinition->{classes}->{$class}->{methods}->{show} );

    #### Open output file
    unless ( open(OUTFILE,">$outfile") ) {
      $response->logEvent( status=>'ERROR', level=>'ERROR', errorCode=>'CannotWriteClass', verbose=>$verbose, debug=>$debug, quiet=>$quiet, outputDestination=>$outputDestination, 
        message=>"Unable to open file '$outfile' for writing");
      return $response;
    }

    #### Write out a premable
    print OUTFILE <<EOU;
package $fullClassName;

###############################################################################
# Class       : $fullClassName
#
# Description : This class is autogenerated via generatePerlClasses.pl and
#
###############################################################################

use strict;
use warnings;

use BDQC::Response qw(processParameters);

my \$CLASS = '$fullClassName';
my \$DEBUG = 0;
my \$VERBOSE = 0;
my \$TESTONLY = 0;

my \$VERSION = '$apiVersion';

#### BEGIN CUSTOMIZED CLASS-LEVEL VARIABLES AND CODE

EOU

  if ( $ooapiDefinition->{classes}->{$class}->{methods}->{"___CLASS-LEVEL CODE"}->{customCode} ) {
    foreach my $line ( @{$ooapiDefinition->{classes}->{$class}->{methods}->{"___CLASS-LEVEL CODE"}->{customCode}} ) {
      print OUTFILE "$line\n";
    }
  } else {
    print OUTFILE "\n\n";
  }

    print OUTFILE <<EOU;

#### END CUSTOMIZED CLASS-LEVEL VARIABLES AND CODE


sub new {
###############################################################################
# Constructor
###############################################################################
  my \$METHOD = 'new';
  print "DEBUG: Entering \$CLASS.\$METHOD\\n" if ( \$DEBUG );
  my \$self = shift;
  my \%parameters = \@_;
  my \$class = ref(\$self) || \$self;

  #### Create the object with any default attributes
  \$self = {
  };
  bless \$self => \$class;

  #### Process constructor object parameters
EOU
    foreach my $attribute ( sort keys %{$ooapiDefinition->{classes}->{$class}->{attributes}} ) {
      my $ucfirstAttribute = ucfirst($attribute);
      print OUTFILE <<EOU;
  my \$$attribute = processParameters( name=>'$attribute', required=>0, allowUndef=>0, parameters=>\\\%parameters, caller=>\$METHOD );
  \$self->{_$attribute} = \$$attribute;
EOU
    }

    print OUTFILE <<EOU;

  #### BEGIN CUSTOMIZATION. DO NOT EDIT MANUALLY ABOVE THIS. EDIT MANUALLY ONLY BELOW THIS.

EOU

  if ( $ooapiDefinition->{classes}->{$class}->{methods}->{new}->{customCode} ) {
    foreach my $line ( @{$ooapiDefinition->{classes}->{$class}->{methods}->{new}->{customCode}} ) {
      print OUTFILE "$line\n";
    }
  } else {
    print OUTFILE "\n\n";
  }

    print OUTFILE <<EOU;

  #### END CUSTOMIZATION. DO NOT EDIT MANUALLY BELOW THIS. EDIT MANUALLY ONLY ABOVE THIS.

  #### Complain about any unexpected parameters
  my \$unexpectedParameters = '';
  foreach my \$parameter ( keys(\%parameters) ) { \$unexpectedParameters .= "ERROR: unexpected parameter '\$parameter'\\n"; }
  die("CALLING ERROR [\$METHOD]: \$unexpectedParameters") if (\$unexpectedParameters);

  print "DEBUG: Exiting \$CLASS.\$METHOD\\n" if ( \$DEBUG );
  return(\$self);
}


EOU

    #### Make sure we don't generate another new later as if it were a method
    if ( exists($ooapiDefinition->{classes}->{$class}->{methods}->{new}) ) {
      delete($ooapiDefinition->{classes}->{$class}->{methods}->{new});
    }
    if ( exists($ooapiDefinition->{classes}->{$class}->{methods}->{"___CLASS-LEVEL CODE"}) ) {
      delete($ooapiDefinition->{classes}->{$class}->{methods}->{"___CLASS-LEVEL CODE"});
    }

    #### Generate all the getter and setter for attributes
    foreach my $attribute ( sort keys %{$ooapiDefinition->{classes}->{$class}->{attributes}} ) {
      my $ucfirstAttribute = ucfirst($attribute);
      my $datatype = '';
      my $enum = '';
      my @qualifiers = keys(%{$ooapiDefinition->{classes}->{$class}->{attributes}->{$attribute}});
      foreach my $qualifier ( @qualifiers ) {
        next if ( $qualifier eq 'name' );
        next if ( $qualifier =~ /^\s*$/ );
        my $qualifierValue = $ooapiDefinition->{classes}->{$class}->{attributes}->{$attribute}->{$qualifier};
        if ( $qualifier eq 'datatype' ) {
          $datatype = $qualifierValue;
        } elsif ( $qualifier =~ /enum\((.+)\)/ ) {
          $enum = $1;
        } else {
          print "ERROR: Unknown attribute qualifier '$qualifier' in class $class\n";
          exit;
        }
      }

      $builtinMethods{"get$ucfirstAttribute"} = 1;
      $builtinMethods{"set$ucfirstAttribute"} = 1;

      print OUTFILE <<EOU;
sub get$ucfirstAttribute {
###############################################################################
# get$ucfirstAttribute
###############################################################################
  my \$METHOD = 'get$ucfirstAttribute';
  print "DEBUG: Entering \$CLASS.\$METHOD\\n" if ( \$DEBUG );
  my \$self = shift || die("parameter self not passed");

  print "DEBUG: Exiting \$CLASS.\$METHOD\\n" if ( \$DEBUG );
  return(\$self->{_$attribute});
}


sub set$ucfirstAttribute {
###############################################################################
# set$ucfirstAttribute
###############################################################################
  my \$METHOD = 'set$ucfirstAttribute';
  print "DEBUG: Entering \$CLASS.\$METHOD\\n" if ( \$DEBUG );
  my \$self = shift || die("parameter self not passed");
  my \$value = shift;

EOU

      if ( $datatype eq 'string' ) {
        # nothing to do, anything can be a string. Could try to make sure it's not a pointer
      } elsif ( $datatype eq 'int32' ) {
        print OUTFILE <<EOU;
  #### Ensure that the value is of type int32
  unless ( \$value =~ /^\s*[\\-\\+]*\\d+\\s*\$/ ) {
    print "ERROR: Unable to set $attribute to '\$value': not valid int32\\n";
  }

EOU
      } elsif ( $datatype eq 'float' ) {
        print OUTFILE <<EOU;
  #### Ensure that the value is of type int32
  unless ( \$value =~ /^\s*[eE\+\-\.\d]+\s*$/ ) {
    print "ERROR: Unable to set $attribute to '\$value': not valid float\\n";
  }

EOU
      } else {
        print "ERROR: Unrecognized datatype '$datatype'\n";
      }

      if ( $enum ) {
        print OUTFILE <<EOU;
  #### Ensure that the value is one of the permitted ones in the enumeration
  my \$allowedValues = '$enum';
  my \@allowedValues = split(/,/,\$allowedValues);
  my \$isAllowed = 0;
  foreach my \$allowedValue ( \@allowedValues ) {
    if ( \$value eq \$allowedValue ) {
      \$isAllowed = 1;
    }
  }
  unless ( \$isAllowed ) {
    print "ERROR: Unable to set $attribute to \$value. Must be one of ($enum)\\n";
  }

EOU
      }

      print OUTFILE <<EOU;

  \$self->{_$attribute} = \$value;
  print "DEBUG: Exiting \$CLASS.\$METHOD\\n" if ( \$DEBUG );
  return 1;
}


EOU
    }


    #### Generate all the getter and setter for contains elements
    foreach my $containerClass ( sort keys %{$ooapiDefinition->{classes}->{$class}->{contains}} ) {
      my $cardinality = $ooapiDefinition->{classes}->{$class}->{contains}->{$containerClass};
      my $suffix = '';
      $suffix = 'List' if ( $cardinality eq 'listOf' );
      print OUTFILE <<EOU;
sub get$containerClass {
###############################################################################
# get$containerClass
###############################################################################
  my \$METHOD = 'get$containerClass';
  print "DEBUG: Entering \$CLASS.\$METHOD\\n" if ( \$DEBUG );
  my \$self = shift || die("parameter self not passed");

  print "DEBUG: Exiting \$CLASS.\$METHOD\\n" if ( \$DEBUG );
  
  return(\$self->{_$containerClass});
}


sub set$containerClass {
###############################################################################
# set$containerClass
###############################################################################
  my \$METHOD = 'set$containerClass';
  print "DEBUG: Entering \$CLASS.\$METHOD\\n" if ( \$DEBUG );
  my \$self = shift || die("parameter self not passed");
  my \$value = shift;

  \$self->{_$containerClass} = \$value;
  print "DEBUG: Exiting \$CLASS.\$METHOD\\n" if ( \$DEBUG );
  return 1;
}


EOU
    }



    #### Generate the methods if there are any
    foreach my $method ( sort keys %{$ooapiDefinition->{classes}->{$class}->{methods}} ) {
      print OUTFILE <<EOU;
sub $method {
###############################################################################
# $method
###############################################################################
  my \$METHOD = '$method';
  print "DEBUG: Entering \$CLASS.\$METHOD\\n" if ( \$DEBUG );
  my \$self = shift || die ("self not passed");
  my \%parameters = \@_;

  #### Define standard parameters
  my ( \$response, \$debug, \$verbose, \$quiet, \$testonly, \$outputDestination, \$rmiServer );

  {
  #### Set up a response object
  \$response = BDQC::Response->new();
  \$response->setState( status=>'NOTSET', message=>"Status not set in method \$METHOD");

  #### Process standard parameters
  \$debug = processParameters( name=>'debug', required=>0, allowUndef=>0, default=>0, overrideIfFalse=>\$DEBUG, parameters=>\\\%parameters, caller=>\$METHOD, response=>\$response );
  \$verbose = processParameters( name=>'verbose', required=>0, allowUndef=>0, default=>0, overrideIfFalse=>\$VERBOSE, parameters=>\\\%parameters, caller=>\$METHOD, response=>\$response );
  \$quiet = processParameters( name=>'quiet', required=>0, allowUndef=>0, default=>0, parameters=>\\\%parameters, caller=>\$METHOD, response=>\$response );
  \$testonly = processParameters( name=>'testonly', required=>0, allowUndef=>0, default=>0, parameters=>\\\%parameters, caller=>\$METHOD, response=>\$response );
  \$outputDestination = processParameters( name=>'outputDestination', required=>0, allowUndef=>0, default=>'STDERR', parameters=>\\\%parameters, caller=>\$METHOD, response=>\$response );
  \$rmiServer = processParameters( name=>'rmiServer', required=>0, allowUndef=>0, parameters=>\\\%parameters, caller=>\$METHOD, response=>\$response );
  print "DEBUG: Entering \$CLASS.\$METHOD\\n" if ( \$debug && !\$DEBUG );
  }
  #### Process specific parameters
EOU

    #### Create a buffer for some code snippets to be used later
    my $rmiParametersBuffer = '';

    #### Generate code for the specific parameters if there are any
    foreach my $parameter ( sort keys %{$ooapiDefinition->{classes}->{$class}->{methods}->{$method}->{parameters}} ) {
      my $ucfirstParameter = ucfirst($parameter);
      my ($dummy,$type,$name,$necessity,$constraints) = @{$ooapiDefinition->{classes}->{$class}->{methods}->{$method}->{parameters}->{$parameter}};
      my $required = 0;
      my $allowUndef = 1;
      if ( $necessity eq 'required' ) {
        $required = 1;
        $allowUndef = 0;
      }
      if ( $type eq 'attribute' ) {
        unless ( $ooapiDefinition->{classes}->{$class}->{attributes}->{$name} ) {
          print "ERROR: In method $method, parameter $parameter is not attribute\n";
          $type = "string";
        }
      } else {
        $rmiParametersBuffer .= "\$methodParameters->{$parameter} = \$$parameter;\n";
      }
      print OUTFILE <<EOU;
  my \$$parameter = processParameters( name=>'$parameter', required=>$required, allowUndef=>$allowUndef, parameters=>\\\%parameters, caller=>\$METHOD, response=>\$response );
EOU
      if ( $type eq 'attribute' ) {
        print OUTFILE <<EOU;
  if ( ! defined(\$$parameter) ) {
    \$$parameter = \$self->get$ucfirstParameter();
  } else {
    \$self->set$ucfirstParameter(\$$parameter);
  }

EOU
      }

      if ( $necessity eq 'requiredOrAvailable' ) {
        print OUTFILE <<EOU;
  if ( ! defined(\$$parameter) ) {
    \$response->logEvent( status=>'ERROR', level=>'ERROR', errorCode=>'Attribute${parameter}NotDefined', verbose=>\$verbose, debug=>\$debug, quiet=>\$quiet, outputDestination=>\$outputDestination, 
       message=>"Attribute '$parameter' must be defined");
    print "DEBUG: Exiting \$CLASS.\$METHOD\\n" if ( \$debug );
    return \$response;
  }

EOU
      }

    }

    print OUTFILE <<EOU;
  #### Die if any unexpected parameters are passed
  my \$unexpectedParameters = '';
  foreach my \$parameter ( keys(\%parameters) ) { \$unexpectedParameters .= "ERROR: unexpected parameter '\$parameter'\\n"; }
  die("CALLING ERROR [\$METHOD]: \$unexpectedParameters") if (\$unexpectedParameters);

  #### Return if there was a problem with the required parameters
  return \$response if ( \$response->{errorCode} =~ /MissingParameter/i );

EOU
  if ( $ooapiDefinition->{enableRMI} ) {
    print OUTFILE <<EOU;
  #### If the rmiServer parameter is set, then send this off to the remote server to execute
  if ( \$rmiServer ) {
    my \$methodParameters;
    \$methodParameters->{debug} = \$debug;
    \$methodParameters->{verbose} = \$verbose;
    \$methodParameters->{quiet} = \$quiet;
    $rmiParametersBuffer

    my \$result = \$self->invokeRemoteMethod( rmiServer=>\$rmiServer, methodName=>\$METHOD, methodParameters=>\$methodParameters );
    \$response->mergeResponse( sourceResponse=>\$result );
    print "DEBUG: Exiting \$CLASS.\$METHOD\\n" if ( \$debug );
    return \$response;
  }

EOU
  }

  print OUTFILE <<EOU;
  #### Set the default state to not implemented. Do not change this. Override later
  my \$isImplemented = 0;

  #### BEGIN CUSTOMIZATION. DO NOT EDIT MANUALLY ABOVE THIS. EDIT MANUALLY ONLY BELOW THIS.

EOU

  if ( $ooapiDefinition->{classes}->{$class}->{methods}->{$method}->{customCode} ) {
    foreach my $line ( @{$ooapiDefinition->{classes}->{$class}->{methods}->{$method}->{customCode}} ) {
      print OUTFILE "$line\n";
    }
  } else {
    print OUTFILE "\n\n";
  }

    print OUTFILE <<EOU;

  #### END CUSTOMIZATION. DO NOT EDIT MANUALLY BELOW THIS. EDIT MANUALLY ONLY ABOVE THIS.
  {
  if ( ! \$isImplemented ) {
    \$response->logEvent( status=>'ERROR', level=>'ERROR', errorCode=>"Method\${METHOD}NotImplemented", message=>"Method \$METHOD has not yet be implemented", verbose=>\$verbose, debug=>\$debug, quiet=>\$quiet, outputDestination=>\$outputDestination );
  }

  #### Update the status codes and return
  \$response->setState( status=>'OK', message=>"Method \$METHOD completed normally") if ( \$response->{status} eq 'NOTSET' );
  print "DEBUG: Exiting \$CLASS.\$METHOD\\n" if ( \$debug );
  }
  return \$response;
}


EOU

    }

    #### End the file with a footer
    unless ( $ooapiDefinition->{classes}->{$class}->{methods}->{show} ) {
      print OUTFILE <<EOU;
sub show {
###############################################################################
# show
###############################################################################
  my \$METHOD = 'show';
  print "DEBUG: Entering \$CLASS.\$METHOD\\n" if ( \$DEBUG );
  my \$self = shift || die ("self not passed");
  my \%parameters = \@_;

  #### Create a simple text representation of the data in the object
  my \$buffer = '';
  \$buffer .= "\$self\\n";
EOU

    #### Generate code for the specific parameters if there are any
    foreach my $attribute ( sort keys %{$ooapiDefinition->{classes}->{$class}->{attributes}} ) {
      my $ucfirstAttribute = ucfirst($attribute);
      print OUTFILE <<EOU;
  my \$$attribute = \$self->get$ucfirstAttribute() || '';
  \$buffer .= "  $attribute=\$$attribute\\n";
EOU
    }

    #### End the file with a footer
    print OUTFILE <<EOU;

  print "DEBUG: Exiting \$CLASS.\$METHOD\\n" if ( \$DEBUG );
  return \$buffer;
}


EOU
    }

    #### Create the serialize and deserialize methods
    if ( $ooapiDefinition->{serialize} ) {
      $builtinMethods{serialize} = 1;
      $builtinMethods{package} = 1;
      $builtinMethods{deserialize} = 1;
      $builtinMethods{unpackage} = 1;
      print OUTFILE <<EOU;
sub serialize {
###############################################################################
# serialize
###############################################################################
  my \$METHOD = 'serialize';
  print "DEBUG: Entering \$CLASS.\$METHOD\\n" if ( \$DEBUG );
  my \$self = shift || die ("self not passed");
  my \%parameters = \@_;

  #### Create a temporary hash to serialize to JSON
  my \$package = \$self->package( envelopeData=>\$parameters{envelopeData} );

  use JSON;
  my \$buffer = encode_json(\$package);

  print "DEBUG: Exiting \$CLASS.\$METHOD\\n" if ( \$DEBUG );
  return \$buffer;
}


sub package {
###############################################################################
# package
###############################################################################
  my \$METHOD = 'package';
  print "DEBUG: Entering \$CLASS.\$METHOD\\n" if ( \$DEBUG );
  my \$self = shift || die ("self not passed");
  my \%parameters = \@_;

  #### Create a temporary hash to serialize to JSON
  my \%objectData = ();
EOU

      #### Generate code for the specific attributes if there are any
      foreach my $attribute ( sort keys %{$ooapiDefinition->{classes}->{$class}->{attributes}} ) {
        my $ucfirstAttribute = ucfirst($attribute);
        print OUTFILE <<EOU;
  \$objectData{$attribute} = \$self->get$ucfirstAttribute() || '';
EOU
    }

      #### Continue creating the JSON
      print OUTFILE <<EOU;

  \$self =~ /(0x[\da-m]+)/;
  my \$id = \$1;

  my \%object = ();
  \$object{class} = \$CLASS;
  \$object{id} = \$id;
  \$object{version} = \$VERSION;
  \$object{content} = \\\%objectData;

  if ( \$parameters{envelopeData} ) {
    foreach my \$item ( keys(\%{\$parameters{envelopeData}}) ) {
      \$object{\$item} = \$parameters{envelopeData}->{\$item};
    }
  }

  print "DEBUG: Exiting \$CLASS.\$METHOD\\n" if ( \$DEBUG );
  return \\%object;
}


sub deserialize {
###############################################################################
# deserialize
###############################################################################
  my \$METHOD = 'deserialize';
  print "DEBUG: Entering \$CLASS.\$METHOD\\n" if ( \$DEBUG );
  my \$self = shift || die ("self not passed");
  my \%parameters = \@_;

  my \$data = \$parameters{data} || die("ERROR: (\$CLASS.\$METHOD) No data parameter passed");

  my \$package = decode_json(\$data);

  \$self->unpackage( package=>\$package );

  print "DEBUG: Exiting \$CLASS.\$METHOD\\n" if ( \$DEBUG );
  return 1;
}


sub unpackage {
###############################################################################
# unpackage
###############################################################################
  my \$METHOD = 'unpackage';
  print "DEBUG: Entering \$CLASS.\$METHOD\\n" if ( \$DEBUG );
  my \$self = shift || die ("self not passed");
  my \%parameters = \@_;

  my \$package = \$parameters{package} || die("ERROR: (\$CLASS.\$METHOD) No package parameter passed");

  my \$class = \$package->{class};
  my \$id = \$package->{id};
  my \$version = \$package->{version};
  my \$content = \$package->{content};

EOU

      #### Generate code for the specific attributes if there are any
      foreach my $attribute ( sort keys %{$ooapiDefinition->{classes}->{$class}->{attributes}} ) {
        my $ucfirstAttribute = ucfirst($attribute);
        print OUTFILE <<EOU;
  \$self->set$ucfirstAttribute(\$content->{$attribute}) if ( exists(\$content->{$attribute}) );
EOU
    }

      #### Continue creating the JSON
      print OUTFILE <<EOU;

  print "DEBUG: Exiting \$CLASS.\$METHOD\\n" if ( \$DEBUG );
  return 1;
}


EOU

    }

    #### Create the invokeRemoteMethod method
    if ( $ooapiDefinition->{enableRMI} ) {
      $builtinMethods{invokeRemoteMethod} = 1;
      print OUTFILE <<EOU;
sub invokeRemoteMethod {
###############################################################################
# invokeRemoteMethod
###############################################################################
  my \$METHOD = 'invokeRemoteMethod';
  print "DEBUG: Entering \$CLASS.\$METHOD\\n" if ( \$DEBUG );
  my \$self = shift || die ("self not passed");
  my \%parameters = \@_;

  #### Set up a response object
  my \$response = BDQC::Response->new();

  #### Process standard parameters
  my \$debug = processParameters( name=>'debug', required=>0, allowUndef=>0, default=>0, overrideIfFalse=>\$DEBUG, parameters=>\\\%parameters, caller=>\$METHOD, response=>\$response );
  my \$verbose = processParameters( name=>'verbose', required=>0, allowUndef=>0, default=>0, overrideIfFalse=>\$VERBOSE, parameters=>\\\%parameters, caller=>\$METHOD, response=>\$response );
  my \$quiet = processParameters( name=>'quiet', required=>0, allowUndef=>0, default=>0, parameters=>\\\%parameters, caller=>\$METHOD, response=>\$response );
  my \$outputDestination = processParameters( name=>'outputDestination', required=>0, allowUndef=>0, default=>'STDERR', parameters=>\\\%parameters, caller=>\$METHOD, response=>\$response );
  print "DEBUG: Entering \$CLASS.\$METHOD\\n" if ( \$debug && !\$DEBUG );

  #### Process specific parameters
  my \$methodName = processParameters( name=>'methodName', required=>1, allowUndef=>0, parameters=>\\\%parameters, caller=>\$METHOD, response=>\$response );
  my \$methodParameters = processParameters( name=>'methodParameters', required=>1, allowUndef=>0, parameters=>\\\%parameters, caller=>\$METHOD, response=>\$response );
  my \$rmiServer = processParameters( name=>'rmiServer', required=>1, allowUndef=>0, parameters=>\\\%parameters, caller=>\$METHOD, response=>\$response );

  #### Die if any unexpected parameters are passed
  my \$unexpectedParameters = '';
  foreach my \$parameter ( keys(\%parameters) ) { \$unexpectedParameters .= "ERROR: unexpected parameter '\$parameter'\\n"; }
  die("CALLING ERROR [\$METHOD]: \$unexpectedParameters") if (\$unexpectedParameters);

  #### Return if there was a problem with the required parameters
  return \$response if ( \$response->{errorCode} =~ /MissingParameter/i );

  #### Put the method and parameters in a hash
  my \$envelopeData;
  \$envelopeData->{methodName} = \$methodName;
  \$envelopeData->{methodParameters} = \$methodParameters;

  #### First serialize myself
  my \$serializedSelf = \$self->serialize( envelopeData=>\$envelopeData );

  #### Then transmit myself to the target server to invoke the requested method
  use LWP::UserAgent;
  use HTTP::Request::Common;
  my \$url = \$rmiServer->getProxiUrl();
  my \$userAgent = LWP::UserAgent->new();
  my \$httpResponse = \$userAgent->request( POST \$url, Content_Type => 'multipart/form-data', Content => [ data => \$serializedSelf ] );

  if (\$httpResponse->is_success) {
    print "** Response is:".\$httpResponse->decoded_content();
    my \$objectPackageArray = decode_json(\$httpResponse->decoded_content());
    my \$firstPackage = shift(\@{\$objectPackageArray});
    \$response->unpackage( package=>\$firstPackage );
    my \$secondPackage = shift(\@{\$objectPackageArray});
    \$self->unpackage( package=>\$secondPackage );

  } else {
    #print "** ERROR: Response is:\\n";
    #print \$httpResponse->status_line, "\\n";
    #print \$httpResponse->decoded_content();
    if ( \$httpResponse->status_line =~ /404/ ) {
      \$response->logEvent( status=>'ERROR', level=>'ERROR', errorCode=>"404FromRemoteHost", verbose=>\$verbose, debug=>\$debug, quiet=>\$quiet, outputDestination=>\$outputDestination, 
        message=>"Remote web server reports that PROXI API endpoint URL '".\$rmiServer->getProxiUrl()."' does not exist.");
    } elsif ( \$httpResponse->status_line =~ /500 Can't connect/ ) {
     \$response->logEvent( status=>'ERROR', level=>'ERROR', errorCode=>"RemoteHostNotResponding", verbose=>\$verbose, debug=>\$debug, quiet=>\$quiet, outputDestination=>\$outputDestination, 
        message=>"Remote web server for URL '".\$rmiServer->getProxiUrl()."' is not responding at all: ".\$httpResponse->status_line);
    } else {
      \$response->logEvent( status=>'ERROR', level=>'ERROR', errorCode=>"BadResponseFromRemote", verbose=>\$verbose, debug=>\$debug, quiet=>\$quiet, outputDestination=>\$outputDestination, 
        message=>"Bad response of unknown type from remote PROXI server");
    }
  }

  #### Update the status codes and return
  \$response->setState( status=>'OK', message=>"Method \$METHOD completed normally") unless ( \$response->{status} eq 'ERROR' );
  print "DEBUG: Exiting \$CLASS.\$METHOD\\n" if ( \$debug );
  return \$response;
}


EOU

    }

    #### If there were any additional custom methods, dump those
    if ( $ooapiDefinition->{classes}->{$class}->{extraMethods} ) {
      foreach my $foundMethod ( sort keys(%{$ooapiDefinition->{classes}->{$class}->{extraMethods}}) ) {
        next if ( $builtinMethods{$foundMethod} );
        foreach my $line ( @{$ooapiDefinition->{classes}->{$class}->{extraMethods}->{$foundMethod}->{customCode}} ) {
          print OUTFILE "$line\n";
        }
      }
    }

    #### End the file with a footer
    print OUTFILE <<EOU;

###############################################################################
1;
EOU

    #### Close file
    close(OUTFILE);
  }

  #### Update the status codes and return
  $response->setState( status=>'OK', message=>"Method $METHOD completed normally");
  print "DEBUG: Exiting $CLASS.$METHOD\n" if ( $debug );
  return $response;
}
 
