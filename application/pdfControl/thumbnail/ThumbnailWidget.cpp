/*
 * Copyright (C) 2019 ~ 2019 Deepin Technology Co., Ltd.
 *
 * Author:     duanxiaohui
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "ThumbnailWidget.h"

#include "docview/docummentproxy.h"


#include "CentralDocPage.h"
#include "DocSheet.h"

ThumbnailWidget::ThumbnailWidget(DocSheet *sheet, DWidget *parent)
    : CustomWidget(THUMBAIL_WIDGET, parent), m_sheet(sheet)
{
    m_ThreadLoadImage.setThumbnail(this);

    initWidget();

    initConnection();

    dApp->m_pModelService->addObserver(this);
}

ThumbnailWidget::~ThumbnailWidget()
{
    // 等待子线程退出
    if (m_ThreadLoadImage.isRunning()) {
        m_ThreadLoadImage.stopThreadRun();
        m_ThreadLoadImage.clearList();
    }

    dApp->m_pModelService->removeObserver(this);
}

// 处理消息事件
int ThumbnailWidget::dealWithData(const int &msgType, const QString &msgContent)
{
    if (msgType == MSG_OPERATION_OPEN_FILE_OK)
        handleOpenSuccess();
    if (msgType == MSG_OPERATION_UPDATE_THEME) {
        slotUpdateTheme();
    } else if (msgType == MSG_VIEWCHANGE_ROTATE_VALUE) {  //  文档旋转了
        slotSetRotate(msgContent.toInt());
    } else if (msgType == MSG_OPERATION_OPEN_FILE_OK) {

    } else if (MSG_FILE_PAGE_CHANGE == msgType) {
        slotDocFilePageChanged(msgContent);
        m_pPageWidget->dealWithData(msgType, msgContent);
    }


    return MSG_NO_OK;
}

void ThumbnailWidget::handleOpenSuccess()
{
    slotOpenFileOk(m_sheet->qGetPath());
    m_pPageWidget->dealWithData(MSG_OPERATION_OPEN_FILE_OK, m_sheet->qGetPath());
}

// 初始化界面
void ThumbnailWidget::initWidget()
{
    m_pThumbnailListWidget = new CustomListWidget(m_sheet, this);
    m_pThumbnailListWidget->setSpacing(8);
    connect(m_pThumbnailListWidget, SIGNAL(sigValueChanged(const int &)), SLOT(slotLoadThumbnail(const int &)));

    m_pPageWidget = new PagingWidget(m_sheet, this);

    auto m_pvBoxLayout = new QVBoxLayout(this);
    m_pvBoxLayout->setContentsMargins(0, 0, 0, 0);
    m_pvBoxLayout->setSpacing(0);

    m_pvBoxLayout->addWidget(m_pThumbnailListWidget);
    m_pvBoxLayout->addWidget(new DHorizontalLine(this));
    m_pvBoxLayout->addWidget(m_pPageWidget);

    this->setLayout(m_pvBoxLayout);
}

//  画 选中项的背景颜色
void ThumbnailWidget::setSelectItemBackColor(QListWidgetItem *item)
{
    //  先清空之前的选中颜色
    auto curItem = m_pThumbnailListWidget->currentItem();
    if (curItem) {
        SetSelectItemColor(curItem, false);
    }

    if (item) {
        SetSelectItemColor(item, true);
    }
}

//  添加项
void ThumbnailWidget::addThumbnailItem(const int &iIndex)
{
    auto widget = new ThumbnailItemWidget(this);
    widget->setLabelPage(iIndex);

    auto item = new QListWidgetItem(m_pThumbnailListWidget);
    item->setFlags(Qt::NoItemFlags);

    int tW = LEFTMINWIDTH;
    int tH = 212;
    dApp->adaptScreenView(tW, tH);
    item->setSizeHint(QSize(tW, tH));

    m_pThumbnailListWidget->addItem(item);
    m_pThumbnailListWidget->setItemWidget(item, widget);
}

void ThumbnailWidget::initConnection()
{
    connect(&m_ThreadLoadImage, SIGNAL(sigLoadImage(const int &, const QImage &)),
            this, SLOT(slotLoadImage(const int &, const QImage &)));
    connect(&m_ThreadLoadImage, SIGNAL(sigRotateImage(const int &)), SLOT(slotRotateThumbnail(const int &)));

    connect(this, SIGNAL(sigSetRotate(const int &)), SLOT(slotSetRotate(const int &)));
}

//  文件  当前页变化, 获取与 文档页  对应的 item, 设置 选中该item, 绘制item
void ThumbnailWidget::slotDocFilePageChanged(const QString &sPage)
{
    int page = sPage.toInt();
    auto curPageItem = m_pThumbnailListWidget->item(page);
    if (curPageItem) {
        auto curItem = m_pThumbnailListWidget->currentItem();
        if (curItem == curPageItem)
            return;

        setSelectItemBackColor(curPageItem);
    }
}

void ThumbnailWidget::slotUpdateTheme()
{
    updateWidgetTheme();
}

void ThumbnailWidget::slotSetRotate(const int &angle)
{
    m_nRotate = angle;
    if (m_ThreadLoadImage.isRunning()) {
        m_ThreadLoadImage.stopThreadRun();
    }

    m_ThreadLoadImage.setIsLoaded(true);
    m_ThreadLoadImage.start();
}

void ThumbnailWidget::slotRotateThumbnail(const int &index)
{
    if (m_pThumbnailListWidget) {
        if (index >= m_pThumbnailListWidget->count()) {
            return;
        }
        auto item = m_pThumbnailListWidget->item(index);
        if (item) {
            auto pWidget = getItemWidget(item);
            if (pWidget) {
                pWidget->rotateThumbnail(m_nRotate);
            }
        }
    }
}

/**
 * @brief ThumbnailWidget::slotLoadThumbnail
 * 根据纵向滚动条来加载缩略图
 * value 为当前滚动条移动的数值,最小是0，最大是滚动条的最大值
 */
