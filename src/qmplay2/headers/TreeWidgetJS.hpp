#pragma once

#include <QMPlay2Lib.hpp>

#include <QObject>

class QTreeWidgetItem;

class QMPLAY2SHAREDLIB_EXPORT TreeWidgetItemJS : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE TreeWidgetItemJS();
    ~TreeWidgetItemJS();

    Q_INVOKABLE void setText(const int column, const QString &text);
    Q_INVOKABLE void setToolTip(const int column, const QString &text);
    Q_INVOKABLE void setData(const int column, const int role, const QVariant &data);

public:
    QTreeWidgetItem *get();

private:
    QTreeWidgetItem *const m_item;
    bool m_canDelete = true;
};

/**/

class QTreeWidget;

class QMPLAY2SHAREDLIB_EXPORT TreeWidgetJS : public QObject
{
    Q_OBJECT

public:
    TreeWidgetJS(QTreeWidget *treeWidget, QObject *parent = nullptr);
    ~TreeWidgetJS();

    Q_INVOKABLE void setColumnCount(const int columns);

    Q_INVOKABLE void sortByColumn(const int column, const int order);

    Q_INVOKABLE void setHeaderItemText(const int column, const QString &name);
    Q_INVOKABLE void setHeaderSectionResizeMode(const int logicalIndex, const int resizeMode);

    Q_INVOKABLE void addTopLevelItem(TreeWidgetItemJS *item);

private:
    QTreeWidget *const m_treeW;
};
