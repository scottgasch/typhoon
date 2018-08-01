#!/usr/bin/perl

$max = 0;
while(<>)
{
    if ($_ =~ /^Problem \"([^\"]+)\" solved in/)
    {
        $k = $1;
        $k =~ s/[^\.]*\.//;
        $solved[$k]++;
        $max = $solved[$k] if ($solved[$k] > $max);
    }
}

for ($k = 0; $k < 880; $k++)
{
    if ($solved[$k] >= ($max - 3))
    {
        print "drop ecm.$k\n";
    }
    else
    {
        printf "keep ecm.$k (%u solution(s))\n", $solved[$k];
    }
}
