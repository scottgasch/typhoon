#!/usr/bin/perl

$correct[0] = $correct[1] = 0;
$total[0] = $total[1] = 0;

while(<>)
{
    #if (/Avoid null move heuristic was (\d+)\s*\/\s*(\d+)./)
    #{
    #    $correct[0] += $1;
    #    $total[0] += $2;
    #}
    if (/ick null move heuristic rate was (\d+)\s*\((\d+)\)\s*\/\s*(\d+)./)
    {
        $correct[1] += ($1 + $2);
        $total[1] += $3;
    }
}

#print "Avoid null move tried $total[0] times, succeeded $correct[0] times.\n";
#printf "Avoid null move success rate %5.2f percent.\n", $correct[0] / $total[0] * 100.0;
print "Quick null move tried $total[1] times, succeeded $correct[1] times.\n";
printf "Quick null move success rate %5.2f percent.\n", $correct[1] / $total[1] * 100.0;