void ThumbnailWidget::slotLoadThumbnail(const int &value)
{
    if (m_nValuePreIndex < 1 || value < 1)
        return;

    int indexPage = value / m_nValuePreIndex;

    if (indexPage < 0 || indexPage > m_totalPages || m_ThreadLoadImage.inLoading(indexPage)) {
        return;
    }

    int loadStart = (indexPage - (FIRST_LOAD_PAGES / 2)) < 0 ? 0 : (indexPage - (FIRST_LOAD_PAGES / 2));
    int loadEnd = (indexPage + (FIRST_LOAD_PAGES / 2)) > m_totalPages ? m_totalPages : (indexPage + (FIRST_LOAD_PAGES / 2));
    if (m_ThreadLoadImage.isRunning()) {
        m_ThreadLoadImage.stopThreadRun();
    }
    m_ThreadLoadImage.setStartAndEndIndex(loadStart, loadEnd);
    m_ThreadLoadImage.setIsLoaded(true);
    m_ThreadLoadImage.start();
}

void ThumbnailWidget::slotLoadImage(const int &row, const QImage &image)
{
    if (!m_pThumbnailListWidget) {
        return;
    }
    auto item = m_pThumbnailListWidget->item(row);
    if (item) {
        auto pWidget = getItemWidget(item);
        if (pWidget) {
            pWidget->rotateThumbnail(m_nRotate);
            pWidget->setLabelImage(image);
        }
    }
}

void ThumbnailWidget::SlotSetBookMarkState(const int &iType, const int &iPage)
{
    bool bStatus = false;
    if (iType == -1) {
    } else {
        bStatus = true;
    }

    auto item = m_pThumbnailListWidget->item(iPage);
    if (item) {
        auto pWidget = getItemWidget(item);
        if (pWidget) {
            pWidget->qSetBookMarkShowStatus(bStatus);
        }
    }
}

