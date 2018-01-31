package BDQC::UI;

###############################################################################
# Class       : BDQC::UI
#
# Description : This class is autogenerated via generatePerlClasses.pl and
#
###############################################################################

use strict;
use warnings;

use BDQC::Response qw(processParameters);

my $CLASS = 'BDQC::UI';
my $DEBUG = 0;
my $VERBOSE = 0;
my $TESTONLY = 0;

my $VERSION = '0.0.1';

#### BEGIN CUSTOMIZED CLASS-LEVEL VARIABLES AND CODE
use Data::Dumper;


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

  #### BEGIN CUSTOMIZATION. DO NOT EDIT MANUALLY ABOVE THIS. EDIT MANUALLY ONLY BELOW THIS.


  #### END CUSTOMIZATION. DO NOT EDIT MANUALLY BELOW THIS. EDIT MANUALLY ONLY ABOVE THIS.

  #### Complain about any unexpected parameters
  my $unexpectedParameters = '';
  foreach my $parameter ( keys(%parameters) ) { $unexpectedParameters .= "ERROR: unexpected parameter '$parameter'\n"; }
  die("CALLING ERROR [$METHOD]: $unexpectedParameters") if ($unexpectedParameters);

  print "DEBUG: Exiting $CLASS.$METHOD\n" if ( $DEBUG );
  return($self);
}


