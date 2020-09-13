#ifndef WSW_290ddc6d_0a8e_448e_8285_9e54dd121bc2_H
#define WSW_290ddc6d_0a8e_448e_8285_9e54dd121bc2_H

#include <QAbstractListModel>
#include <QJsonArray>

#include <optional>

#include "../qcommon/qcommon.h"
#include "../qcommon/wswstringview.h"
#include "../qcommon/wswstaticvector.h"
#include "../qcommon/wswstdtypes.h"

namespace wsw::ui {

class GametypeOptionsModel : public QAbstractListModel {
	Q_OBJECT
public:
	enum Kind {
		Boolean,
		OneOfList,
	};
	Q_ENUM( Kind )

	Q_PROPERTY( bool available READ isAvailable NOTIFY availableChanged )
	Q_PROPERTY( QString tabTitle READ getTabTitle NOTIFY tabTitleChanged )

	Q_SIGNAL void availableChanged( bool available );
	Q_SIGNAL void tabTitleChanged( const QString &title );
private:
	enum class Role {
		Title = Qt::UserRole + 1,
		Desc,
		Kind,
		Model,
		Current
	};

	using StringSpan = std::pair<unsigned, unsigned>;

	struct OptionEntry {
		StringSpan title;
		StringSpan desc;
		Kind kind;
		int model;
		StringSpan booleanStringSpan;
		std::pair<unsigned, unsigned> listEntriesSpan;
	};

	struct ListEntry {
		StringSpan iconPathSpan;
		StringSpan itemStringSpan;
		StringSpan commandArgSpan;
	};

	[[nodiscard]]
	bool isAvailable() const { return !m_optionEntries.empty(); }
	[[nodiscard]]
	auto getTabTitle() const -> QString { return m_tabTitle; }

	[[nodiscard]]
	auto roleNames() const -> QHash<int, QByteArray> override;
	[[nodiscard]]
	auto rowCount( const QModelIndex & ) const -> int override;
	[[nodiscard]]
	auto data( const QModelIndex &index, int role ) const -> QVariant override;

	[[nodiscard]]
	bool parseEntryParts( const wsw::StringView &string, wsw::StaticVector<wsw::StringView, 4> &parts );

	[[nodiscard]]
	auto addListItems( const wsw::StringView &string ) -> std::optional<std::pair<unsigned, unsigned>>;

	[[nodiscard]]
	auto addString( const wsw::StringView &string ) -> StringSpan;

	[[nodiscard]]
	auto getString( const StringSpan &span ) const -> QString;

	[[nodiscard]]
	bool doReload();

	void clear();

	[[nodiscard]]
	bool parseAvailableEntryNums( const wsw::StringView &string );
	[[nodiscard]]
	bool parseCurrentSelectedItemNums( const wsw::StringView &string, const wsw::Vector<unsigned> &availableNums );

	QString m_tabTitle;
	// TODO: Use something less wasteful to resources?
	wsw::Vector<OptionEntry> m_optionEntries;
	wsw::Vector<ListEntry> m_listEntries;
	wsw::Vector<unsigned> m_availableEntryNums;
	wsw::Vector<unsigned> m_currentSelectedItemNums;
	wsw::String m_stringData;
	wsw::Vector<unsigned> m_tmpAvailableEntryNums;
	wsw::Vector<unsigned> m_tmpCurrentSelectedItemNums;
public:
	void reload();
	void handleOptionsStatusCommand();
};

}

#endif