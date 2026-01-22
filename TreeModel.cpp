#include "TreeModel.h"


TreeModel::TreeModel(const QStringList& headers, QObject* parent) :
	QAbstractItemModel(parent) {

	QVariantList rootData;
	for (const QString& header : headers)
		rootData << header;

	m_pRootItem = std::make_unique<TreeItem>(rootData);
}		

TreeModel::~TreeModel() = default;

QVariant TreeModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid())
		return {};

	if (role != Qt::DisplayRole && role != Qt::EditRole)
		return {};

	TreeItem* item = getItem(index);

	return item->data(index.column());
}

Qt::ItemFlags TreeModel::flags(const QModelIndex& index) const
{
	if (index.isValid())
		return Qt::NoItemFlags;

	return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	return (orientation == Qt::Horizontal && role == Qt::DisplayRole) ?
		m_pRootItem->data(section) : QVariant{};
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex& parent) const {
	if (!hasIndex(row, column, parent))
		return {};

	TreeItem* parentItem = getItem(parent);

	if (auto* childItem = parentItem->child(row))
		return createIndex(row, column, childItem);

	return {};
}

QModelIndex TreeModel::parent(const QModelIndex& index) const {
	if (!index.isValid())
		return {};

	TreeItem* childItem = getItem(index);
	TreeItem* parentItem = childItem ? childItem->parentItem() : nullptr;

	return (parentItem && parentItem != m_pRootItem.get()) ?
		createIndex(parentItem->row(), 0, parentItem) : QModelIndex{};
}

int TreeModel::rowCount(const QModelIndex& parent) const {
	if (parent.isValid() && parent.column() > 0)
		return 0;

	const TreeItem* parentItem = getItem(parent);

	return parentItem ? parentItem->childCount() : 0;
}

int TreeModel::columnCount(const QModelIndex& parent) const {
	Q_UNUSED(parent);
	return m_pRootItem->columnCount();
}

bool TreeModel::setData(const QModelIndex& index, const QVariant& value, int role) {
	if (role != Qt::EditRole)
		return false;

	TreeItem* item = getItem(index);
	bool result = item->setData(index.column(), value);

	if (result)
		emit dataChanged(index, index, { Qt::DisplayRole, Qt::EditRole });

	return result;
}

TreeItem* TreeModel::getItem(const QModelIndex& index) const {
	if (index.isValid()) {
		if (auto* item = static_cast<TreeItem*>(index.internalPointer()))
			return item;
	}

	return m_pRootItem.get();
}

bool TreeModel::insertRows(int position, int rows, const QModelIndex& parent)
{
	TreeItem* parentItem = getItem(parent);
	if (!parentItem)
		return false;

	beginInsertRows(parent, position, position + rows - 1);
	const bool success = parentItem->insertChildren(position,
		rows,
		m_pRootItem->columnCount());
	endInsertRows();

	return success;
}

bool TreeModel::removeColumns(int position, int columns, const QModelIndex& parent)
{
	beginRemoveColumns(parent, position, position + columns - 1);
	const bool success = m_pRootItem->removeColumns(position, columns);
	endRemoveColumns();

	if (m_pRootItem->columnCount() == 0)
		removeRows(0, rowCount());

	return success;
}

bool TreeModel::removeRows(int position, int rows, const QModelIndex& parent)
{
	TreeItem* parentItem = getItem(parent);
	if (!parentItem)
		return false;

	beginRemoveRows(parent, position, position + rows - 1);
	const bool success = parentItem->removeChildren(position, rows);
	endRemoveRows();

	return success;
}

bool TreeModel::insertColumns(int position, int columns, const QModelIndex& parent)
{
	beginInsertColumns(parent, position, position + columns - 1);
	const bool success = m_pRootItem->insertColumns(position, columns);
	endInsertColumns();

	return success;
}

bool TreeModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role)
{
	if (role != Qt::EditRole || orientation != Qt::Horizontal)
		return false;

	const bool result = m_pRootItem->setData(section, value);

	if (result)
		emit headerDataChanged(orientation, section, section);

	return result;
}