# PayFleens
PayFleens is an UCI and xboard Chess Engine, it started as a goal to learn C with the YouTube video series by Bluefever and his engine Vice, at the moment PayFleens is using Vice as codebase. PayFleens is greatly inspired by Ethereal and Stockfish.

I haven't a makefile (I'll make it soon) so you can compile with this command:
`gcc xboard.c PayFleens.c uci.c evaluate.c ttable.c init.c bitboards.c hashkeys.c board.c data.c attack.c io.c movegen.c validate.c makemove.c perft.c search.c misc.c polybook.c polykeys.c -o PayFleens -O2 -s`

# Special Thanks
Thank you guys:
Bluefever & Andrew Grant.
