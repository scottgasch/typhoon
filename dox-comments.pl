#!/usr/bin/perl

for $file (@ARGV)
{
    print "$file\n";

    if (!open(FILE, $file))
    {
        print "Cannot open $file: $!\n";
        next;
    }
    
    if (-e "$file.$$")
    {
        print "$file.$$ already exists, I will not klobber.\n";
        next;
    }
    
    if (!open(OUT, ">$file.$$"))
    {
        print "Cannot open $file.$$: $!\n";
        next;
    }

    while (<FILE>)
    {
        s#^/\*\+\+#/\*\*#g;
        s#^\-\-\*/#\*\*/#g;
        print OUT;
    }
    close(FILE);
    close(OUT);
    print "processed $file\n";

    system "/bin/mv -f $file.$$ $file";
}
