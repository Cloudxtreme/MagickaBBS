#include "bbs.h"
#include "lua/lua.h"
#include "lua/lauxlib.h"

extern int mynode;
extern struct bbs_config conf;

extern struct user_record *gUser;
extern int gSocket;

int l_bbsWString(lua_State *L) {
	char *str = (char *)lua_tostring(L, -1);
	
	s_putstring(gSocket, str);
	
	return 0;
}

int l_bbsRString(lua_State *L) {
	char buffer[256];
	int len = lua_tonumber(L, -1);
	
	if (len > 256) {
		len = 256;
	}
	
	s_readstring(gSocket, buffer, len);
	
	lua_pushstring(L, buffer);
	
	return 1;
}

int l_bbsRChar(lua_State *L) {
	char c;
	
	c = s_getc(gSocket);
	
	lua_pushlstring(L, &c, 1);
	
	return 1;
}

int l_bbsDisplayAnsi(lua_State *L) {
	char *str = (char *)lua_tostring(L, -1);
	
	s_displayansi(gSocket, str);
	
	return 0;
}

int l_bbsVersion(lua_State *L) {
	char buffer[64];
	snprintf(buffer, 64, "Magicka BBS v%d.%d (%s)", VERSION_MAJOR, VERSION_MINOR, VERSION_STR);
	
	lua_pushstring(L, buffer);
	
	return 1;
}

int l_bbsNode(lua_State *L) {
	lua_pushnumber(L, mynode);
	
	return 1;
}

int l_bbsReadLast10(lua_State *L) {
	int offset = lua_tonumber(L, -1);
	struct last10_callers l10;
	FILE *fptr;
	
	fptr = fopen("last10.dat", "rb");
	if (!fptr) {
		return 0;
	}
	fseek(fptr, offset * sizeof(struct last10_callers), SEEK_SET);
	if (fread(&l10, sizeof(struct last10_callers), 1, fptr) == 0) {
		return 0;
	}
	fclose(fptr);
	
	lua_pushstring(L, l10.name);
	lua_pushstring(L, l10.location);
	lua_pushnumber(L, l10.time);
	
	return 3;
}

int l_bbsGetEmailCount(lua_State *L) {
	lua_pushnumber(L, mail_getemailcount(gUser));
	
	return 1;
}

int l_bbsMailScan(lua_State *L) {
	mail_scan(gSocket, gUser);
	return 0;
}

int l_bbsRunDoor(lua_State *L) {
	char *cmd = (char *)lua_tostring(L, 1);
	int stdio = lua_toboolean(L, 2);
	
	rundoor(gSocket, gUser, cmd, stdio);
	
	return 0;
}

void lua_push_cfunctions(lua_State *L) {
	lua_pushcfunction(L, l_bbsWString);
	lua_setglobal(L, "bbs_write_string");
	lua_pushcfunction(L, l_bbsRString);
	lua_setglobal(L, "bbs_read_string");
	lua_pushcfunction(L, l_bbsDisplayAnsi);
	lua_setglobal(L, "bbs_display_ansi");
	lua_pushcfunction(L, l_bbsRChar);
	lua_setglobal(L, "bbs_read_char");
	lua_pushcfunction(L, l_bbsVersion);
	lua_setglobal(L, "bbs_version");
	lua_pushcfunction(L, l_bbsNode);
	lua_setglobal(L, "bbs_node");
	lua_pushcfunction(L, l_bbsReadLast10);
	lua_setglobal(L, "bbs_read_last10");
	lua_pushcfunction(L, l_bbsGetEmailCount);
	lua_setglobal(L, "bbs_get_emailcount");
	lua_pushcfunction(L, l_bbsMailScan);
	lua_setglobal(L, "bbs_mail_scan");
	lua_pushcfunction(L, l_bbsRunDoor);
	lua_setglobal(L, "bbs_run_door");
}
