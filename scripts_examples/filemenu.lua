function menu()
	-- display menu ansi
	bbs_display_ansi("filemenu");


	-- display prompt
	local dir_no;
	local dir_name;
	local sub_no;
	local sub_name;
	
	dir_no, dir_name, sub_no, sub_name = bbs_cur_filearea_info();
	bbs_write_string(string.format("\r\n\027[0mDir: (%d) %s\r\nSub: (%d) %s\r\n(LUA) TL: %dm > ", dir_no, dir_name, sub_no, sub_name, bbs_time_left()));
	
	-- read char entered
	cmd = bbs_read_char();

	-- do stuff if you want


	-- return the char entered

	return cmd;
end
