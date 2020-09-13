#include "gametypeoptionsmodel.h"

#include "../client/client.h"
#include "../qcommon/wswstringsplitter.h"
#include "../qcommon/wswtonum.h"

using wsw::operator""_asView;

namespace wsw::ui {

auto GametypeOptionsModel::roleNames() const -> QHash<int, QByteArray> {
	return {
		{ (int)Role::Title, "title" },
		{ (int)Role::Desc, "desc" },
		{ (int)Role::Kind, "kind" },
		{ (int)Role::Model, "model" },
		{ (int)Role::Current, "current" }
	};
}

auto GametypeOptionsModel::rowCount( const QModelIndex & ) const -> int {
	return (int)m_availableEntryNums.size();
}

auto GametypeOptionsModel::data( const QModelIndex &index, int role ) const -> QVariant {
 	if( !index.isValid() ) {
 		return QVariant();
 	}
 	const int row = index.row();
 	if( (unsigned)row < (unsigned)m_availableEntryNums.size() ) {
 		return QVariant();
 	}
 	if( (Role)role == Role::Current ) {
 		// TODO: Get rid of parallel arrays?
 		assert( m_currentSelectedItemNums.size() == m_availableEntryNums.size() );
 		return m_currentSelectedItemNums[row];
 	}

 	const unsigned num = m_availableEntryNums[row];
 	assert( num < (unsigned)m_availableEntryNums.size() );
 	const auto &entry = m_optionEntries[num];
 	switch( (Role)role ) {
		case Role::Title: return getString( entry.title );
		case Role::Desc: return getString( entry.desc );
		case Role::Kind: return entry.kind;
		case Role::Model: return entry.model;
		default: return QVariant();
	}
}

bool GametypeOptionsModel::parseEntryParts( const wsw::StringView &string,
											wsw::StaticVector<wsw::StringView, 4> &parts ) {
	parts.clear();
	wsw::StringSplitter splitter( string );
	while( const auto maybeToken = splitter.getNext( ';' ) ) {
		if( parts.size() == 4 ) {
			return false;
		}
		const auto token( maybeToken->trim() );
		if( token.empty() ) {
			return false;
		}
		parts.push_back( token );
	}
	return parts.size() == 4;
}

auto GametypeOptionsModel::addString( const wsw::StringView &string ) -> StringSpan {
	const auto oldSize = m_stringData.size();
	// TODO: Sanitize string data
	m_stringData.append( string.data(), string.size() );
	m_stringData.push_back( '\0' );
	return StringSpan( oldSize, string.size() );
}

auto GametypeOptionsModel::getString( const StringSpan &nameSpan ) const -> QString {
	assert( nameSpan.first < m_stringData.size() );
	assert( nameSpan.first + nameSpan.second < m_stringData.size() );
	return QString::fromLatin1( m_stringData.data() + nameSpan.first, nameSpan.second );
}

void GametypeOptionsModel::clear() {
	m_optionEntries.clear();
	m_listEntries.clear();
	m_availableEntryNums.clear();
	m_stringData.clear();
}

void GametypeOptionsModel::reload() {
	beginResetModel();

	clear();
	if( !doReload() ) {
		clear();
	}

	endResetModel();

	Q_EMIT availableChanged( isAvailable() );
}

static const wsw::StringView kBoolean( "Boolean"_asView );
static const wsw::StringView kAnyOfList( "AnyOfList"_asView );

static const QString kLoadouts( "Loadouts" );

bool GametypeOptionsModel::doReload() {
	wsw::StaticVector<wsw::StringView, 4> parts;

	unsigned configStringNum = CS_GAMETYPE_OPTIONS;
	for(; configStringNum < CS_GAMETYPE_OPTIONS + MAX_GAMETYPE_OPTIONS; ++configStringNum ) {
		const auto maybeString = ::cl.configStrings.get( configStringNum );
		if( !maybeString ) {
			break;
		}

		if( !parseEntryParts( *maybeString, parts ) ) {
			return false;
		}

		const auto &kindToken = parts[2];
		if( kindToken.equalsIgnoreCase( kBoolean ) ) {
			// TODO: Check whether parts[3] could be a valid command
			OptionEntry entry {
				addString( parts[0] ),
				addString( parts[1] ),
				Kind::Boolean,
				0,
				addString( parts[3] ),
				{ 0, 0 }
			};
			m_optionEntries.emplace_back( std::move( entry ) );
			continue;
		}

		if( !kindToken.equalsIgnoreCase( kAnyOfList ) ) {
			return false;
		}

		auto maybeListItemsSpan = addListItems( parts[4] );
		if( !maybeListItemsSpan ) {
			return false;
		}

		OptionEntry entry {
			addString( parts[0] ),
			addString( parts[1] ),
			Kind::OneOfList,
			(int)maybeListItemsSpan->second,
			*maybeListItemsSpan
		};
		m_optionEntries.emplace_back( std::move( entry ) );
	}

	if( auto maybeTitle = ::cl.configStrings.get( CS_GAMETYPE_OPTIONS_TITLE ) ) {
		QString title( QString::fromLatin1( maybeTitle->data(), maybeTitle->size() ) );
		if( m_tabTitle != title ) {
			m_tabTitle = title;
			Q_EMIT tabTitleChanged( m_tabTitle );
		}
	} else {
		if( m_tabTitle != kLoadouts ) {
			m_tabTitle = kLoadouts;
			Q_EMIT tabTitleChanged( m_tabTitle );
		}
	}

	return true;
}

auto GametypeOptionsModel::addListItems( const wsw::StringView &string )
-> std::optional<std::pair<unsigned, unsigned>> {
	wsw::StringSplitter splitter( string );

	const auto maybeCommandToken = splitter.getNext( ',' );
	if( !maybeCommandToken ) {
		return std::nullopt;
	}

	const auto oldListItemsSize = m_listEntries.size();

	int tokenNum = 0;
	StringSpan tmpSpans[3];
	while( const auto maybeToken = splitter.getNext( ',' ) ) {
		const int spanNum = ( tokenNum++ ) % 3;
		// TODO: Sanitize a displayed name...
		// TODO: Check for a valid icon path...
		// TODO: Check for a valid command...
		tmpSpans[spanNum] = addString( *maybeToken );
		if( spanNum != 3 ) {
			continue;
		}

		// More than 7 list options are disallowed
		if( m_listEntries.size() - oldListItemsSize > 6 ) {
			return std::nullopt;
		}

		ListEntry entry { tmpSpans[0], tmpSpans[1], tmpSpans[2] };
		m_listEntries.push_back( entry );
	}

	if( tokenNum % 3 ) {
		return std::nullopt;
	}

	return std::make_pair( oldListItemsSize, m_listEntries.size() - oldListItemsSize );
}

bool GametypeOptionsModel::parseAvailableEntryNums( const wsw::StringView &string ) {
	m_tmpAvailableEntryNums.clear();

	wsw::StringSplitter splitter( string );
	while( const auto maybeToken = splitter.getNext() ) {
		if( const auto maybeNum = wsw::toNum<unsigned>( *maybeToken ) ) {
			const auto num = *maybeNum;
			if( *maybeNum < m_optionEntries.size() ) {
				// Check for duplicate nums
				const auto end = m_tmpAvailableEntryNums.cend();
				if( std::find( m_tmpAvailableEntryNums.cbegin(), m_tmpAvailableEntryNums.cend(), num ) == end ) {
					m_tmpAvailableEntryNums.push_back( num );
					continue;
				}
			}
		}
		return false;
	}

	return true;
}

bool GametypeOptionsModel::parseCurrentSelectedItemNums( const wsw::StringView &string,
														 const wsw::Vector<unsigned> &availableNums ) {
	m_tmpCurrentSelectedItemNums.clear();

	unsigned tokenNum = 0;
	wsw::StringSplitter splitter( string );
	while( const auto maybeToken = splitter.getNext() ) {
		if( const auto maybeNum = wsw::toNum<unsigned>( *maybeToken ) ) {
			if( tokenNum >= availableNums.size() ) {
				return false;
			}
			const auto num = *maybeNum;
			const auto &entry = m_optionEntries[availableNums[tokenNum]];
			if( entry.kind == Boolean ) {
				if( num != 0 && num != 1 ) {
					return false;
				}
			} else {
				assert( entry.kind == OneOfList );
				if( num >= entry.listEntriesSpan.second ) {
					return false;
				}
			}
			m_tmpCurrentSelectedItemNums.push_back( num );
		}
	}

	return true;
}

void GametypeOptionsModel::handleOptionsStatusCommand() {
	if( Cmd_Argc() != 2 ) {
		return;
	}

	wsw::StringSplitter splitter( wsw::StringView( Cmd_Argv( 1 ) ) );
	const auto maybeAvailablePart = splitter.getNext( '|' );
	const auto maybeCurrentPart = splitter.getNext( '|' );

	// There must be exactly 2 parts (without trailing ones)
	if( !maybeAvailablePart || !maybeCurrentPart || splitter.getNext() ) {
		return;
	}

	if( !parseAvailableEntryNums( maybeAvailablePart->trim() ) ) {
		return;
	}
	if( !parseCurrentSelectedItemNums( maybeCurrentPart->trim(), m_tmpAvailableEntryNums ) ) {
		return;
	}

	if( m_availableEntryNums == m_tmpAvailableEntryNums && m_tmpCurrentSelectedItemNums == m_currentSelectedItemNums ) {
		return;
	}

	beginResetModel();

	m_availableEntryNums.clear();
	m_availableEntryNums.swap( m_tmpAvailableEntryNums );
	m_currentSelectedItemNums.clear();
	m_currentSelectedItemNums.swap( m_tmpCurrentSelectedItemNums );

	endResetModel();
}

}