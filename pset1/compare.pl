#! /usr/bin/perl -w
use Time::HiRes;
use Fcntl qw(F_GETFL F_SETFL O_NONBLOCK);
use POSIX;

my($Red, $Redctx, $Green, $Cyan, $Off) = ("\x1b[01;31m", "\x1b[0;31m", "\x1b[01;32m", "\x1b[01;36m", "\x1b[0m");
$Red = $Redctx = $Green = $Cyan = $Off = "" if !-t STDERR || !-t STDOUT;

$SIG{"CHLD"} = sub {};
my($run61_pid);

sub run_sh61_pipe ($$;$) {
    my($text, $fd, $size) = @_;
    my($n, $buf) = (0, "");
    return $text if !defined($fd);
    while ((!defined($size) || length($text) <= $size)
           && defined(($n = POSIX::read($fd, $buf, 8192)))
           && $n > 0) {
        $text .= substr($buf, 0, $n);
    }
    return $text;
}

sub run_sh61 ($;%) {
    my($command, %opt) = @_;
    my($outfile) = exists($opt{"stdout"}) ? $opt{"stdout"} : undef;
    my($size_limit_file) = exists($opt{"size_limit_file"}) ? $opt{"size_limit_file"} : $outfile;
    $size_limit_file = [$size_limit_file] if $size_limit_file && !ref($size_limit_file);
    my($size_limit) = exists($opt{"size_limit"}) ? $opt{"size_limit"} : undef;
    my($dir) = exists($opt{"dir"}) ? $opt{"dir"} : undef;
    if (defined($dir) && $size_limit_file) {
        $dir =~ s{/+$}{};
        $size_limit_file = [map { m{^/} ? $_ : "$dir/$_" } @$size_limit_file];
    }
    pipe(OR, OW);
    fcntl(OR, F_SETFL, fcntl(OR, F_GETFL, 0) | O_NONBLOCK);
    1 while waitpid(-1, WNOHANG) > 0;

    $run61_pid = fork();
    if ($run61_pid == 0) {
        setpgrp(0, 0);
        defined($dir) && chdir($dir);

        my($fn) = exists($opt{"stdin"}) ? $opt{"stdin"} : "/dev/null";
        my($fd) = POSIX::open($fn, O_RDONLY);
        POSIX::dup2($fd, 0);
        POSIX::close($fd) if $fd != 0;

        close(OR);
        open(OW, ">", $outfile) || die if defined($outfile) && $outfile ne "pipe";
        POSIX::dup2(fileno(OW), 1);
        POSIX::dup2(fileno(OW), 2);
        close(OW) if fileno(OW) != 1 && fileno(OW) != 2;

        { exec($command) };
        print STDERR "error trying to run $command: $!\n";
        exit(1);
    }

    my($before) = Time::HiRes::time();
    my($died) = 0;
    my($max_time) = exists($opt{"time_limit"}) ? $opt{"time_limit"} : 0;
    my($out, $buf, $nb) = ("", "");

    close(OW);

    eval {
        do {
            Time::HiRes::usleep(300000);
            die "!" if waitpid($run61_pid, WNOHANG) > 0;
            if (defined($size_limit) && $size_limit_file && @$size_limit_file) {
                my($len) = 0;
                $out = run_sh61_pipe($out, fileno(OR), $size_limit);
                foreach my $fname (@$size_limit_file) {
                    $len += ($fname eq "pipe" ? length($out) : -s $fname);
                }
                if ($len > $size_limit) {
                    $died = "output file size too large, expected <= $size_limit";
                    die "!";
                }
            }
        } while (Time::HiRes::time() < $before + $max_time);
        $died = sprintf("timeout after %.2fs", $max_time)
            if waitpid($run61_pid, WNOHANG) <= 0;
    };

    my($delta) = Time::HiRes::time() - $before;

    my($answer) = exists($opt{"answer"}) ? $opt{"answer"} : {};
    $answer->{"command"} = $command;
    $answer->{"time"} = $delta;

    if (exists($opt{"nokill"})) {
        $answer->{"pgrp"} = $run61_pid;
    } else {
        kill 9, -$run61_pid;
    }
    $run61_pid = 0;

    if ($died) {
        $answer->{"error"} = $died;
        close(OR);
        return $answer;
    }

    if ($outfile) {
        $out = "";
        close(OR);
        open(OR, "<", (defined($dir) ? "$dir/$outfile" : $outfile));
    }
    $answer->{"output"} = run_sh61_pipe($out, fileno(OR), $size_limit);
    close(OR);

    return $answer;
}

sub test_runnable ($) {
    my($number) = @_;
    return !@ARGV || grep {
        $_ eq $number
          || ($_ =~ m{^(\d+)-(\d+)$} && $number >= $1 && $number <= $2)
          || ($_ =~ m{(?:^|,)$number(,|$)})
      } @ARGV;
}

