
function menu()
	-- display menu ansi
	bbs_display_ansi("mainmenu");


	-- display prompt
	bbs_write_string("\r\n\027[0m(LUA) TL: " .. string.format("%d", bbs_time_left()) .. "m > ");


	-- read char entered
	cmd = bbs_read_char();

	-- do stuff if you want


	-- return the char entered

	return cmd;
end
