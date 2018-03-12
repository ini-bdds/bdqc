#!/usr/bin/perl -w

use Test::More tests => 25;
use Test::Harness;
use strict;
use FindBin qw ( $Bin );
use IO::Uncompress::AnyInflate qw( anyinflate );
use lib( "$Bin/../.." );

#$|++; # do not buffer output
my $test_num = 0;
my $test_dir;
my $data_file = '';
my $data_path = '';
my $web_path = "/net/dblocal/wwwspecial/peptideatlas/tmp/";

ok( mk_test_dir(), 'Create testing dir' );
ok( open_log_file(), 'Create log file' );
ok( open_outlier_file(), 'Create outlier file' );
use_ok( 'IO::Uncompress::AnyInflate' );
#use_ok( 'File::Fetch' );
ok( import_inflate(), "Import inflate" );

# Test 1
SKIP: {
  $data_file = 'test1_UniformQlogFiles';
  $data_path = "$Bin/../testdata/$data_file";
  $test_num = 1;

    skip "Data dir does not exist, unzip ../testdata/$data_file.zip to run these tests", 5 if ( ! -e  $data_path );
    ok( run_bdqc_step_1(), 'Running bdqc stepwise - create kb' );
    ok( run_bdqc_step_2(), '                      - Calc and collate' );
    ok( run_bdqc_step_3(), '                      - Show Outliers' );
    ok( run_bdqc_step_4(), '                      - Write HTML' );
    ok( write_outliers(), "Writing outliers to test file" );
};

# Test 2
SKIP: {
  $data_file = 'test2_UniformQlogFiles_1outlier';
  $data_path = "$Bin/../testdata/$data_file";
  $test_num = 2;

    skip "Data dir does not exist, unzip ../testdata/$data_file.zip to run these tests", 5 if ( ! -e  $data_path );
    ok( run_bdqc_step_1(), 'Running bdqc stepwise - create kb' );
    ok( run_bdqc_step_2(), '                      - Calc and collate' );
    ok( run_bdqc_step_3(), '                      - Show Outliers' );
    ok( run_bdqc_step_4(), '                      - Write HTML' );
    ok( write_outliers(), "Writing outliers to test file" );
};

SKIP: {
  $data_file = 'test2_UniformQlogFiles_1outlier';
  $data_path = "$Bin/../testdata/$data_file";
  $test_num = '2a';

    skip "Data dir does not exist, unzip ../testdata/$data_file.zip to run these tests", 2 if ( ! -e  $data_path );
    ok( run_bdqc_auto(), 'Running bdqc automatic mode ' );
    ok( write_outliers(), "Writing outliers to test file" );
};

# Test 3
$data_path = "$Bin/../lib";
$test_num = '3';
ok( run_bdqc_auto(), 'Run bdqc on PM files' );

$test_num = '3a';
ok( run_bdqc_auto_list(), 'Run bdqc on PM files via list' );

$test_num = 5;
$test_num = '3b';
ok( run_bdqc_auto_glob(), 'Run bdqc on PM files via glob' );
ok( write_outliers( 'nerdy' ), "Writing outliers to test file" );

SKIP: {
  $data_path = "$Bin/../testdata/nextprot";
  $test_num = '4';

  skip "nextprot data not found, exec mkdir nextprot; cd nextprot; wget 'ftp://ftp.nextprot.org/pub/current_release/chr_reports/*' from ../testdata/ to run these tests", 1 if ( ! -e  $data_path );
  ok( run_bdqc_auto(), 'Running bdqc on nextprot chromosome files ' );
  ok( write_outliers(), "Writing outliers to test file" );
};

SKIP: {
  $test_num = '5';

    skip "No PA dir found, please view HTML files in place if desired", 1 if ( ! -e  $web_path );
    ok( cp_html_files(), 'Copying HTML files, view at http://www.peptidatlas.org/tmp/TEST[1-5][abc].hml' );
};

ok( close_files() );


######################
## Test subroutines ##
######################

sub cp_html_files {
  system( "cp $test_dir/*.html $web_path" );
  return 1;
}

sub import_inflate {
  IO::Uncompress::AnyInflate->import( qw( anyinflate ) );
  return 1;
}

sub open_log_file {
  open LOGFILE, ">$test_dir/run.log";
  return 1;
}

sub close_files {
  close LOGFILE;
  close OUTFILE;
  return 1;
}

sub open_outlier_file {
  open OUTFILE, ">$test_dir/outlier.txt";
  return 1;
}


#  bdqc --kbRootPath $test --step --dataDir $data_path
sub run_bdqc_step_1 {
  print LOGFILE "Test $test_num - No outliers, stepwise\n____________________________\n";
  print LOGFILE "\nCreate qckb:\n";
  my $output = `$Bin/../bin/bdqc --kbRootPath '$test_dir/TEST$test_num' --step --dataDirectory $data_path 2>&1`; 
  print LOGFILE $output;
  return ( -e "$test_dir/TEST$test_num.qckb.json" ) ? 1 : 0;
}

