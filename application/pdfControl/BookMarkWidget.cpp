#include "BookMarkWidget.h"
#include <QDebug>

BookMarkWidget::BookMarkWidget(CustomWidget *parent) :
    CustomWidget("BookMarkWidget", parent)
{
    m_pVBoxLayout = new QVBoxLayout;
    m_pVBoxLayout->setContentsMargins(0, 0, 0, 0);
    m_pVBoxLayout->setSpacing(0);
    this->setLayout(m_pVBoxLayout);

    initWidget();

    connect(m_pBookMarkListWidget, SIGNAL(itemClicked(QListWidgetItem *)), this, SLOT(slotShowSelectItem(QListWidgetItem *)));
}

void BookMarkWidget::slotShowSelectItem(QListWidgetItem *item)
{
    m_pCurrentItem = item;
}

void BookMarkWidget::slotAddBookMark()
{
    qDebug() << tr("AddBookMark...");
}

void BookMarkWidget::initWidget()
{
    m_pBookMarkListWidget = new DListWidget;
    m_pBookMarkListWidget->setSpacing(10);
    //设置自动适应布局调整（Adjust适应，Fixed不适应），默认不适应
    m_pBookMarkListWidget->setResizeMode(QListWidget::Adjust);

    m_pAddBookMarkBtn = new DImageButton;
    m_pAddBookMarkBtn->setFixedSize(QSize(250, 50));
    m_pAddBookMarkBtn->setText(tr("添加书签"));
    connect(m_pAddBookMarkBtn, SIGNAL(clicked()), this, SLOT(slotAddBookMark()));

    m_pVBoxLayout->addWidget(m_pBookMarkListWidget);
    m_pVBoxLayout->addWidget(m_pAddBookMarkBtn);

    for (int page = 0; page < 20; ++page) {
        BookMarkItemWidget *t_widget = new BookMarkItemWidget;
        t_widget->setPicture(QString(tr(":/resources/image/logo/logo_big.svg")));
        t_widget->setPage(QString("页面%1").arg(page + 1));
        t_widget->setMinimumSize(QSize(250, 150));

        QListWidgetItem *item = new QListWidgetItem(m_pBookMarkListWidget);
        item->setFlags(Qt::ItemIsSelectable);
        item->setSizeHint(QSize(250, 150));

        m_pBookMarkListWidget->insertItem(page, item);
        m_pBookMarkListWidget->setItemWidget(item, t_widget);
    }
}

void BookMarkWidget::dltItem()
{
    if (m_pCurrentItem != nullptr) {
        delete m_pCurrentItem;
        m_pCurrentItem = nullptr;
    }
}

int BookMarkWidget::dealWithData(const int &msgType, const QString &msgContent)
{
    if (MSG_BOOKMARK_DLTITEM == msgType) {
        dltItem();
        qDebug() << "dlt bookmark item";
        return ConstantMsg::g_effective_res;
    }

    return 0;
}