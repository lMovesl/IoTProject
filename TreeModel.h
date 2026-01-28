#ifndef TREE_MODEL_H
#define TREE_MODEL_H

#include <QAbstractItemModel>

#include "TreeItem.h"

class TreeModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	Q_DISABLE_COPY_MOVE(TreeModel)

	explicit TreeModel(const QStringList& headers, QObject* parent = nullptr);
	~TreeModel() override;

	QVariant data(const QModelIndex& index, int role) const override;
	Qt::ItemFlags flags(const QModelIndex& index) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override;
	QModelIndex parent(const QModelIndex& index) const override;
	int rowCount(const QModelIndex& parent = {}) const override;
	int columnCount(const QModelIndex& parent = {}) const override;

	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
	bool setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role = Qt::EditRole) override;
	bool insertColumns(int position, int columns, const QModelIndex& parent = {}) override;
	bool removeColumns(int position, int columns, const QModelIndex& parent = {}) override;
	bool insertRows(int position, int rows, const QModelIndex& parent = {}) override;
	bool removeRows(int position, int rows, const QModelIndex& parent = {}) override;

	TreeItem* item(int row, const QModelIndex& parent = {}) const;
	TreeItem* getItem(const QModelIndex& index) const;
private:

	std::unique_ptr<TreeItem> m_pRootItem;
};

#endif //TREE_MODEL_H