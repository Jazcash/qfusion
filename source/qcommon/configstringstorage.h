#ifndef WSW_64e10710_4a93_4f7e_8746_1158315f7bc4_H
#define WSW_64e10710_4a93_4f7e_8746_1158315f7bc4_H

#include "qcommon.h"
#include "wswstringview.h"

namespace wsw {

class ConfigStringStorage {
public:
	static inline constexpr unsigned kMaxStrings = MAX_CONFIGSTRINGS;
protected:
	struct Entry {
		char *data;
		unsigned len;
		unsigned capacity;
	};

	Entry m_entries[kMaxStrings];

	// MAX_CONFIGSTRING_CHARS gets deprecated
	static const size_t kMaxShortStringSize = MAX_QPATH;

	struct alignas( 8 )ShortStringBlock {
		ShortStringBlock *prev { nullptr };
		ShortStringBlock *next { nullptr };
		char buffer[kMaxShortStringSize];
	};

	ShortStringBlock *m_freeHead { nullptr };
	ShortStringBlock *m_usedHead { nullptr };

	// TODO: Generalize...
	ShortStringBlock m_localBlocks[128];

	void freeEntries();
	void makeFreeListLinks();

	static void assignEntryData( Entry *entry, char *data, size_t capacity, const wsw::StringView &string );

	[[nodiscard]]
	inline auto getUnderlyingLocalStorage( const char *data ) -> ShortStringBlock *;

	[[nodiscard]]
	auto getNoCheck( unsigned index ) const -> std::optional<wsw::StringView>;

	void setNoCheck( unsigned index, const wsw::StringView &string );

	[[nodiscard]]
	static auto checkIndex( unsigned index ) -> unsigned {
		if( index >= kMaxStrings ) {
			throw std::out_of_range( "The index is out of range" );
		}
		return index;
	}

	[[nodiscard]]
	static auto groupNumToIndex( unsigned num, unsigned startNum, unsigned maxGroupStrings ) -> unsigned {
		if( num >= maxGroupStrings || num + startNum >= kMaxStrings ) {
			throw std::out_of_range( "The num is out of range" );
		}
		return num + startNum;
	}

	[[nodiscard]]
	static auto callvoteNumToIndex( unsigned num, unsigned field ) -> unsigned {
		if( num >= MAX_CALLVOTEINFOS / 4 || CS_CALLVOTEINFOS + num * 4 + field >= kMaxStrings ) {
			throw std::out_of_range( "The num is out of range" );
		}
		return CS_CALLVOTEINFOS + num * 4 + field;
	}
public:
	ConfigStringStorage();

	virtual ~ConfigStringStorage();

	virtual void clear();

	virtual void copyFrom( const ConfigStringStorage &that );

	[[nodiscard]]
	auto get( unsigned index ) const -> std::optional<wsw::StringView> {
		return getNoCheck( checkIndex( index ) );
	}

	void set( unsigned index, const wsw::StringView &string ) {
		setNoCheck( checkIndex( index ), string );
	}

#define DEFINE_CONFIGSTRING_ACCESSORS( index, name )                                               \
	[[nodiscard]]                                                                                  \
	auto get##name() const -> std::optional<wsw::StringView> {                                     \
		return getNoCheck( index );                                                                \
	}                                                                                              \
	void set##name( const wsw::StringView &string ) {                                              \
		setNoCheck( index, string );                                                               \
	}

	DEFINE_CONFIGSTRING_ACCESSORS( CS_HOSTNAME, HostName )
	DEFINE_CONFIGSTRING_ACCESSORS( CS_MAXCLIENTS, MaxClients )
	DEFINE_CONFIGSTRING_ACCESSORS( CS_MESSAGE, Message )
	DEFINE_CONFIGSTRING_ACCESSORS( CS_MAPNAME, MapName )

	DEFINE_CONFIGSTRING_ACCESSORS( CS_AUDIOTRACK, AudioTrack )
	DEFINE_CONFIGSTRING_ACCESSORS( CS_SKYBOX, SkyBox )
	DEFINE_CONFIGSTRING_ACCESSORS( CS_GAMETYPETITLE, GametypeTitle )
	DEFINE_CONFIGSTRING_ACCESSORS( CS_GAMETYPENAME, GametypeName )
	DEFINE_CONFIGSTRING_ACCESSORS( CS_GAMETYPEVERSION, GametypeVersion )
	DEFINE_CONFIGSTRING_ACCESSORS( CS_GAMETYPEAUTHOR, GametypeAuthor )
	DEFINE_CONFIGSTRING_ACCESSORS( CS_AUTORECORDSTATE, AutoRecordState )
	DEFINE_CONFIGSTRING_ACCESSORS( CS_TEAM_SPECTATOR_NAME, TeamSpectatorName )
	DEFINE_CONFIGSTRING_ACCESSORS( CS_TEAM_PLAYERS_NAME, TeamPlayersName )
	DEFINE_CONFIGSTRING_ACCESSORS( CS_TEAM_ALPHA_NAME, TeamAlphaName )
	DEFINE_CONFIGSTRING_ACCESSORS( CS_TEAM_BETA_NAME, TeamBetaName )

	DEFINE_CONFIGSTRING_ACCESSORS( CS_MATCHNAME, MatchName )
	DEFINE_CONFIGSTRING_ACCESSORS( CS_MATCHSCORE, MatchScore )
	DEFINE_CONFIGSTRING_ACCESSORS( CS_MATCHUUID, MatchUuid )

	// TODO...
	DEFINE_CONFIGSTRING_ACCESSORS( CS_WORLDMODEL, WorldModel )
	DEFINE_CONFIGSTRING_ACCESSORS( CS_MAPCHECKSUM, MapCheckSum )