// 初始化缩略图列表list，无缩略图
void ThumbnailWidget::fillContantToList()
{
    if (m_totalPages > 0) {
        for (int idex = 0; idex < m_totalPages; ++idex) {
            addThumbnailItem(idex);
        }

        showItemBookMark();
        auto item = m_pThumbnailListWidget->item(0);
        if (item) {
            SetSelectItemColor(item, true);
        }
        /**
         * @brief listCount
         * 计算每个item的大小
         */
        int listCount = m_pThumbnailListWidget->count();
        m_nValuePreIndex = m_pThumbnailListWidget->verticalScrollBar()->maximum() / listCount;
    }

    m_isLoading = false;
}

void ThumbnailWidget::showItemBookMark()
{
    QList<int> pageList = dApp->m_pDBService->getBookMarkList(m_strBindPath);
    foreach (int index, pageList) {
        auto pWidget = getItemWidget(m_pThumbnailListWidget->item(index));
        if (pWidget) {
            pWidget->qSetBookMarkShowStatus(true);
        }
    }
}

/**
 * @brief ThumbnailWidget::prevPage
 * 上一页
 */
void ThumbnailWidget::prevPage()
{
    CentralDocPage::Instance()->qDealWithData(MSG_OPERATION_PREV_PAGE, "");
}

/**
 * @brief ThumbnailWidget::nextPage
 下一页
 */
void ThumbnailWidget::nextPage()
{
    CentralDocPage::Instance()->qDealWithData(MSG_OPERATION_NEXT_PAGE, "");
}

/**
 * brief ThumbnailWidget::adaptWindowSize
 * 缩略图列表自适应视窗大小
 * @param scale  缩放因子 大于0的数
 */
void ThumbnailWidget::adaptWindowSize(const double &scale)
{
    double width = 1.0;
    double height = 1.0;

    int tW = LEFTMINWIDTH;
    int tH = 212;
    dApp->adaptScreenView(tW, tH);

    //set item size
    width = static_cast<double>(tW) * scale;
    height = static_cast<double>(tH) * scale;

    if (m_pThumbnailListWidget) {
        int itemCount = 0;
        itemCount = m_pThumbnailListWidget->count();
        if (itemCount > 0) {
            for (int index = 0; index < itemCount; index++) {
                auto item = m_pThumbnailListWidget->item(index);
                if (item) {
                    auto itemWidget = getItemWidget(item);
                    if (itemWidget) {
                        item->setSizeHint(QSize(static_cast<int>(width), static_cast<int>(height)));
                        itemWidget->adaptWindowSize(scale);
                        m_pThumbnailListWidget->update();
                    }
                }
            }
        }
    }
}

/**
 * @brief ThumbnailWidget::updateThumbnail
 * 高亮操作之后要跟换相应的缩略图
 * @param page 页码数，从0开始
 */
void ThumbnailWidget::updateThumbnail(const int &page)
{
    if (m_pThumbnailListWidget == nullptr) {
        return;
    }
    int itemNum = m_pThumbnailListWidget->count();
    if (itemNum <= 0) {
        return;
    }
    for (int index = 0; index < itemNum; index++) {
        auto item = m_pThumbnailListWidget->item(index);
        if (item) {
            auto itemWidget = getItemWidget(item);
            if (itemWidget) {
                if (itemWidget->nPageIndex() == page) {
                    CentralDocPage *pMtwe = CentralDocPage::Instance();
                    auto dproxy = pMtwe->getCurFileAndProxy(m_strBindPath);
                    if (nullptr == dproxy) {
                        return;
                    }
                    QImage image;
                    int tW = 146;
                    int tH = 174;
                    dApp->adaptScreenView(tW, tH);
                    dproxy->getImage(page, image, tW, tH);
                    itemWidget->setLabelImage(image);
                    return;
                }
            }
        }
    }
}

