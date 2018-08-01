#!/usr/bin/perl

@data = ( " main.o root.o search.o draw.o hash.o dynamic.o ",
          " generate.o see.o move.o movesup.o ",
          " eval.o pawnhash.o bitboard.o ",
          " command.o script.o input.o vars.o util.o list.o fbsd.o gamelist.o ",
          " board.o ics.o san.o fen.o book.o bench.o mersenne.o ",
          " piece.o sig.o data.o probe.o egtb.o" );

$num_permutations = factorial(scalar @data);
for ($i = 0; $i < $num_permutations; $i++)
{
    open(OBJS, ">_object_files.$i") || die $!;
    print OBJS "OBJS=\t\t";
    my @permutation = @data[n2perm($i, $#data)];
    print OBJS "@permutation\n";
    close(OBJS);
    system("cp _object_files.$i _objs_");
    system("make");
    system("cp typhoon typhoon.$i");
}

# Utility function: factorial with memoizing
BEGIN {
    my @fact = (1);
sub factorial($) {
    my $n = shift;
    return $fact[$n] if defined $fact[$n];
    $fact[$n] = $n * factorial($n - 1);
}
}

# n2pat($N, $len) : produce the $N-th pattern of length $len
sub n2pat 
{
    my $i   = 1;
    my $N   = shift;
    my $len = shift;
    my @pat;
    while ($i <= $len + 1) {   # Should really be just while ($N) { ...
        push @pat, $N % $i;
        $N = int($N/$i);
        $i++;
    }
    return @pat;
}

# pat2perm(@pat) : turn pattern returned by n2pat() into
# permutation of integers.  XXX: splice is already O(N)
sub pat2perm 
{
    my @pat    = @_;
    my @source = (0 .. $#pat);
    my @perm;
    push @perm, splice(@source, (pop @pat), 1) while @pat;
    return @perm;
}

# n2perm($N, $len) : generate the Nth permutation of $len objects
sub n2perm {
    pat2perm(n2pat(@_));
}
