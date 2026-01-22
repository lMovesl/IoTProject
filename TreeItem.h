#ifndef TREE_ITEM_H
#define TREE_ITEM_H

#include <QVariantList>

class TreeItem
{
public:
	explicit TreeItem(QVariantList data, TreeItem* parent = nullptr);

	void appendChild(std::unique_ptr<TreeItem>&& child);

	TreeItem* child(int row);
	int childCount() const;
	int columnCount() const;
	QVariant data(int column) const;
	//reports the item's location within its parent's list of items
	int row() const;
	TreeItem* parentItem() const;

	bool insertChildren(int position, int count, int columns);
	bool insertColumns(int position, int columns);
	bool removeChildren(int position, int count);
	bool removeColumns(int position, int columns);
	bool setData(int column, const QVariant& value);

private:
	std::vector<std::unique_ptr<TreeItem>> m_vpChildItems;
	TreeItem* m_pParent = nullptr;
	QVariantList m_varlstData;
};

#endif //TREE_ITEM_H 
