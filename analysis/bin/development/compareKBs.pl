#!/usr/bin/env perl
#
###############################################################################
# Program       compareKBs.pl
# Author        Eric Deutsch <edeutsch@systemsbiology.org>
# Date          2018-07-30
#
# Description : This program compares two KBs to look for differences
#
#  compareKBs.pl
#
###############################################################################

use strict;
use warnings;
$| = 1;

use Getopt::Long;
use FindBin;
use Data::Dumper;
use Storable qw( dclone );

use lib "$FindBin::Bin/../../lib";

use BDQC::KB;
use BDQC::UI;
my $ui = new BDQC::UI;


main();
exit 0;


###############################################################################
sub main {

  my $response = BDQC::Response->new();
  $response->setState( status=>'OK', message=>"Starting BDQC");

  #### Create and read Quality Control Knowledge Base object 1
  my $qckb1 = BDQC::KB->new();
  my $result = $qckb1->createKb();
  $response->mergeResponse( sourceResponse=>$result );

  $result = $qckb1->loadKb( kbRootPath=>"test_twothree" );
  $response->mergeResponse( sourceResponse=>$result );
  my $kb1 = $qckb1->getQckb();


  #### Create and read Quality Control Knowledge Base object 2
  my $qckb2 = BDQC::KB->new();
  $result = $qckb2->createKb();
  $response->mergeResponse( sourceResponse=>$result );

  $result = $qckb2->loadKb( kbRootPath=>"test_threetwo" );
  $response->mergeResponse( sourceResponse=>$result );
  my $kb2 = $qckb2->getQckb();


  #### Custom compare

  #### Organize files
  my $fileType = "txt";
  my $counter = 0;
  my $files;
  foreach my $file ( @{$kb1->{fileTypes}->{$fileType}->{fileTagList}} ) {
    $file =~ s/dir\d://;
    $files->{$file}->{kb1} = $counter;
    $counter++;
  }
  print "Found $counter files\n";

  #### Custom compare
  $counter = 0;
  foreach my $file ( @{$kb2->{fileTypes}->{$fileType}->{fileTagList}} ) {
    $file =~ s/dir\d://;
    $files->{$file}->{kb2} = $counter;
    $counter++;
  }
  print "Found $counter files\n";
  my $nFiles = $counter;


  #### Compare signatures
  foreach my $signature ( keys(%{$kb1->{fileTypes}->{$fileType}->{signatures}}) ) {
    foreach my $metric ( keys(%{$kb1->{fileTypes}->{$fileType}->{signatures}->{$signature}}) ) {
      print "== Signature $signature - $metric\n";
      foreach my $file ( keys(%{$files}) ) {
        my $offset1 = $files->{$file}->{kb1};
        my $offset2 = $files->{$file}->{kb2};
        my $value1 = $kb1->{fileTypes}->{$fileType}->{signatures}->{$signature}->{$metric}->{values}->[$offset1];
        my $value2 = $kb2->{fileTypes}->{$fileType}->{signatures}->{$signature}->{$metric}->{values}->[$offset2];

	if ( defined($value1) && $value1 =~ /HASH/ ) {
          my $tmp = "";
	  foreach my $key ( sort(keys(%{$value1})) ) {
	    $tmp .= "$key--$value1->{$key},";
	  }
          $value1 = $tmp;
        }
	if ( defined($value2) && $value2 =~ /HASH/ ) {
          my $tmp = "";
	  foreach my $key ( sort(keys(%{$value2})) ) {
	    $tmp .= "$key--$value2->{$key},";
	  }
          $value2 = $tmp;
        }


        #if ( $metric eq 'meanAsciiValue' ) {
	#  print "offset1=$offset1 , offset2=$offset2\n";
	#  print "value1=$value1 , value2=$value2\n";
        #}
        next if ( !defined($value1) && !defined($value2) );
        if ( ( !defined($value1) && defined($value2) ) ||
             ( !defined($value2) && defined($value1) ) ||
             $value1 ne $value2 ) {
	  $value1 = "NULL" unless ( defined($value1) );
	  $value2 = "NULL" unless ( defined($value2) );
	  #if ( $metric eq 'columns.1.nDiscreteValues' ) {
          #if ( $value1 !~ /HASH/ && $value2 !~ /HASH/ ) {
            print "Value mismatch for file $file:\n";
            print "  $value1 =/= $value2\n";
	  #}
        }

      }

    }

  }


  #### If we are in an error state, show the results and exit abnormally
  if ( $response->{status} ne 'OK' ) {
    print "==============================\n";
    print "bdqc.pl terminated with errors:\n";
    print $response->show();
    exit 12;
  }

  return;
}


