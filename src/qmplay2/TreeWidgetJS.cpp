#include <TreeWidgetJS.hpp>

#include <QLoggingCategory>
#include <QHeaderView>
#include <QTreeWidget>

TreeWidgetItemJS::TreeWidgetItemJS()
    : m_item(new QTreeWidgetItem)
{}
TreeWidgetItemJS::~TreeWidgetItemJS()
{
    if (m_canDelete)
        delete m_item;
}

void TreeWidgetItemJS::setText(const int column, const QString &text)
{
    m_item->setText(column, text);
}
void TreeWidgetItemJS::setToolTip(const int column, const QString &text)
{
    m_item->setToolTip(column, text);
}
void TreeWidgetItemJS::setData(const int column, const int role, const QVariant &data)
{
    m_item->setData(column, role, data);
}

QTreeWidgetItem *TreeWidgetItemJS::get()
{
    m_canDelete = false;
    return m_item;
}

/**/

TreeWidgetJS::TreeWidgetJS(QTreeWidget *treeWidget, QObject *parent)
    : QObject(parent)
    , m_treeW(treeWidget)
{}
TreeWidgetJS::~TreeWidgetJS()
{}

void TreeWidgetJS::setColumnCount(const int columns)
{
    m_treeW->setColumnCount(columns);
}

void TreeWidgetJS::sortByColumn(const int column, const int order)
{
    m_treeW->sortByColumn(
        column,
        static_cast<Qt::SortOrder>(qBound(0, order, 1))
    );
}

void TreeWidgetJS::setHeaderItemText(const int column, const QString &name)
{
    m_treeW->headerItem()->setText(column, name);
}
void TreeWidgetJS::setHeaderSectionResizeMode(const int logicalIndex, const int resizeMode)
{
    m_treeW->header()->setSectionResizeMode(
        logicalIndex,
        static_cast<QHeaderView::ResizeMode>(qBound(0, resizeMode, 3))
    );
}

void TreeWidgetJS::addTopLevelItem(TreeWidgetItemJS *item)
{
    if (item)
        m_treeW->addTopLevelItem(item->get());
}
