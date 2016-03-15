x <- runif(1000);
write( x, file="z", ncolumns=1 )
system( './ut-recstats < z' );
mean(x);var(x)