// 关联成功打开文件槽函数
void ThumbnailWidget::slotOpenFileOk(const QString &sPath)
{
    m_strBindPath = sPath;

    CentralDocPage *pMtwe = CentralDocPage::Instance();
    DocummentProxy *_proxy = pMtwe->getCurFileAndProxy(sPath);
    if (_proxy) {
        m_isLoading = true;
        m_ThreadLoadImage.setIsLoaded(false);
        if (m_ThreadLoadImage.isRunning()) {
            m_ThreadLoadImage.stopThreadRun();
        }

        m_totalPages = _proxy->getPageSNum();

        m_pThumbnailListWidget->clear();
        m_nValuePreIndex = 0;
        fillContantToList();

        FileDataModel fdm = CentralDocPage::Instance()->qGetFileData(sPath);
        m_nRotate = fdm.qGetData(Rotate);

        if (m_nRotate < 0) {
            m_nRotate = qAbs(m_nRotate);
        }
        m_nRotate %= 360;

        int currentPage = _proxy->currentPageNo();

        m_ThreadLoadImage.setPages(m_totalPages);
        m_ThreadLoadImage.setFilePath(sPath);
        if (!m_ThreadLoadImage.isRunning()) {
            m_ThreadLoadImage.clearList();;
            m_ThreadLoadImage.setStartAndEndIndex(currentPage - (FIRST_LOAD_PAGES / 2), currentPage + (FIRST_LOAD_PAGES / 2));
        }
        m_ThreadLoadImage.setIsLoaded(true);
        m_ThreadLoadImage.start();
        slotDocFilePageChanged(QString::number(currentPage));
    }
}

void ThumbnailWidget::SetSelectItemColor(QListWidgetItem *item, const bool &bSelect)
{
    auto pWidget = getItemWidget(item);
    if (pWidget) {
        pWidget->setBSelect(bSelect);
    }

    if (bSelect) {
        m_pThumbnailListWidget->setCurrentItem(item);
    }
}

ThumbnailItemWidget *ThumbnailWidget::getItemWidget(QListWidgetItem *item)
{
    auto pWidget = qobject_cast<ThumbnailItemWidget *>(m_pThumbnailListWidget->itemWidget(item));
    if (pWidget) {
        return pWidget;
    }
    return nullptr;
}

/*******************************ThreadLoadImage*************************************************/

ThreadLoadImage::ThreadLoadImage(QObject *parent)
    : QThread(parent)
{
}

void ThreadLoadImage::stopThreadRun()
{
    m_isLoaded = false;
    quit();
    wait();  //阻塞等待
}

bool ThreadLoadImage::inLoading(int index)
{
    if (index >=  m_nStartPage && index < m_nEndPage) {
        return true;
    }

    return false;
}

void ThreadLoadImage::setStartAndEndIndex(int startIndex, int endIndex)
{
    m_nStartPage = startIndex;  // 加载图片起始页码
    m_nEndPage = endIndex;   // 加载图片结束页码

    if (m_nStartPage < 0) {
        m_nStartPage = 0;
    }
    if (m_nEndPage >= m_pages) {
        m_nEndPage = m_pages - 1;
    }
}

// 加载缩略图线程
void ThreadLoadImage::run()
{
    do {
        msleep(50);

        if (!m_pThumbnailWidget) {
            break;
        }

        if (m_pThumbnailWidget->isLoading()) {
            break;
        }

        if (m_nStartPage < 0) {
            m_nStartPage = 0;
        }
        if (m_nEndPage >= m_pages) {
            m_nEndPage = m_pages - 1;
        }
        CentralDocPage *pMtwe = CentralDocPage::Instance();
        auto dproxy = pMtwe->getCurFileAndProxy(m_filepath);
        if (nullptr == dproxy) {
            break;
        }

        int tW = 146;
        int tH = 174;
        dApp->adaptScreenView(tW, tH);

        for (int page = m_nStartPage; page <= m_nEndPage; page++) {
            if (!m_isLoaded)
                break;
            if (m_listLoad.contains(page)) {
                emit sigRotateImage(page);
                msleep(50);
                continue;
            }
            QImage image;
            bool bl = dproxy->getImage(page, image, tW, tH);
            if (bl) {
                m_listLoad.append(page);
                emit sigLoadImage(page, image);
                msleep(50);
            }
        }

    } while (0);
}
