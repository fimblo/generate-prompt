#!/usr/bin/perl

use strict;
use warnings;

my %configurations = (
                      'MAN'    => 'generate-prompt.1.org',
                      'GITHUB' => 'README.org'
                     );

foreach my $marker (keys %configurations) {
  my $content = filter_content('source-instructions.org', $marker);
  my $output_file = $configurations{$marker};

  open my $fh, '>', $output_file or die "Can't open file: $!";
  print $fh $content;
  close $fh;
}



sub filter_content {
  my ($input_file, $marker) = @_;
  open my $fh, '<', $input_file or die "Can't open file: $!";
  my @lines = <$fh>;
  close $fh;

  my $skip = 0;
  my @filtered_lines;

  foreach my $line (@lines) {
    if ($line =~ /#\+BEGIN_${marker}_ONLY/) {
      $skip = 0;
    }
    elsif ($line =~ /#\+END_${marker}_ONLY/) {
      $skip = 1;
    }
    elsif (!$skip) {
      if ($marker eq 'MAN') {
        my $tmp = $line;
        chomp $tmp;
        if ($tmp =~ /\S/
            && $tmp !~ /\s*\*/
            && $tmp !~ /\s*#\+/) {
          $line = $tmp . " \\\\" . "\n";
        }
      }
      push(@filtered_lines, $line);
    }
  }

  return join("", @filtered_lines);
}
