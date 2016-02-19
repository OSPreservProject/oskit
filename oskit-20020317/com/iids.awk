BEGIN {
	printf "OBJFILES +=";
}

{
	printf " %s.o", $2;

	filename = $2 ".c";
	iidmacro = toupper($2);
	printf "#include <%s>\n", $1 >filename;
	printf "const oskit_iid_t %s = %s;\n", $2, iidmacro >filename;
	close(filename);
}

