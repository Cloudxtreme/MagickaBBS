function menu()
	-- display menu ansi
	bbs_display_ansi("mailmenu");


	-- display prompt
	local conf_no;
	local conf_name;
	local area_no;
	local area_name;
	
	conf_no, conf_name, area_no, area_name = bbs_cur_mailarea_info();
	bbs_write_string(string.format("\r\n\027[0mConf: (%d) %s\r\nArea: (%d) %s\r\n(LUA) TL: %dm > ", conf_no, conf_name, area_no, area_name, bbs_time_left()));
	
	-- read char entered
	cmd = bbs_read_char();

	-- do stuff if you want


	-- return the char entered

	return cmd;
end
