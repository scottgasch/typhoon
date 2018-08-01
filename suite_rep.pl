#!/usr/bin/perl

$NUM_BINS = 20;


$max_bin = 0;
open(LOG, $ARGV[0]) || die "Can't open $ARGV[0], $!\n";
while(<LOG>)
{
    if ($_ =~ /^Problem \"([^\"]+)\" solved in (\d+\.\d+)/)
    {
        $name = $1;
        $time = $2;
    }
    elsif ($_ =~ /^>>>>> Problem \"([^\"]+)\" was not solved\. <<<<</)
    {
        $name = $1;
        $time = -1;
    }

    if ($name ne "")
    {
        if ($name =~ /[^\.]+\.(\d+)/)
        {
            $name = $1;
        }
        if ($time != -1)
        {
            $sec_per = 20.0 / $NUM_BINS;
            $bin_num = $time / $sec_per;
            $bin_num++;
            $bin_num = int($bin_num);
        }
        else
        {
            $bin_num = 0;
        }

        if (!defined($bin[$bin_num]))
        {
            $bin[$bin_num] = 0;
        }
        $bin[$bin_num]++;
        $ids[$bin_num] .= " " . $name;
        if ($bin[$bin_num] > $max_bin)
        {
            $max_bin = $bin[$bin_num];
        }
        $name = "";
        $time = 0;
    }
}
close(LOG);


for ($x = 1;
     $x < $NUM_BINS + 1;
     $x++)
{
    $num_star = $bin[$x] / $max_bin * 60;
    $num_star = int($num_star);
    printf "%02u..%02u sec: %s ($bin[$x])\n", $x-1, $x, "#" x $num_star;
    print  "\n       ids: $ids[$x]\n\n";
}
$num_star = $bin[0] / $max_bin * 60;
$num_star = int($num_star);
printf "not solved: %s ($bin[0])\n", "#" x $num_star;
print  "\n       ids: $ids[0]\n\n";


