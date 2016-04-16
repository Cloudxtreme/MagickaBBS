function menu()
	-- display menu ansi
	bbs_write_string("\027[2J");
	bbs_display_ansi("doors");


	-- display prompt
	bbs_write_string(string.format("\r\n\027[1;34m   [\027[0;36mTime Left\027[1;37m %dm\027[34m]-> \027[0m", bbs_time_left()));
	
	-- read char entered
	cmd = bbs_read_char();

	-- do stuff if you want


	-- return the char entered

	return cmd;
end