#  bdqc --kbRootPath $test --calcSig --collate --calcModel
sub run_bdqc_step_2 {
  print LOGFILE "\nCalc signatures and models, collate:\n";
  my $output = `$Bin/../bin/bdqc --kbRootPath '$test_dir/TEST$test_num'  --calcSig --collate --calcModel 2>&1`; 
  print LOGFILE $output;
  return ( -e "$test_dir/TEST$test_num.qckb.json" ) ? 1 : 0;
}
#
#  bdqc --kbRootPath $test --showOutliers
sub run_bdqc_step_3 {
  print LOGFILE "\nShow outliers:\n";
  my $output = `$Bin/../bin/bdqc --kbRootPath '$test_dir/TEST$test_num' --showOutliers 2>&1`; 
  print LOGFILE $output;
  return ( -e "$test_dir/TEST$test_num.qckb.json" ) ? 1 : 0;
  return 0;
}

#  bdqc --kbRootPath $test --writeHTML
sub run_bdqc_step_4 {
  print LOGFILE "\nWrite HTML:\n";
  my $output = `$Bin/../bin/bdqc --kbRootPath '$test_dir/TEST$test_num' --quiet --writeHTML 2>&1`; 
  $output = ( -e "$test_dir/TEST$test_num.html" ) ? "OK" : "Failed";
  print LOGFILE $output;
  print LOGFILE "\n\n#####################################\n\n";
  return ( -e "$test_dir/TEST$test_num.html" ) ? 1 : 0;
}

# bdqc auto 
sub run_bdqc_auto {
  print LOGFILE "Test $test_num - Automatic mode\n____________________________\n";
  my $output = `$Bin/../bin/bdqc --kbRootPath '$test_dir/TEST$test_num' --dataDirectory $data_path 2>&1`; 
  print LOGFILE $output;
  print LOGFILE "\n\n#####################################\n\n";
  return ( -e "$test_dir/TEST$test_num.qckb.json" ) ? 1 : 0;
}

# show outliers and write to outliers file
sub write_outliers {
  my $out = shift || 'friendly';
  print OUTFILE "Test $test_num $out:\n";
  my $output = `$Bin/../bin/bdqc --quiet --kbRootPath '$test_dir/TEST$test_num' --showOut --out $out 2>&1`; 
  print OUTFILE $output;
  print OUTFILE "\n\n#####################################\n\n";
  return 1;
}



# bdqc auto with list
sub run_bdqc_auto_list {
  print LOGFILE "Test $test_num - Automatic mode with list\n____________________________\n";
  my $list = ` find $data_path -name '*.pm' `;
  open LIST, ">$test_dir/file_list.txt";
  print LIST $list;
  close LIST;
  my $output = `$Bin/../bin/bdqc --kbRootPath '$test_dir/TEST$test_num' --inputFiles $test_dir/file_list.txt 2>&1`; 
  print LOGFILE $output;
  print LOGFILE "\n\n#####################################\n\n";
  return ( -e "$test_dir/TEST$test_num.qckb.json" ) ? 1 : 0;
}

# bdqc auto with glob
sub run_bdqc_auto_glob {
  print LOGFILE "Test $test_num - Automatic mode with glob\n____________________________\n";
  my $output = `$Bin/../bin/bdqc --kbRootPath '$test_dir/TEST$test_num' --inputFiles $test_dir/* 2>&1`; 
  print LOGFILE $output;
  print LOGFILE "\nNerdy outliers:\n";
  $output = `$Bin/../bin/bdqc --kbRootPath '$test_dir/TEST$test_num' --showOutliers --out  2>&1`; 
  print LOGFILE $output;
  print LOGFILE "\n\n#####################################\n\n";
  return ( -e "$test_dir/TEST$test_num.qckb.json" ) ? 1 : 0;
}

sub fetch_nextprot {
  if ( -e "$Bin/../testdata/nextprot" ) {
  } else {
#    $data_path = "$Bin/../testdata/$data_file";
    my $ff = File::Fetch->new( uri => 'ftp://ftp.nextprot.org/pub/current_release/chr_reports/*' );
    $ff->fetch();
  }
}



 


 

#  /bin/cp -p $test.html /net/dblocal/wwwspecial/peptideatlas/tmp/bdqc.html
#  /bin/cp -p $test.html /net/dblocal/wwwspecial/peptideatlas/tmp/bdqc/
#  View http://www.peptideatlas.org/tmp/bdqc.html

sub mk_test_dir {
  $test_dir = time();
  mkdir( $test_dir );
  return ( -e $test_dir ) ? 1 : 0;
}

sub prep_data {
  my $test = shift;
  my $data_path;
  my $gz_file;
  if ( $test eq 'test_1' ) {
    $data_path = "$Bin/../testdata/test1_UniformQlogFiles";
    if ( -e $data_path ) {
      return 1;
    } else {
      $gz_file = $data_path . '.zip';
      anyinflate( $gz_file, $data_path );
      return 1;
    }
  }
}

__DATA__