#undef DEFINE_CONFIGSTRING_ACCESSORS

#define DEFINE_CONFIGSTRING_GROUP_ACCESSORS( startIndex, maxGroupStrings, name )                   \
	[[nodiscard]]                                                                                  \
	auto get##name( unsigned num ) const -> std::optional<wsw::StringView> {                       \
		return getNoCheck( groupNumToIndex( num, startIndex, maxGroupStrings ) );                  \
	}                                                                                              \
	void set##name( const wsw::StringView &string, unsigned num ) {                                \
		set( groupNumToIndex( num, startIndex, maxGroupStrings ), string );                        \
	}

	DEFINE_CONFIGSTRING_GROUP_ACCESSORS( CS_MODELS, MAX_MODELS, Model )
	DEFINE_CONFIGSTRING_GROUP_ACCESSORS( CS_SOUNDS, MAX_SOUNDS, Sound )
	DEFINE_CONFIGSTRING_GROUP_ACCESSORS( CS_IMAGES, MAX_IMAGES, Image )
	DEFINE_CONFIGSTRING_GROUP_ACCESSORS( CS_SKINFILES, MAX_SKINFILES, SkinFile )
	DEFINE_CONFIGSTRING_GROUP_ACCESSORS( CS_LIGHTS, MAX_LIGHTSTYLES, LightStyle )
	DEFINE_CONFIGSTRING_GROUP_ACCESSORS( CS_ITEMS, MAX_ITEMS, Item )
	DEFINE_CONFIGSTRING_GROUP_ACCESSORS( CS_PLAYERINFOS, MAX_CLIENTS, PlayerInfo )
	DEFINE_CONFIGSTRING_GROUP_ACCESSORS( CS_GAMECOMMANDS, MAX_GAMECOMMANDS, GameCommand )
	DEFINE_CONFIGSTRING_GROUP_ACCESSORS( CS_LOCATIONS, MAX_LOCATIONS, Location )
	DEFINE_CONFIGSTRING_GROUP_ACCESSORS( CS_WEAPONDEFS, MAX_WEAPONDEFS, WeaponDef )
	DEFINE_CONFIGSTRING_GROUP_ACCESSORS( CS_GENERAL, MAX_GENERAL, General )
	DEFINE_CONFIGSTRING_GROUP_ACCESSORS( CS_CALLVOTEINFOS, MAX_CALLVOTEINFOS, CallvoteInfo )
	DEFINE_CONFIGSTRING_GROUP_ACCESSORS( CS_MMPLAYERINFOS, MAX_MMPLAYERINFOS, MMPlayerInfo )
	DEFINE_CONFIGSTRING_GROUP_ACCESSORS( CS_HELPMESSAGES, MAX_HELPMESSAGES, HelpMessage )

#undef DEFINE_CONFIGSTRING_GROUP_ACCESSORS

	static constexpr const unsigned kCallvoteFieldName = 0;
	static constexpr const unsigned kCallvoteFieldDesc = 1;
	static constexpr const unsigned kCallvoteFieldArgs = 2;
	static constexpr const unsigned kCallvoteFieldStatus = 3;

	[[nodiscard]]
	auto getCallvoteName( unsigned num ) const -> std::optional<wsw::StringView> {
		return getNoCheck( callvoteNumToIndex( num, kCallvoteFieldName ) );
	}
	[[nodiscard]]
	auto getCallvoteDesc( unsigned num ) const -> std::optional<wsw::StringView> {
		return getNoCheck( callvoteNumToIndex( num, kCallvoteFieldDesc ) );
	}
	[[nodiscard]]
	auto getCallvoteArgs( unsigned num ) const -> std::optional<wsw::StringView> {
		return getNoCheck( callvoteNumToIndex( num, kCallvoteFieldArgs ) );
	}
	[[nodiscard]]
	auto getCallvoteStatus( unsigned num ) const -> std::optional<wsw::StringView> {
		return getNoCheck( callvoteNumToIndex( num, kCallvoteFieldStatus ) );
	}
};

}

#endif
