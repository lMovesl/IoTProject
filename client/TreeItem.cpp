#include "TreeItem.h"

#include <algorithm>

TreeItem::TreeItem(QVariantList data, TreeItem* parent) :
	m_varlstData(std::move(data)),
	m_pParent(parent) {
}

void TreeItem::appendChild(std::unique_ptr<TreeItem>&& child) {
	m_vpChildItems.push_back(std::move(child));
}

TreeItem* TreeItem::child(int row) {
	return (row >= 0 && row < childCount()) ? m_vpChildItems.at(row).get() : nullptr;
}

int TreeItem::childCount() const {
	return static_cast<int>(m_vpChildItems.size());
}

int TreeItem::columnCount() const {
	return static_cast<int>(m_varlstData.count());
}

QVariant TreeItem::data(int column) const {
	return m_varlstData.value(column);
}

int TreeItem::row() const {
	if (!m_pParent)
		return 0;

	const auto it = std::ranges::find_if(
		m_pParent->m_vpChildItems.cbegin(),
		m_pParent->m_vpChildItems.cend(),
		[this](const std::unique_ptr<TreeItem>& item ) { return item.get() == this; }
	);

	if (it != m_pParent->m_vpChildItems.cend())
		return std::distance(m_pParent->m_vpChildItems.cbegin(), it);

	return -1;
}

TreeItem* TreeItem::parentItem() const {
	return m_pParent;
}

bool TreeItem::insertChildren(int position, int count, int columns) {
	if (position < 0 || position > static_cast<qsizetype>(m_vpChildItems.size()))
		return false;

	for (int row = 0;row < count; ++row) {
		QVariantList data(columns);
		m_vpChildItems.insert(
			m_vpChildItems.cbegin() + position,
			std::make_unique<TreeItem>(data, this)
		);
	}

	return true;
}

bool TreeItem::insertColumns(int position, int columns) {
	if (position < 0 || position > m_varlstData.size())
		return false;

	for (int column = 0; column < columns; ++column)
		m_varlstData.insert(position, QVariant());

	for (auto& child : std::as_const(m_vpChildItems))
		child->insertColumns(position, columns);

	return true;
}

bool TreeItem::removeChildren(int position, int count) {
	if (position < 0 || position + count > static_cast<qsizetype>(m_vpChildItems.size()))
		return false;

	for (int row = 0; row < count; ++row)
		m_vpChildItems.erase(m_vpChildItems.cbegin() + position);

	return true;
}

bool TreeItem::removeColumns(int position, int columns)
{
	if (position < 0 || position + columns > m_varlstData.size())
		return false;

	for (int column = 0; column < columns; ++column)
		m_varlstData.remove(position);

	for (auto& child : std::as_const(m_vpChildItems))
		child->removeColumns(position, columns);

	return true;
}

bool TreeItem::setData(int column, const QVariant& value) {
	if (column < 0 || column > m_varlstData.size())
		return false;

	m_varlstData[column] = value;
	return true;
}
