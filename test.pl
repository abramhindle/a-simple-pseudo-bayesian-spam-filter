#!/usr/bin/perl
my @a=(0,0);
foreach my $file (@ARGV) {
	my $out = (system("./filter -classify < $file >> out") >> 8);
	$a[$out]++;
}
print join("\t",@a),$/;
