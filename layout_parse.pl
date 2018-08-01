#!/usr/bin/perl

opendir(DIR, ".") || die;
@files = readdir(DIR);
closedir(DIR);

$max = 0;
$maxpos = -1;
$sum = 0;
$min = 99999999;
$minpos = -1;
$count = 0;

for $file(@files)
{
    if ($file =~ /(\d+)typhoon\.log/)
    {
        $num = $1;
        open(FILE, $file);
        while(<FILE>)
        {
            if (/\((\d+) nps\)\./)
            {
                $count++;
                $speed = $1;
                $sum += $speed;
                if ($speed > $max)
                {
                    $maxpos = $num;
                    $max = $speed;
                }
                if ($speed < $min)
                {
                    $minpos = $num;
                    $min = $speed;
                }
            }
        }
        close(FILE);
    }
}

printf "max $max at $maxpos, min $min at $minpos, avg %f\n", ($sum/$count);
