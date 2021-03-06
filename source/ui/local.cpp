#include "local.h"
#include "../qcommon/qcommon.h"
#include "../qcommon/wswstringview.h"

#include <QColor>

namespace wsw::ui {

class HtmlColorNamesCache {
	QString names[10];
public:
	auto getColorName( int colorNum ) -> const QString & {
		assert( (unsigned)colorNum < 10u );
		if( !names[colorNum].isEmpty() ) {
			return names[colorNum];
		}
		const float *rawColor = color_table[colorNum];
		names[colorNum] = QColor::fromRgbF( rawColor[0], rawColor[1], rawColor[2] ).name( QColor::HexRgb );
		return names[colorNum];
	}
};

static HtmlColorNamesCache htmlColorNamesCache;

static const QLatin1String kFontOpeningTagPrefix( "<font color=\"" );
static const QLatin1String kFontOpeningTagSuffix( "\">" );
static const QLatin1String kFontClosingTag( "</font>" );

// Can't be written as a regular ASCII literal with a 5-character length
static const char kEntityCharsToEscapeData[5] { '>', '<', '&', '\"', (char)0xA0 };
static const wsw::CharLookup kEntityCharsToEscapeLookup( wsw::StringView( kEntityCharsToEscapeData, 5 ) );

static const QLatin1String kEntityGt( "&gt;" );
static const QLatin1String kEntityLt( "&lt;" );
static const QLatin1String kEntityAmp( "&amp;" );
static const QLatin1String kEntityQuot( "&quot;" );
static const QLatin1String kEntityNbsp( "&nbsp;" );

static auto appendEscapingEntities( QString *dest, const wsw::StringView &src ) {
	wsw::StringView sv( src );
	for(;; ) {
		const std::optional<unsigned> maybeIndex = sv.indexOf( kEntityCharsToEscapeLookup );
		if( !maybeIndex ) {
			dest->append( QLatin1String( sv.data(), sv.size() ) );
			return;
		}

		const auto index = *maybeIndex;
		dest->append( QLatin1String( sv.data(), index ) );
		const char ch = sv[index];
		sv = sv.drop( index + 1 );

		// The subset of actually supported entities is tiny
		// https://github.com/qt/qtdeclarative/blob/dev/src/quick/util/qquickstyledtext.cpp
		// void QQuickStyledTextPrivate::parseEntity(const QChar *&ch, const QString &textIn, QString &textOut)

		// TODO: Aren't &quot; and &nbsp; stripped by the engine?
		// Still, some content could be supplied by external sources like info servers.
		if( ch == '>' ) {
			dest->append( kEntityGt );
		} else if( ch == '<' ) {
			dest->append( kEntityLt );
		} else if( ch == '&' ) {
			dest->append( kEntityAmp );
		} else if( ch == '"' ) {
			dest->append( kEntityQuot );
		} else if( (int)ch == 0xA0 ) {
			dest->append( kEntityNbsp );
		}
	}
}

auto toStyledText( const wsw::StringView &text ) -> QString {
	QString result;
	result.reserve( (int)text.size() + 1 );

	bool hadColorToken = false;
	wsw::StringView sv( text );
	for(;; ) {
		const std::optional<unsigned> maybeIndex = sv.indexOf( '^' );
		if( !maybeIndex ) {
			appendEscapingEntities( &result, sv );
			if( hadColorToken ) {
				result.append( kFontClosingTag );
			}
			return result;
		}

		const auto index = *maybeIndex;
		appendEscapingEntities( &result, sv.take( index ) );
		sv = sv.drop( index + 1 );
		if( sv.empty() ) {
			if( hadColorToken ) {
				result.append( kFontClosingTag );
			}
			return result;
		}

		if( sv.front() < '0' || sv.front() > '9' ) {
			if( sv.front() == '^' ) {
				result.append( '^' );
				sv = sv.drop( 1 );
			}
			continue;
		}

		if( hadColorToken ) {
			result.append( kFontClosingTag );
		}

		result.append( kFontOpeningTagPrefix );
		result.append( htmlColorNamesCache.getColorName( sv.front() - '0' ) );
		result.append( kFontOpeningTagSuffix );

		sv = sv.drop( 1 );
		hadColorToken = true;
	}
}

}