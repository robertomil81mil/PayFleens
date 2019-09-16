all:
	g++ PayFleens.c init.c  bitboards.c hashkeys.c board.c data.c attack.c io.c validate.c movegen.c makemove.c perft.c search.c pvtable.c evaluate.c -o PayFleens
	g++ xboard.c PayFleens.c uci.c evaluate.c pvtable.c init.c bitboards.c hashkeys.c board.c data.c attack.c io.c movegen.c validate.c makemove.c perft.c search.c misc.c -o PayFleensTest -O2 -s
	yVkbL60v7HQSKSkZ