function menu()
	-- display menu ansi
	bbs_write_string("\027[2J");
	bbs_display_ansi("filemenu");


	-- display prompt
	local dir_no;
	local dir_name;
	local sub_no;
	local sub_name;
	
	dir_no, dir_name, sub_no, sub_name = bbs_cur_filearea_info();
	bbs_write_string(string.format("\r\n\027[0m   \027[0;36mDirectory: \027[1;34m(\027[1;37m%d\027[1;34m) \027[1;37m%-20s\027[0;36mSubdirectory: \027[1;34m(\027[1;37m%d\027[1;34m) \027[1;37m%-20s\r\n", dir_no, dir_name, sub_no, sub_name));


	bbs_write_string(string.format("\r\n\027[1;34m   [\027[0;36mTime Left\027[1;37m %dm\027[34m]-> \027[0m", bbs_time_left()));
	
	-- read char entered
	cmd = bbs_read_char();

	-- do stuff if you want


	-- return the char entered

	return cmd;
end
