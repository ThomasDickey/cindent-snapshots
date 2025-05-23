#!/usr/bin/env perl
# $Id: make-man.pl,v 1.34 2024/05/15 00:13:09 tom Exp $
#------------------------------------------------------------------------------
# Copyright:  2010-2022,2024 by Thomas E. Dickey
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, distribute with modifications, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
# DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
# THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
# Except as contained in this notice, the name(s) of the above copyright
# holders shall not be used in advertising or otherwise to promote the
# sale, use or other dealings in this Software without prior written
# authorization.
#------------------------------------------------------------------------------
# make a usable manpage from a formatted info-file, tuned for indent.info
# (info2man does not handle the .DS parts)

use strict;
use warnings;

use POSIX qw(strftime);

our $margin = 70;
our $ignore = ".";

sub no_leading_dot($) {
    my $result = $_[0];
    $result =~ s/^\./\\./;
    return $result;
}

sub is_empty($) {
    my $result = 0;
    if ( $_[0] =~ /^$/ ) {
        $result = 1;
    }
    elsif ( $_[0] ne $ignore and $_[0] =~ /^\./ ) {
        $result = 1;
    }
    return $result;
}

sub is_ignored($) {
    return ( $_[0] eq $ignore );
}

sub is_menu_head($) {
    my $result = 0;
    if ( $_[0] eq "* Menu:" ) {
        $result = 1;
    }
    return $result;
}

sub is_menu_body($) {
    my $result = 0;
    if ( $_[0] =~ /^\* .*:/ ) {
        $result = 1;
    }
    elsif ( $_[0] =~ /\s+\(line\s+\d+\)$/ ) {
        $result = 1;
    }
    return $result;
}

sub format_menu($) {
    my $result = $_[0];

    $result =~ s/\s+\(line\s+\d+\)$//;
    $result =~ s/^\*\s+//;
    if ( $result =~ /::\s/ ) {
        $result =~ s/::\s+/\tsee \\fB/;
        $result =~ s/$/\\fR/;
    }
    elsif ( $result =~ /:\s/ ) {
        $result =~ s/:\s+/\t/;
    }
    $result = &no_leading_dot($result);
    return $result;
}

sub is_para_head($) {
    my $result = 0;
    if ( $_[0] =~ /^   [^\s]/ ) {
        $result = 1;
    }
    return $result;
}

sub is_para_body($) {
    my $result = 0;
    if ( $_[0] =~ /^[^\s]/ ) {
        $result = 1;
    }
    return $result;
}

sub is_display($) {
    my $result = 0;
    if ( $_[0] =~ /^    / ) {
        $result = 1;
    }
    return $result;
}

sub format_display($) {
    my $result = $_[0];
    $result =~ s/^     //;
    $result = &no_leading_dot($result);
    return $result;
}

sub is_title($$) {
    my $text = $_[0];
    my $next = $_[1];

    my $result = "";
    if ( length($text) eq length($next) ) {
        if (   $next =~ /^[*]*$/
            or $next =~ /^[=]*$/ )
        {
            $result = ".SH";
        }
        elsif ( $next =~ /^[-]*$/ ) {
            $result = ".SS";
        }
    }
    return $result;
}

sub isnt_title($$) {
    my $item   = $_[0];
    my @data   = @{ $_[1] };
    my $result = 1;
    my $skip   = 0;
    while ( $item + 1 < $#data ) {
        my $text = $data[$item];
        if ( $skip != 0 ) {
            $skip = 0;
        }
        elsif ( $text =~ /^\037/ ) {
            $skip = 1;
        }
        elsif ( $text ne "" ) {
            last;
        }
        ++$item;
    }
    if ( $item + 1 < $#data ) {
        my $check = &is_title( $data[$item], $data[ $item + 1 ] );
        $result = 0 if ( $check ne "" );
    }
    return $result;
}

