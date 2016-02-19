BEGIN {
	printf "ISADRIVERS =";
}

/^driver/ {
	printf " %s", $2;

	filename = $2 ".h";
	symbol = toupper("n"$2);
	printf "#define %s %d\n", symbol, $4 >filename;
}

