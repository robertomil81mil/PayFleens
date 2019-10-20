all:
	g++ PayFleens.c init.c  bitboards.c hashkeys.c board.c data.c attack.c io.c validate.c movegen.c makemove.c perft.c search.c pvtable.c evaluate.c -o PayFleens
	g++ xboard.c PayFleens.c uci.c evaluate3.c pvtable.c init.c bitboards.c hashkeys.c board.c data.c attack.c io.c movegen.c validate.c makemove.c perft.c search2.c misc.c polybook.c polykeys.c -o PayFleens+threshold+N -O2 -s
	yVkbL60v7HQSKSkZ
	cd C:\programm\PayFleens
	cd C:\programm\PayFleens-master
	cd C:\programm\PayFleens-master\Pay3
	T7 = lmr like reduction + old ext
	T8 = R like reduction + old ext
	T9 = Trying 1023 nodes and ext
	T10 = using the Ethereal method whitout newdepth