sub do_file($) {
    my ($name) = @_;
    my $n;
    my $k;
    my $DS = 0;
    my $RS = 0;
    my $TS = 0;
    my $section;
    my $text;
    my $next;
    my @markup;

    open( FP, $name ) || die "Can't open $name: $!\n";
    my (@input) = <FP>;
    close(FP);

    for $n ( 0 .. $#input ) {
        chomp $input[$n];    # trim newlines
        $input[$n] =~ s/\s+$//;
        $input[$n]  = &no_leading_dot( $input[$n] );
        $markup[$n] = "";
        if ( $input[$n] =~ /^\000\010/ ) {
            $input[$n] = $ignore;
        }
    }

    my $content = 0;
    for $n ( 0 .. $#input ) {
        $text = $input[$n];
        if ( $n < $#input ) {
            $next = $input[ $n + 1 ];
        }
        else {
            $next = "";
        }
        if ( $text =~ /^\037/ ) {
            $content = 1;
            if ( $n eq $#input ) {
                $k = $n;
            }
            elsif ( $next =~ /^Tag Table:$/ ) {
                $k = $#input;
            }
            else {
                $k = $n + 1;
            }
            for $k ( $n .. $k ) {
                $input[$k] = $ignore;
            }
        }
        elsif ( $content == 0 ) {
            $input[$n] = ".\\\" " . $input[$n];
        }
        elsif ( &is_empty($text) or &is_ignored($text) ) {
            if ( $n < $#input ) {
                if ( $RS ne 0 ) {
                    $markup[$n] .= ".RE";
                    $RS = 0;
                }
                if ( &is_menu_head($next) ) {
                    $TS = 1;
                    $markup[$n] .= ".TS";
                }
                elsif ( $TS ne 0 and not &is_menu_body($next) ) {
                    $TS = 0;
                    $markup[$n] .= ".TE";
                }
                if ( &is_para_head($next) ) {
                    if ( $DS ne 0 ) {
                        $markup[$n] .= ".NE";
                        $DS = 0;
                    }
                    $markup[$n] .= ".PP";
                    $input[ $n + 1 ] =~ s/^\s+//;
                }
                elsif ( &is_display($next) ) {
                    if ( $DS eq 0 ) {
                        $markup[$n] .= ".NS";
                        $DS = 1;
                    }
                }
                elsif ( &is_para_body($next) ) {
                    if ( $DS ne 0 ) {
                        $markup[$n] .= ".NE";
                        $markup[$n] .= ".PP"
                          if ( &isnt_title( $n + 1, \@input ) );
                        $DS = 0;
                    }
                    elsif ( $TS ne 0 ) {

                        # skip
                    }
                    elsif ( length($text) < $margin ) {
                        my $hang = 0;

                        for $k ( $n + 2 .. $#input ) {
                            if ( &is_display( $input[$k] ) ) {
                                $hang = $k;
                                last;
                            }
                            elsif ( length( $input[$k] ) < $margin ) {
                                if ( not &is_para_body( $input[$k] ) ) {
                                    last;
                                }
                            }
                        }
                        if ($hang) {
                            for $k ( $n + 2 .. $hang - 1 ) {
                                $markup[$k] .= ".br";
                            }
                            $markup[$hang] .= ".RS 5";
                            $markup[$n]    .= ".PP";
                            $RS = 1;
                        }
                    }
                }
                if ( $DS ne 0 ) {
                    if ( $markup[$n] eq "" ) {
                        $markup[$n] .= ".sp";
                    }
                }
                elsif ( $markup[$n] eq "" ) {

                    # kill blank lines where we have not added a command
                    $input[$n] = $ignore;
                }
            }
            else {
                $input[$n] = $ignore;
            }
        }
        else {
            if ( $n < $#input ) {
                if ( $TS eq 0 and &is_display($text) ) {
                    if ( $RS ne 0 ) {
                        $input[$n] = &format_display($text);
                        next;
                    }
                    elsif ( $DS eq 0 ) {
                        $markup[$n] .= ".NS";
                        $DS = 1;
                    }
                }
                if ( $DS ne 0 ) {
                    $input[$n] = &format_display($text);
                    next;
                }
                else {
                    if ( $TS ne 0 ) {
                        if ( &is_menu_body( $input[$n] ) ) {
                            $input[$n] = &format_menu($text);
                            next;
                        }
                        else {
                            $TS = 0;
                        }
                    }
                    $section = &is_title( $input[$n], $next );
                    if ( $section ne "" ) {
                        $input[$n] = $section . " \"" . $input[$n] . "\"";
                        $input[ $n + 1 ] = $ignore;
                        next;
                    }
                }
            }
        }
    }

    my $wrap = 0;
    for $n ( 0 .. $#input ) {
        if ($wrap) {
            $input[$n] =~ s/^([^']+)'/$1\\fR/;
            $wrap = 0;
        }
        $input[$n] =~ s/``([^`']+)``([^`']+)''([^`']+)''/\\fB$1$2$3\\fP/g;
        $input[$n] =~ s/['`]([^`']+)`([^`']+)'([^`']+)'/\\fB$1$2$3\\fP/g;
        $input[$n] =~ s/``([^']+)''/\\fB$1\\fR/g;
        $input[$n] =~ s/['`]([^']+)'/\\fB$1\\fR/g;
        $input[$n] =~ s/-/\\-/g;
        $input[$n] =~ s/\\n/\\en/g if ( $input[$n] !~ /^\./ );
        $input[$n] =~ s/\(\*note (.*)::\)/(see \\fB$1\\fR)/g;
        $input[$n] =~ s/\*Note (.*)::/(see \\fB$1\\fR)/g;

        if ( $input[$n] =~ /['`]/ and $input[$n] !~ /^\./ ) {
            $wrap = 1;
            $input[$n] =~ s/['`]/\\fB/;
        }
        $input[$n] =~ s/\s+$//;
    }

    my $rootname = $name;
    $rootname =~ s/\..*//;
    my $ROOTNAME = uc($rootname);
    my $date     = strftime "%Y-%m-%d", localtime;
    printf "'\\\" t
.TH $ROOTNAME 1 $date \"\" \"User Commands\"
.ie n .ds CW R
.el   \\{
.ie \\n(.g .ds CW CR
.el       .ds CW CW
.\\}
.de NS
.ie n  .sp
.el    .sp .5
.ie n  .in +4
.el    .in +2
.nf
.ft \\*(CW
..
.de NE
.fi
.ft R
.ie n  .in -4
.el    .in -2
..
.SH NAME
$rootname \\-
$name
";

    for $n ( 0 .. $#input ) {
        if ( $markup[$n] =~ /^\.TS/ ) {
            printf "%s\n", $markup[$n];
            printf "l l\n";
            printf "l l .\n";
        }
        elsif ( $markup[$n] =~ /^\./ ) {
            foreach $k ( split /\./, $markup[$n] ) {
                if ( $k ne "" ) {
                    printf ".%s\n", $k;
                }
            }
        }
        if ( not &is_ignored( $input[$n] ) ) {
            if ( $input[$n] =~ /^\./
                or ( $n > 0 and $input[ $n - 1 ] =~ /^\.\w+$/ ) )
            {
                printf "%s\n", $input[$n];
            }
            else {
                if ( $input[$n] ne "" ) {
                    my $input = $input[$n];
                    $input =~ s/\s+$//;
                    $input =~ s/\.\s+/.\n/g
                      unless ( $input =~ /\b\w\.\s/ or $input =~ /(\.\s+){3}/ );
                    my @parts = split /\n/, $input;
                    for my $part ( 0 .. $#parts ) {
                        printf "%s\n", $parts[$part];
                    }
                }
            }
        }
    }
}

while ( $#ARGV >= 0 ) {
    &do_file( shift @ARGV );
}

# vi:ts=4 sw=4