sub outlierDetails {
###############################################################################
# outlierDetails
###############################################################################
  my $METHOD = 'outlierDetails';
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


sub viewOutliers {
###############################################################################
# viewOutliers
###############################################################################
  my $METHOD = 'viewOutliers';
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
  #### Die if any unexpected parameters are passed
  my $unexpectedParameters = '';
  foreach my $parameter ( keys(%parameters) ) { $unexpectedParameters .= "ERROR: unexpected parameter '$parameter'\n"; }
  die("CALLING ERROR [$METHOD]: $unexpectedParameters") if ($unexpectedParameters);

  #### Return if there was a problem with the required parameters
  return $response if ( $response->{errorCode} =~ /MissingParameter/i );

  #### Set the default state to not implemented. Do not change this. Override later
  my $isImplemented = 0;

  #### BEGIN CUSTOMIZATION. DO NOT EDIT MANUALLY ABOVE THIS. EDIT MANUALLY ONLY BELOW THIS.


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

sub parseParameters {
  # Custom sub written to avoid dependence on CGI.pm 
  my $self = shift;
  my %params;
  if ( $ENV{QUERY_STRING} ) {
    $params{cgimode} = 1;
    $params{writeHTML} = 1;
    my @params = split( /[&;]/, $ENV{QUERY_STRING} );
    for my $param ( @params ) {
      my ( $k, $v ) = split( /=/, $param );
      $params{$k} = $v;
    }
  }
  $self->{params} = \%params;
  return \%params;
}

sub getHeader {
  my $self = shift;
  return qq~Content-Type: text/html; charset=ISO-8859-1

~;
}

sub startPage {
  my $self = shift;
  my $start = qq~
<html>
 <head></head>
  <body onload="drawplot()">
  ~;
  return $start;
}

sub getModelSelect {
  my $self = shift;
  my $models = shift || return "";
  my $use_nl = shift;
  $use_nl = 1 if !defined $use_nl;
  my $sep = ( $use_nl ) ? "\n" : ' ';

  my $msel = "<select id=plotselect onchange=drawplot()>$sep";                   
  my $has_opts;
  for my $m ( sort { $a cmp $b } ( keys( %{$models} ) ) ) {
    next if $m eq 'files';
#    next if $models->{$m}->{data}->[0]->{dataType} eq 'string';
    $msel .= qq~<option id="$m">$m</option>$sep~;
    $has_opts++;
  }
  $msel .= "</select>$sep";
  return ( $has_opts ) ? $msel : '';
}

sub getModelSelectFunction {
  my $self = shift;
  my $models = shift || return "";
  my $js = qq~
  <script type="text/javascript" charset="utf-8">
  function setmodel () {
    var model = document.getElementById("ftselect").value;
    var msel = [];
  ~;
  for my $ft ( sort ( keys( %{$models} ) ) ) {
    my $mod = $models->{$ft};
    my $mselect = "'" . $self->getModelSelect( $mod, 0 ) . "'";
#    $mselect =~ s/\s//g;
#    $mselect =~ s/\n//g;
    $js .= " msel['$ft'] = $mselect;\n";
  }
  $js .= qq~
    document.getElementById("mseldiv").innerHTML = msel[model];
  }
  function showSignaturePlot( type, model ) {
//    alert( type );
//    alert( document.getElementById("ftselect").selectedIndex );
    document.getElementById("ftselect").value = type
//    alert( document.getElementById("ftselect").selectedIndex );
    setmodel();
//    alert(model);
//    alert( document.getElementById("plotselect").selectedIndex );
    var psel = document.getElementById("plotselect");
    var i;
    var txt = "All options:  \\n";
    for ( i = 0; i < psel.length; i++ ) {
      txt = txt + "\\n" + psel.options[i].value;
    }
//    alert( txt );

//    alert( document.getElementById("plotselect").value );
    document.getElementById("plotselect").value = model;
    drawplot();
//    alert( document.getElementById("plotselect").selectedIndex );
  }
  </script>
  ~;
  return $js;
}


###

sub getFiletypeSelect {
  my $self = shift;
  my $models = shift || return "";
  my $ftsel = "<select id=ftselect onchange='setmodel();drawplot()'>\n";                   
  my $has_opts;
  for my $ft ( sort ( keys( %{$models} ) ) ) {
    my $has_data = 0;
    for my $m ( keys( %{$models->{$ft}} ) ) {
      next if $m eq 'files';
      $has_data++;
    }
    next unless $has_data;

    $ftsel .= "<option id='$ft'>$ft</option>\n";
    $has_opts++;
  }
  $ftsel .= "</select>\n";
  return ( $has_opts ) ? $ftsel : '';
}


sub getPlotHTML {
  my $self = shift;
  my %args = @_;
  my $kb = $args{kb} || die;
  use Data::Dumper;
  return '' unless $args{params} && $args{models};
  $args{msel} ||= $self->getModelSelect( $args{models} );

  my %style = ( 1 => 'all',
                2 => 'suspectedoutliers',
                3 => 'Outliers' );

  $args{params}->{style} ||= 1;

#  <tr><td><b>Models to consider:</b></td><td>$args{msensitivity}</td></tr>

  my $HTML = qq~
  <div id=top_div></div>
  <h3> $args{title}</h3>

  <script type="text/javascript" src="https://cdn.plot.ly/plotly-latest.min.js"></script>
  $args{msel_fx}
  <br><br>
  <form name=plot id=plot>
  <table>
  <tr><td align=right><b>Choose FileType:</b></td><td>$args{ftsel}</td></tr>
  <tr><td><b>Select Model to View:</b></td><td>$args{msel}</td></tr>
  </table>
  </form>
  <div id="plot_div" style="width:600px;height:400px;border-style:solid;border-color:gray;border-width:2px"></div>
  <script type="text/javascript" charset="utf-8">
  function drawplot () {
    var models = [];
    var hover = [];
    var jitter = [];
    var color = [];
  ~;

  my @mkeys = sort(keys( %{$args{models}} ) );
  use Data::Dumper;
  my %fcnt;
  for my $ft ( @mkeys ) {
    my $currmodel = $args{models}->{$ft};
    $HTML .= qq~
    models['$ft'] = [];
    hover['$ft'] = [];
    jitter['$ft'] = [];
    color['$ft'] = [];
    ~;
    $fcnt{$ft} = scalar( @{$args{models}->{$ft}->{files}} );
    for my $m ( sort { lc($a) cmp lc($b) } ( keys( %{$currmodel} ) ) ) {
      next if $m eq 'files';

      my @data;
      my @flag;
      my @jitter;
      my @color;
      my $sign = 1;
  
      my $n = 'rgb(8,81,156)';
      my $e = 'rgb(255,255,0)';
      my $o = 'rgb(219,64,82)';
  
      use Data::Dumper;
      for my $d ( @{$currmodel->{$m}->{data}} ) {
        push @data, $d->{value};  
        my $hlbl = ( $d->{filetag} ) ? "$d->{filetag}: $d->{deviationFlag}" : $d->{deviationFlag};  
        $hlbl = " Value: $d->{datum}<br> $hlbl" if $d->{datum} ne $d->{value};

        push @flag, $hlbl;
        push @color, ( $d->{deviationFlag} eq 'normal' ) ? $n :
                     ( $d->{deviationFlag} eq 'outlier' ) ? $o : $e;
        push @jitter, $sign*rand(0.2)/10;
        $sign = ( $sign == 1 ) ? -1 : 1;
      }
      my @cdata;
      for my $d ( @data ) {
        $d = '' if !defined $d;
        push @cdata, $d;
      }
      my $dstr = join( ',', @cdata );
  
      $HTML .= "models['$ft']['$m'] = [$dstr]\n"; 
      my $lblstr = join( "','", @flag );
      $HTML .= "hover['$ft']['$m'] = ['$lblstr']\n"; 
      my $colorstr = join( "','", @color );
      $HTML .= "color['$ft']['$m'] = ['$colorstr']\n"; 
      my $jitterstr = join(',', @jitter );
      $HTML .= "jitter['$ft']['$m'] = [$jitterstr]\n"; 
  
    }
  }
  $HTML .= qq~

    var ft = document.getElementById("ftselect").value;
    var model = document.getElementById("plotselect").value;

    var plotdata = [
    {
      y: models[ft][model],
      x: jitter[ft][model],
      text: hover[ft][model],
      hovermode: 'closest',
      hoverinfo: 'y+text',
      type: 'scatter',
      mode: 'markers',
      marker: { color: color[ft][model] },
      showlegend: false,
      showticklabels: false,
    },
    {
      y: models[ft][model],
      type: 'box',
      boxpoints: false,
      showticklabels: false,
      hoverinfo: 'none',
      name: model,
      marker: {
       color: 'rgb(8,81,156)',
      },
//       outliercolor: 'rgba(219, 64, 82, 0.6)',
//       line: {
//         outliercolor: 'rgba(219, 64, 82, 1.0)',
//         outlierwidth: 2
//       }
//      }
    }
    ]

    Plotly.newPlot('plot_div', plotdata, { showlegend: false, hovermode: 'closest', xaxis: { showticklabels: false } } );
  }

  function toggleView ( type ) {
    if ( document.getElementById(type).style.display == 'none' ) {
      document.getElementById(type).style.display = 'inline-block';
    } else {
      document.getElementById(type).style.display = 'none';
    }
  }
  </script>
  <h3 style=text-decoration:underline> File types with outliers</ul> </h3>
  ~;
  my $outliers = $args{outliers};
  my $tmpl = $outliers->{templates}->{friendly};
  my $fcnt;
  
  my $sp = "&nbsp;&nbsp;";
  foreach my $fileType ( sort keys(%{$outliers->{fileTypes}}) ) {
    my $nOutliers = 0;
    my $outlierHTML = '';

    foreach my $outlierFileTagName ( sort keys(%{$outliers->{fileTypes}->{$fileType}->{fileTags}}) ) {
      my $outlierFileTagList = $outliers->{fileTypes}->{$fileType}->{fileTags}->{$outlierFileTagName};
      my $nOutlierFlags = scalar(@{$outlierFileTagList});
      my $nOutlierFiles = 0;

#      $outlierHTML .= "  $outlierFileTagName is an outlier because:<br>\n";
      $outlierHTML .= $tmpl->{HeadTemplate} . "<br>\n"; 
      $outlierHTML =~ s/FILETAG/$outlierFileTagName/gm;
      $outlierHTML =~ s/NOUTLIERFLAGS/$nOutlierFlags/gm;
      $nOutlierFiles++;
      $nOutliers += $nOutlierFiles;

      foreach my $outlier ( @{$outlierFileTagList} ) {
        my $signature = $outlier->{signature};
        my $attribute = $outlier->{attribute};
        my $fsig = $kb->{signatureInfo}->{"$signature.$attribute"}->{friendlyName} || "$signature.$attribute";
        my $value = $outlier->{value};
        my $deviation = $outlier->{deviation}->{deviation};
        $deviation = sprintf( "%0.1f", $deviation);
        $value = '(null)' if ( ! defined($value) );
        $value = substr($value,0,70)."...." if ( length($value)>74 );

        my $noun_link = qq~<a href="#top_div" onclick="showSignaturePlot('$fileType','$signature.$attribute')">$fsig</a>~;
        my $side = ( $deviation < 0 ) ? 'upper' : 'lower';
        my $verb = $kb->{signatureInfo}->{"$signature.$attribute"}->{sideName}->{$side} || "different";

        my $itemHTML = $tmpl->{ItemTemplate} . "<br>\n";
        $itemHTML =~ s/NOUN/$noun_link/gm;
        $itemHTML =~ s/VERB/$verb/gm;
        $outlierHTML .= $itemHTML;
      }
    }

    if ( $nOutliers ) {
      $HTML .= qq~$sp<b>File Type: $fileType</b>, <a href="javascript:void(0);" onclick="toggleView('${fileType}_div')"> $nOutliers outliers</a> out of $fcnt{$fileType} files<br>\n~;
      $HTML .= qq~$sp$sp$sp<div id="${fileType}_div" style='display:none;border-style:dashed;border-color:gray;border-width:2px'>$outlierHTML</div><br>\n~;
    } else {
      $HTML .= qq~$sp<b>File Type: $fileType</b>, $nOutliers outliers out of $fcnt{$fileType} files <br>\n~;
      $HTML .= qq~<div width=800 id="no_div" style='display: inline'>$outlierHTML</div><br>\n~;
    }
  }
  return $HTML;
}

sub endPage {
  my $self = shift;
  my $end = qq~
  </html>
  ~;
  return $end;
}

1;
