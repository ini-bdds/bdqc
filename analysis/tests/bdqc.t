#!/usr/bin/perl -w

use Test::More tests => 18;
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
use_ok( 'IO::Uncompress::AnyInflate' );
#use_ok( 'File::Fetch' );
ok( import_inflate(), "Import inflate" );

sub import_inflate {
  IO::Uncompress::AnyInflate->import( qw( anyinflate ) );
  return 1;
}

sub open_log_file {
  open LOGFILE, ">$test_dir/run.log";
  return 1;
}

# Test 1
SKIP: {
  $data_file = 'test1_UniformQlogFiles';
  $data_path = "$Bin/../testdata/$data_file";
  $test_num = 1;

    skip "Data dir does not exist, unzip ../testdata/$data_file.zip to run these tests", 4 if ( ! -e  $data_path );
    ok( run_bdqc_step_1(), 'Running bdqc stepwise - create kb' );
    ok( run_bdqc_step_2(), '                      - Calc and collate' );
    ok( run_bdqc_step_3(), '                      - Show Outliers' );
    ok( run_bdqc_step_4(), '                      - Write HTML' );
};

# Test 2
SKIP: {
  $data_file = 'test2_UniformQlogFiles_1outlier';
  $data_path = "$Bin/../testdata/$data_file";
  $test_num = 2;

    skip "Data dir does not exist, unzip ../testdata/$data_file.zip to run these tests", 4 if ( ! -e  $data_path );
    ok( run_bdqc_step_1(), 'Running bdqc stepwise - create kb' );
    ok( run_bdqc_step_2(), '                      - Calc and collate' );
    ok( run_bdqc_step_3(), '                      - Show Outliers' );
    ok( run_bdqc_step_4(), '                      - Write HTML' );
};

SKIP: {
  $data_file = 'test2_UniformQlogFiles_1outlier';
  $data_path = "$Bin/../testdata/$data_file";
  $test_num = '2a';

    skip "Data dir does not exist, unzip ../testdata/$data_file.zip to run these tests", 1 if ( ! -e  $data_path );
    ok( run_bdqc_auto(), 'Running bdqc automatic mode ' );
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

SKIP: {
  $data_path = "$Bin/../testdata/nextprot";
  $test_num = '4';

    skip "nextprot data not found, exec mkdir nextprot; cd nextprot; wget 'ftp://ftp.nextprot.org/pub/current_release/chr_reports/*' from ../testdata/ to run these tests", 1 if ( ! -e  $data_path );
    ok( run_bdqc_auto(), 'Running bdqc on nextprot chromosome files ' );
};

SKIP: {
  $test_num = '5';

    skip "No PA dir found, please view HTML files in place if desired", 1 if ( ! -e  $web_path );
    ok( cp_html_files(), 'Copying HTML files, view at http://www.peptidatlas.org/tmp/TEST[1-5][abc].hml' );
};


######################
## Test subroutines ##
######################

sub cp_html_files {
  system( "cp $test_dir/*.html $web_path" );
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
  $output = `$Bin/../bin/bdqc --kbRootPath '$test_dir/TEST$test_num' --showOutliers --out nerdy 2>&1`; 
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
sub authenticate {
  return $sbeams->Authenticate();
}

sub format_number {
	my $input = 123456789;
	my $formatted = $sbeams->formatScientific( output_mode => 'text',
	                                            precision => 1,
                                              number => $input
																						);
	return ( $formatted eq '1.2E8' ) ? 1 : 0;
}

sub read_fasta {
  my $file = "/tmp/.util.test.fasta";
  open ( FAS, ">$file" ) || return 0;
  my $fasta = qq~
>IPI000001
ATGATGGAHAKSGJKAGHAKGJOADFJASASDJOFASJDFOASJFDOASFDJOASJDFQWOCM
AOSJDFOASJDFMOASMDFAOSDFMOASMDFOASMDFOASMFDOASMDFOASMDFOAMSDOFM
AISDFASJDFASJDF
>IPI000002 This is one kicking protein
AOSJDFOASJDFMOASMDFAOSDFMOASMDFOASMDFOASMFDOASMDFOASMDFOAMSDOFM
ATGATGGAHAKSGJKAGHAKGJOADFJASASDJOFASJDFOASJFDOASFDJOASJDFQWOCM
AOSJDFOASJDFMOASMDFAOSDFMOASMDFOASMDFOASMFDOASMDFOASMDFOAMSDOFM
>IPI000003 This is one kicking protein
AOSJDFOASJDFMOASMDFAOSDFMOASMDFOASMDFOASMFDOASMDFOASMDFOAMSDOFM
ATGATGGAHAKSGJKAGHAKGJOADFJASASDJOFASJDFOASJFDOASFDJOASJDFQWOCM
AOSJDFOASJDFMOASMDFAOSDFMOASMDFOASMDFOASMFDOASMDFOASMDFOAMSDOFM
~;
print FAS $fasta;
close FAS;
my $fsa = $sbeams->read_fasta_file( filename => $file,
                                    acc_regex => ['^>(IPI\d+)'],
                                    verbose => 0 );
#print STDERR join( "\t", keys( %$fsa ) ) . "\n";

unlink( $file );
return 1;
}

sub delete_key {
  $sbeams->deleteSessionAttribute( key => $key );
  my $newval = $sbeams->getSessionAttribute( key => $key );
  return ( !defined $newval );
}
#qw(anyinflate $AnyInflateError)' );

#   use IO::Uncompress::AnyInflate qw(anyinflate $AnyInflateError) ;
#    anyinflate $input_filename_or_reference => $output_filename_or_reference [,OPTS] 
#        or die "anyinflate failed: $AnyInflateError\n";

#ok( authenticate(), 'Authenticate login' );
#ok( format_number(), 'Format number test' );
#ok( read_fasta(), 'read fasta file test' );

sub breakdown {
 # Put clean-up code here
}
END {
  breakdown();
} # End END