sub read_expected ($) {
    my($fname) = @_;
    open(EXPECTED, $fname) or die;

    my(@expected);
    my($line, $skippable, $sort) = (0, 0, 0);
    while (defined($_ = <EXPECTED>)) {
        ++$line;
        if (m{^//! \?\?\?\s+$}) {
            $skippable = 1;
        } elsif (m{^//!!SORT\s+$}) {
            $sort = 1;
        } elsif (m{^//! }) {
            s{^....(.*?)\s*$}{$1};
            my($m) = {"t" => $_, "line" => $line, "skip" => $skippable,
                      "r" => "", "match" => []};
            foreach my $x (split(/(\?\?\?|\?\?\{.*?\}(?:=\w+)?\?\?)/)) {
                if ($x eq "???") {
                    $m->{r} =~ s{(?:\\ )+$}{\\s+};
                    $m->{r} .= ".*";
                } elsif ($x =~ /\A\?\?\{(.*)\}=(\w+)\?\?\z/) {
                    $m->{r} .= "(" . $1 . ")";
                    push @{$m->{match}}, $2;
                } elsif ($x =~ /\A\?\?\{(.*)\}\?\?\z/) {
                    my($contents) = $1;
                    $m->{r} =~ s{(?:\\ )+$}{\\s+};
                    $m->{r} .= "(?:" . $contents . ")";
                } else {
                    $m->{r} .= quotemeta($x);
                }
            }
            push @expected, $m;
            $skippable = 0;
        }
    }
    return {"l" => \@expected, "nl" => scalar(@expected),
            "skip" => $skippable, "sort" => $sort};
}

sub read_actual ($) {
    my($fname) = @_;
    open(ACTUAL, $fname) or die;
    my(@actual);
    while (defined($_ = <ACTUAL>)) {
        chomp;
        push @actual, $_;
    }
    close ACTUAL;
    \@actual;
}

sub run_compare ($$$$$) {
    my($actual, $exp, $aname, $ename, $outname) = @_;
    @$actual = sort { $a cmp $b } @$actual if $exp->{sort};
    $outname .= " " if $outname;

    my(%chunks);
    my($a, $e) = (0, 0);
    for (; $a != @$actual; ++$a) {
        $_ = $actual->[$a];
        if ($e == $exp->{nl} && !$exp->{skip}) {
            print STDERR "$Red${outname}FAIL: Too much output (expected only ", $exp->{nl}, " output lines)$Off\n";
            print STDERR "  $Redctx$aname:", $a + 1, ": Got `", $_, "`$Off\n";
            return 1;
        } elsif ($e == $exp->{nl}) {
            next;
        }

        my($rex) = $exp->{l}->[$e]->{r};
        while (my($k, $v) = each %chunks) {
            $rex =~ s{\\\?\\\?$k\\\?\\\?}{$v}g;
        }
        if (m{\A$rex\z}) {
            for (my $i = 0; $i < @{$exp->{l}->[$e]->{match}}; ++$i) {
                $chunks{$exp->{l}->[$e]->{match}->[$i]} = ${$i + 1};
            }
            ++$e;
        } elsif (!$exp->{l}->[$e]->{skip}) {
            print STDERR "$Red${outname}FAIL: Unexpected output starting on line ", $a + 1, "$Off\n";
            print STDERR "  $Redctx$ename:", $exp->{l}->[$e]->{line}, ": Expected `", $exp->{l}->[$e]->{t}, "`\n";
            print STDERR "  $aname:", $a + 1, ": Got `", $_, "`$Off\n";
            return 1;
        }
    }

    if ($e != $exp->{nl}) {
        print STDERR "$Red${outname}FAIL: Missing output starting on line ", scalar(@$actual), "$Off\n";
        print STDERR "  $Redctx$ename:", $exp->{l}->[$e]->{line}, ": Expected `", $exp->{l}->[$e]->{t}, "`$Off\n";
        return 1;
    } else {
        print STDERR "$Green${outname}OK$Off\n";
        return 0;
    }
}

if (@ARGV > 0 && -f $ARGV[0]) {
    exit(run_compare(read_actual($ARGV[0]),
                     read_expected($ARGV[1]),
                     $ARGV[0], $ARGV[1], $ARGV[2] ? $ARGV[2] : $ARGV[0]));
} else {
    $ENV{"MALLOC_CHECK_"} = 0;
    my($maxtest, $ntest, $ntestfailed) = (30, 0, 0);
    for ($i = 1; $i <= $maxtest; $i += 1) {
        next if !test_runnable($i);
        ++$ntest;
        printf STDERR "test%03d ", $i;
        $out = run_sh61(sprintf("./test%03d", $i), "stdout" => "pipe", "stdin" => "/dev/null", "time_limit" => 5, "size_limit" => 1000);
        if (exists($out->{error})) {
            print STDERR "${Red}CRASH: $out->{error}$Off\n";
            if ($out->{output} =~ m/\A\s*(.+)/) {
                print STDERR "  ${Redctx}1st line of output: $1$Off\n";
            }
            ++$ntestfailed;
        } else {
            ++$ntestfailed if run_compare([split("\n", $out->{"output"})], read_expected(sprintf("test%03d.c", $i)),
                                          "output", sprintf("test%03d.c", $i), "");
        }
    }
    my($ntestpassed) = $ntest - $ntestfailed;
    if ($ntest == $maxtest && $ntestpassed == $ntest) {
        print STDERR "${Green}All tests passed!$Off\n";
    } else {
        my($color) = ($ntestpassed == 0 ? $Red : ($ntestpassed == $ntest ? $Green : $Cyan));
        print STDERR "${color}$ntestpassed of $ntest ", ($ntest == 1 ? "test" : "tests"), " passed$Off\n";
    }
}
