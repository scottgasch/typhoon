#!/usr/bin/perl

$verbose = 0;
for $arg (@ARGV) {
    $verbose = 1 if ($arg eq "-v");
}

$one_correct = $two_correct = 0;
$one_beta = $two_beta = 0.0;
$one_null = $two_null = 0.0;
$one_time = $two_time = 0.0;
$one_nodes = $two_nodes = 0.0;
$one_depth = $two_depth = 0.0;
$one_extends = $two_extends = 0;
$one_hash = $two_hash = 0;
$one_useful = $two_useful = 0;

open(LOG, $ARGV[0]) || die "Can't open $ARGV[0], $!\n";
while(<LOG>)
{
    if (/^Problem \"([^\"]+)\" solved in (\d+\.\d+)/)
    {
        $one_correct++;
        $name = $1;
        $time = $2;
    }
    elsif (/^>>>>> Problem \"([^\"]+)\" was not solved\. <<<<</)
    {
        $name = $1;
        $time = -1;
    }
    elsif (/First move beta cutoff rate was (\d+\.\d+)/)
    {
        $one_beta += $1;
    }
    elsif (/Null move cutoff rate\: (\d+\.\d+)/)
    {
        $one_null += $1;
    }
    elsif (/Searched for\s+(\d+\.\d+) seconds, saw (\d+) nodes/) 
    {
        $one_time += $1;
        $one_nodes += $2;
    }
    elsif (/tellothers d(\d+)/)
    {
        $one_depth += $1;
    }
    elsif (/Extensions: \((\d+) \+, (\d+) q\+, (\d+) 1mv, (\d+) \!kmvs, (\d+) mult\+ (\d+) pawn/) 
    {
        $one_extends += $1 + $2 + $3 + $4 + $5 + $6;
    }
    elsif (/(\d+) threat, (\d+) zug, (\d+) sing, (\d+) endg, (\d+) bm, (\d+) recap/)
    {
        $one_extends += $1 + $2 + $3 + $4 + $5 + $6;
    }
    elsif (/Hashing percentages\: \((\d+\.\d+) total, (\d+\.\d+) useful\)/)
    {
        $one_hash += $1; $one_useful += $2;
    }

    if ($name ne "")
    {
        if ($name =~ /[^\.]+\.(\d+)/)
        {
            $name = $1;
        }
        $one{$name} = $time;
        $name = "";
        $time = 0;
    }
}
close(LOG);

open(LOG, $ARGV[1]) || die "Can't open $ARGV[1], $!\n";
while(<LOG>)
{
    if ($_ =~ /^Problem \"([^\"]+)\" solved in (\d+\.\d+)/)
    {
        $name = $1;
        $time = $2;
        if ($name =~ /[^\.]+\.(\d+)/)
        {
            $name = $1;
        }
        $two_correct++ if (defined($one{$name}));
    }
    elsif ($_ =~ /^>>>>> Problem \"([^\"]+)\" was not solved\. <<<<</)
    {
        $name = $1;
        $time = -1;
        if ($name =~ /[^\.]+\.(\d+)/)
        {
            $name = $1;
        }
    }
    elsif (/First move beta cutoff rate was (\d+\.\d+)/)
    {
        $two_beta += $1;
    }
    elsif (/Null move cutoff rate\: (\d+\.\d+)/)
    {
        $two_null += $1;
    }
    elsif (/Searched for\s+(\d+\.\d+) seconds, saw (\d+) nodes/) 
    {
        $two_time += $1;
        $two_nodes += $2;
    }
    elsif (/tellothers d(\d+)/)
    {
        $two_depth += $1;
    }
    elsif (/Extensions: \((\d+) \+, (\d+) q\+, (\d+) 1mv, (\d+) \!kmvs, (\d+) mult\+ (\d+) pawn/) 
    {
        $two_extends += $1 + $2 + $3 + $4 + $5 + $6;
    }
    elsif (/(\d+) threat, (\d+) zug, (\d+) sing, (\d+) endg, (\d+) bm, (\d+) recap/)
    {
        $two_extends += $1 + $2 + $3 + $4 + $5 + $6;
    }
    elsif (/Hashing percentages\: \((\d+\.\d+) total, (\d+\.\d+) useful\)/)
    {
        $two_hash += $1; $two_useful += $2;
    }

    if ($name ne "")
    {
        last if !defined $one{$name};
        $two{$name} = $time;
        $name = "";
        $time = 0;
    }
}
close(LOG);

$sigma_diff = 0;
print "\t$ARGV[0]\t$ARGV[1]\n";
foreach $x (sort keys(%one))
{
    $one{$x} = -1 if (!defined($one{$x}));
    $two{$x} = -1 if (!defined($two{$x}));

    if (($one{$x} != -1) &&
        ($two{$x} != -1))
    {
        $diff = $one{$x} - $two{$x};
        $sigma_diff += $diff;
    }
    else
    {
        if (($one{$x} == -1) &&
            ($two{$x} != -1))
        {
            $diff = 31 - $two{$x};
            $one{$x} = "---";
            $sigma_diff += $diff;
        }
        elsif (($two{$x} == -1) &&
               ($one{$x} != -1))
        {
            $diff = -(31 - $one{$x});
            $two{$x} = "---";
            $sigma_diff += $diff;
        }
        else
        {
            $one{$x} = $two{$x} = "";
            $diff = "both missed";
        }
    }
    $diff = " " if ($diff == 0);

    if ($verbose) 
    {
        if (($diff eq "both missed") || ($diff eq " "))
        {
            print "$x:\t\t" . $one{$x} . "\t" . $two{$x} . "\t\t$diff\n";
        }
        else
        {
            printf "$x:\t\t" . $one{$x} . "\t" . $two{$x} . "\t\t%+2.1f\n", $diff;
        }
    }
}

print "-" x 70;
print "\n";
print "solutions:\t$one_correct\t$two_correct\t";

if ($sigma_diff < 0)
{
    $sigma_diff = abs($sigma_diff);
    printf "$ARGV[0] by %6.2f sec\n", $sigma_diff;
}
elsif ($sigma_diff > 0)
{
    printf "$ARGV[1] by %6.2f sec\n", $sigma_diff;
}
else
{
    print "even.\n";
}

@x = keys(%one);
$one_count = $#x;
@x = keys(%two);
$two_count = $#x;

printf "1st mv betas:\t%6.3f\t%6.3f\n", 
    ($one_beta / $one_count), ($two_beta / $two_count);
printf "nullmv cutoff:\t%6.3f\t%6.3f\n", 
    ($one_null / $one_count), ($two_null / $two_count);
printf "avg depth:\t%6.3f\t%6.3f\n", 
    ($one_depth / $one_count), ($two_depth / $two_count);
printf "total hash:\t%6.3f\t%6.3f\n",
    ($one_hash / $one_count), ($two_hash / $two_count);
printf "useful hash:\t%6.3f\t%6.3f\n",
    ($one_useful / $one_count), ($two_useful / $two_count);
printf "k-avg.extend:\t%6.3f\t%6.3f\n",
    ($one_extends / $one_count) / 1024, ($two_extends / $two_count) / 1024;
printf "k-extends:\t%6u\t%6u\n",
    ($one_extends / 1024), ($two_extends / 1024);
printf "overall nps:\t%6u\t%6u\n", 
    ($one_nodes / $one_time), ($two_nodes / $two_time);
