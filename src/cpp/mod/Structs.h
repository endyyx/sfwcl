#ifndef STRUCTS_H
#define STRUCTS_H
#ifdef USE_SDK
#include <memory>

struct SServerInfo 
{
	SServerInfo():
	m_publicIP(0),
	m_publicPort(0),
	m_hostPort(0),
	m_privateIP(0),
	m_numPlayers(0),
	m_maxPlayers(0),
	m_ping(-1),
	m_serverId(-1),
	m_favorite(false),
	m_recent(false),
	m_private(false),
	m_official(false),
	m_anticheat(false),
	m_gamepadsonly(false),
	m_canjoin(true)
	{
	}

	string  m_hostName;
	string  m_mapName;
	string  m_gameType;
	wstring m_gameTypeName;
	string  m_gameVersion;

	string  m_modName;
	string  m_modVersion;	//ignore on 5767

	uint32  m_publicIP;
	ushort  m_publicPort;
	ushort	m_hostPort;
	uint32	m_privateIP;
	int		m_numPlayers;
	int		m_maxPlayers;
	int     m_ping;
	int     m_serverId;
	bool    m_favorite;
	bool    m_recent;
	bool	m_private;
	bool    m_official;
	bool    m_anticheat;
	bool    m_voicecomm;
	bool    m_friendlyfire;
	bool    m_dx10;
	bool    m_dedicated;
	bool    m_gamepadsonly;
	bool	m_canjoin;
};
struct MultiplayerMenu{
	void* m_browser;
	void* m_profile;
	void* m_chat;
	std::auto_ptr<void*> m_ui;
};
#else
struct MultiplayerMenu{};
struct SServerInfo{};
#endif
#endif