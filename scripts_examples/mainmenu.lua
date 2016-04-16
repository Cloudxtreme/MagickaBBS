local function readLines(sPath)
  local file = io.open(sPath, "r")
  if file then
        local tLines = {}
        local sLine = file:read()
        while sLine do
          table.insert(tLines, sLine)
          sLine = file:read()
        end
        file:close()
        return tLines
  end
  return nil
end

function menu()
	-- display menu ansi
	bbs_write_string("\027[2J");
	bbs_display_ansi("mainmenu");

	-- display tagline

	local tLines = readLines("scripts/taglines.txt");

	local rand = math.random(#tLines);

	bbs_write_string("\r\n\027[1;30m   " .. tLines[rand] .. "\r\n");

	-- display prompt
	
	bbs_write_string(string.format("\r\n\027[1;34m   [\027[0;36mTime Left\027[1;37m %dm\027[34m]-> \027[0m", bbs_time_left()));

	-- read char entered
	cmd = bbs_read_char();

	-- do stuff if you want


	-- return the char entered

	return cmd;
end

math.randomseed(os.time());
