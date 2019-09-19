#include "docummentdjvu.h"
#include "pagedjvu.h"
#include "docview/publicfunc.h"
#include <DScrollBar>
#include <QImage>
#include <QTemporaryFile>
#include <QFileInfo>
//#include <QPropertyAnimation>
//#include <QParallelAnimationGroup>
#include <QDebug>

void waitForMessageTag(ddjvu_context_t *context, ddjvu_message_tag_t tag)
{
    ddjvu_message_wait(context);

    while (true) {
        ddjvu_message_t *message = ddjvu_message_peek(context);

        if (message != 0) {
            if (message->m_any.tag == tag) {
                break;
            }

            ddjvu_message_pop(context);
        } else {
            break;
        }
    }
}

DocummentDJVU::DocummentDJVU(DWidget *parent): DocummentBase(parent),
    document(nullptr),
    m_listsearch(),
    m_fileinfo(),
    m_pageByName(),
    m_titleByIndex()
{
    m_threadloaddoc.setDoc(this);
    m_threadloadwords.setDoc(this);
    setWidgetResizable(true);
    setWidget(&m_widget);
    m_scale = 1;
    m_rotate = RotateType_Normal;
    m_currentpageno = 0;
    pblankwidget = new DWidget(this);
    pblankwidget->setMouseTracking(true);
    pblankwidget->hide();
    donotneedreloaddoc = false;
}

bool DocummentDJVU::openFile(QString filepath)
{
    m_context = ddjvu_context_create("deepin_reader");

    if (m_context == 0) {
        return false;
    }

#if DDJVUAPI_VERSION >= 19

    document = ddjvu_document_create_by_filename_utf8(m_context, filepath.toUtf8(), FALSE);

#else

    document = ddjvu_document_create_by_filename(context, QFile::encodeName(filepath), FALSE);

#endif // DDJVUAPI_VERSION

    if (document == 0) {
        ddjvu_context_release(m_context);

        return false;
    }

    waitForMessageTag(m_context, DDJVU_DOCINFO);

    if (ddjvu_document_decoding_error(document)) {
        ddjvu_document_release(document);
        ddjvu_context_release(m_context);

        return false;
    }
    unsigned int mask[] = {0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000};

    m_format = ddjvu_format_create(DDJVU_FORMAT_RGBMASK32, 4, mask);
    ddjvu_format_set_row_order(m_format, 1);
    ddjvu_format_set_y_direction(m_format, 1);
    setBasicInfo(filepath);
    donotneedreloaddoc = true;
    m_pages.clear();
    qDebug() << "djvu numPages :" << ddjvu_document_get_pagenum(document);
    for (int i = 0; i < ddjvu_document_get_pagenum(document); i++) {
        DWidget *qwidget = new DWidget(this);
        QHBoxLayout *qhblayout = new QHBoxLayout(qwidget);
        qhblayout->setAlignment(qwidget, Qt::AlignCenter);
        qwidget->setLayout(qhblayout);
        m_vboxLayout.addWidget(qwidget);
        //        m_vboxLayout.addWidget(m_pages.at(i));
        m_vboxLayout.setAlignment(&m_widget, Qt::AlignCenter);
        qwidget->setMouseTracking(true);
        m_widgets.append(qwidget);

        PageDJVU *page = new PageDJVU(this);
        page->setPage(i);
        m_pages.append((PageBase *)page);
    }

    for (int i = 0; i < m_pages.size(); i++) {
        PageDJVU *page = (PageDJVU *)m_pages.at(i);
        page->setScaleAndRotate(m_scale, m_rotate);
    }
    setViewModeAndShow(m_viewmode);
    donotneedreloaddoc = false;
    if (m_threadloaddoc.isRunning())
        m_threadloaddoc.setRestart();
    else
        m_threadloaddoc.start();
    if (m_threadloadwords.isRunning())
        m_threadloadwords.setRestart();
    else
        m_threadloadwords.start();

    return true;
}

//bool DocummentDJVU::setViewModeAndShow(ViewMode_EM viewmode)
//{
//    int currpageno = m_currentpageno;
//    m_viewmode = viewmode;
//    donotneedreloaddoc = true;
//    switch (m_viewmode) {
//    case ViewMode_SinglePage:
//        showSinglePage();
//        break;
//    case ViewMode_FacingPage:
//        showFacingPage();
//        break;
//    default:
//        return false;
//        break;
//    }
//    pageJump(currpageno);
//    donotneedreloaddoc = false;
//    if (m_threadloaddoc.isRunning())
//        m_threadloaddoc.setRestart();
//    else
//        m_threadloaddoc.start();
//    return true;;
//}

//void DocummentDJVU::showSinglePage()
//{
//    pblankwidget->hide();
//    for (int i = 0; i < m_pages.size(); i++) {
//        m_widgets.at(i)->layout()->addWidget(m_pages.at(i));
//        m_widgets.at(i)->show();
//    }
//    int rex = m_vboxLayout.margin(), rey = m_vboxLayout.margin();
//    for (int i = 0; i < m_widgets.size(); i++) {
//        m_widgets.at(i)->setGeometry(rex, rey, m_widgets.at(i)->layout()->margin() * 2 + m_pages.at(i)->width(), m_widgets.at(i)->layout()->margin() * 2 + m_pages.at(i)->height());
//        rey += m_widgets.at(i)->layout()->margin() * 2 + m_pages.at(i)->height() + m_vboxLayout.spacing();
//    }
//}
//void DocummentDJVU::showFacingPage()
//{
//    for (int i = 0; i < m_widgets.size(); i++) {
//        m_widgets.at(i)->hide();
//    }
//    pblankwidget->hide();
//    for (int i = 0; i < m_pages.size() / 2; i++) {
//        m_widgets.at(i)->layout()->addWidget(m_pages.at(i * 2));
//        m_widgets.at(i)->layout()->addWidget(m_pages.at(i * 2 + 1));
//        m_widgets.at(i)->show();
//    }
//    if (m_pages.size() % 2) {
//        pblankwidget->show();
//        m_widgets.at(m_pages.size() / 2)->layout()->addWidget(m_pages.at(m_pages.size() - 1));
//        m_widgets.at(m_pages.size() / 2)->layout()->addWidget(pblankwidget);
//        m_widgets.at(m_pages.size() / 2)->show();
//    }
//    int rex = m_vboxLayout.margin(), rey = m_vboxLayout.margin();
//    for (int i = 0; i < m_widgets.size() / 2; i++) {
//        int reheight = 0;
//        if (m_pages.at(i * 2)->height() < m_pages.at(i * 2 + 1)->height()) {
//            reheight = m_pages.at(i * 2 + 1)->height();
//        } else {
//            reheight = m_pages.at(i * 2)->height();
//        }
//        m_widgets.at(i)->setGeometry(rex, rey, m_widgets.at(i)->layout()->margin() * 2 + m_widgets.at(i)->layout()->spacing() + m_pages.at(i * 2)->width() + m_pages.at(i * 2 + 1)->width(), m_widgets.at(i)->layout()->margin() * 2 + reheight);
//        rey += m_widgets.at(i)->layout()->margin() * 2 + reheight + m_vboxLayout.spacing();
//    }
//    if (m_pages.size() % 2) {
//        int reheight = m_pages.at(m_pages.size() - 1)->height();
//        m_widgets.at(m_widgets.size() / 2)->setGeometry(rex, rey, m_widgets.at(m_widgets.size() / 2)->layout()->margin() * 2 + m_widgets.at(m_widgets.size() / 2)->layout()->spacing() + m_pages.at(m_pages.size() - 1)->width() * 2, m_widgets.at(m_widgets.size() / 2)->layout()->margin() * 2 + reheight);
//    }
//}

bool DocummentDJVU::loadPages()
{
    if (!document && m_pages.size() == ddjvu_document_get_pagenum(document))
        return false;
    qDebug() << "loadPages";
    //    for (int i = 0; i < m_pages.size(); i++) {
    int startnum = m_currentpageno - 3;
    if (startnum < 0) {
        startnum = 0;
    }
    int endnum = startnum + 7;
    if (endnum > m_pages.size()) {
        endnum = m_pages.size();
    }
    for (int i = startnum; i < endnum; i++) {
        PageDJVU *ppdf = (PageDJVU *)m_pages.at(i);
        ppdf->showImage(m_scale, m_rotate);
    }
    return true;
}

bool DocummentDJVU::loadWords()
{
    if (!document && m_pages.size() == ddjvu_document_get_pagenum(document))
        return false;
    qDebug() << "loadWords";
    for (int i = 0; i < m_pages.size(); i++) {
        PageDJVU *pdjvu = (PageDJVU *)m_pages.at(i);
        pdjvu->loadWords();
        pdjvu->loadLinks();
    }
    return true;
}

//void DocummentDJVU::scaleAndShow(double scale, RotateType_EM rotate)
//{
//    int currpageno = m_currentpageno;
//    m_scale = scale;
//    m_rotate = rotate;
//    donotneedreloaddoc = true;
//    for (int i = 0; i < m_pages.size(); i++) {
//        PageDJVU *page = (PageDJVU *)m_pages.at(i);
//        page->setScaleAndRotate(m_scale, m_rotate);
//    }

//    int rex = m_vboxLayout.margin(), rey = m_vboxLayout.margin();

//    switch (m_viewmode) {
//    case ViewMode_SinglePage:
//        for (int i = 0; i < m_widgets.size(); i++) {
//            m_widgets.at(i)->setGeometry(rex, rey, m_widgets.at(i)->layout()->margin() * 2 + m_pages.at(i)->width(), m_widgets.at(i)->layout()->margin() * 2 + m_pages.at(i)->height());
//            rey += m_widgets.at(i)->layout()->margin() * 2 + m_pages.at(i)->height() + m_vboxLayout.spacing();
//        }
//        break;
//    case ViewMode_FacingPage:
//        for (int i = 0; i < m_widgets.size() / 2; i++) {
//            int reheight = 0;
//            if (m_pages.at(i * 2)->height() < m_pages.at(i * 2 + 1)->height()) {
//                reheight = m_pages.at(i * 2 + 1)->height();
//            } else {
//                reheight = m_pages.at(i * 2)->height();
//            }
//            m_widgets.at(i)->setGeometry(rex, rey, m_widgets.at(i)->layout()->margin() * 2 + m_widgets.at(i)->layout()->spacing() + m_pages.at(i * 2)->width() + m_pages.at(i * 2 + 1)->width(), m_widgets.at(i)->layout()->margin() * 2 + reheight);
//            rey += m_widgets.at(i)->layout()->margin() * 2 + reheight + m_vboxLayout.spacing();
//        }
//        if (m_widgets.size() % 2) {
//            int reheight = m_pages.at(m_pages.size() - 1)->height();
//            m_widgets.at(m_widgets.size() / 2)->setGeometry(rex, rey, m_widgets.at(m_widgets.size() / 2)->layout()->margin() * 2 + m_widgets.at(m_widgets.size() / 2)->layout()->spacing() + m_pages.at(m_pages.size() - 1)->width() * 2, m_widgets.at(m_widgets.size() / 2)->layout()->margin() * 2 + reheight);
//        }
//        break;
//    default:
//        break;
//    }
//    pageJump(currpageno);
//    donotneedreloaddoc = false;
//    m_currentpageno = currpageno;
//    if (m_threadloaddoc.isRunning())
//        m_threadloaddoc.setRestart();
//    else
//        m_threadloaddoc.start();
//}
void DocummentDJVU::removeAllAnnotation()
{
    if (!document)return;
//    for (int i = 0; i < document->numPages(); ++i) {
//        QList<Poppler::Annotation *> listannote = document->page(i)->annotations();
//        foreach (Poppler::Annotation *atmp, listannote) {
//            document->page(i)->removeAnnotation(atmp);
//        }
//    }
//    scaleAndShow(m_scale, m_rotate);
}

QString DocummentDJVU::removeAnnotation(const QPoint &startpos)
{
    //暂时只处理未旋转
    QPoint pt = startpos;
    int page = pointInWhichPage(pt);
    if (page < 0) return "";
    return static_cast<PageDJVU *>(m_pages.at(page))->removeAnnotation(pt);
}

void DocummentDJVU::removeAnnotation(const QString &struuid)
{
    return static_cast<PageDJVU *>(m_pages.at(currentPageNo()))->removeAnnotation(struuid);
}

QString DocummentDJVU::addAnnotation(const QPoint &startpos, const QPoint &endpos, QColor color)
{
    QPoint pt = startpos;
    int page = pointInWhichPage(pt);
    if (page < 0) return "";
    return static_cast<PageDJVU *>(m_pages.at(page))->addAnnotation(pt);
}

void DocummentDJVU::search(const QString &strtext, QMap<int, stSearchRes> &resmap, QColor color)
{
//    //先清理
//    if (m_listsearch.size() > 0) {
//        clearSearch();
//    }

//    for (int i = 0; i < document->numPages(); ++i) {
//        stSearchRes stres;
//        searchHightlight(document->page(i), strtext, stres, color);
//        if (stres.listtext.size() > 0)
//            resmap.insert(i, stres);
//    }
//    static_cast<PageDJVU *>(m_pages.at(currentPageNo()))->showImage(m_scale, m_rotate);
//    scaleAndShow(m_scale, m_rotate); //全部刷新
}

bool DocummentDJVU::save(const QString &filePath, bool withChanges)
{
    // Save document to temporary file...
    QTemporaryFile temporaryFile;
    temporaryFile.setFileTemplate(temporaryFile.fileTemplate() + QLatin1String(".") + QFileInfo(filePath).suffix());
    if (!temporaryFile.open()) {
        return false;
    }

    temporaryFile.close();

    if (!pdfsave(temporaryFile.fileName(), withChanges)) {
        return false;
    }

    // Copy from temporary file to actual file...
    QFile file(filePath);

    if (!temporaryFile.open()) {
        return false;
    }

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    if (!PublicFunc::copyFile(temporaryFile, file)) {
        return false;
    }

    if (withChanges) {
        // m_bModified = false;//reset modify status
        setBasicInfo(filePath);
    }

    return true;
}

bool DocummentDJVU::pdfsave(const QString &filePath, bool withChanges) const
{
//    QScopedPointer< Poppler::PDFConverter > pdfConverter(document->pdfConverter());

//    pdfConverter->setOutputFileName(filePath);

//    Poppler::PDFConverter::PDFOptions options = pdfConverter->pdfOptions();

//    if (withChanges) {
//        options |= Poppler::PDFConverter::WithChanges;
//    }

//    pdfConverter->setPDFOptions(options);

//    return pdfConverter->convert();
    return  false;

}

void DocummentDJVU::clearSearch()
{
//    for (int i = 0; i < document->numPages(); ++i) {
//        foreach (Poppler::Annotation *ptmp, m_listsearch) {
//            document->page(i)->removeAnnotation(ptmp);
//        }
//        //refresh(i);
//    }
//    scaleAndShow(m_scale, m_rotate);
}

void DocummentDJVU::searchHightlight(Poppler::Page *page, const QString &strtext, stSearchRes &stres, const QColor &color)
{
    if (nullptr == page) return;
    QList<QRectF> listrect = page->search(strtext);
    if (listrect.size() <= 0)return;

    if (listrect.size() <= 0)return;
    Poppler::Annotation::Style style;
    style.setColor(color);

    Poppler::Annotation::Popup popup;
    popup.setFlags(Poppler::Annotation::Hidden | Poppler::Annotation::ToggleHidingOnMouse);

    Poppler::HighlightAnnotation *annotation = new Poppler::HighlightAnnotation();

    Poppler::HighlightAnnotation::Quad quad;
    QList<Poppler::HighlightAnnotation::Quad> qlistquad;
    QRectF rec, recboundary;
    foreach (rec, listrect) {
        //获取搜索结果附近文字
        QRectF rctext = rec;
        rctext.setX(rctext.x() - 40);
        rctext.setWidth(rctext.width() + 80);
        stres.listtext.push_back(page->text(rctext));
        recboundary.setTopLeft(QPointF(rec.left() / page->pageSizeF().width(),
                                       rec.top() / page->pageSizeF().height()));
        recboundary.setTopRight(QPointF(rec.right() / page->pageSizeF().width(),
                                        rec.top() / page->pageSizeF().height()));
        recboundary.setBottomLeft(QPointF(rec.left() / page->pageSizeF().width(),
                                          rec.bottom() / page->pageSizeF().height()));
        recboundary.setBottomRight(QPointF(rec.right() / page->pageSizeF().width(),
                                           rec.bottom() / page->pageSizeF().height()));

        qDebug() << "**" << rec << "**";
        quad.points[0] = recboundary.topLeft();
        quad.points[1] = recboundary.topRight();
        quad.points[2] = recboundary.bottomRight();
        quad.points[3] = recboundary.bottomLeft();
        qlistquad.append(quad);
    }
    annotation->setHighlightQuads(qlistquad);
    //annotation->setBoundary(recboundary);
    annotation->setStyle(style);
    annotation->setPopup(popup);
    page->addAnnotation(annotation);
    m_listsearch.append(annotation);
}

void DocummentDJVU::refreshOnePage(int ipage)
{
    if (!document)
        return ;
    PageDJVU *ppdf = (PageDJVU *)m_pages.at(ipage);
    ppdf->showImage(m_scale, m_rotate);
}

void DocummentDJVU::setBasicInfo(const QString &filepath)
{
    QFileInfo info(filepath);
    m_fileinfo.size = info.size();
    m_fileinfo.CreateTime = info.birthTime();
    m_fileinfo.ChangeTime = info.lastModified();
    m_fileinfo.strAuther = info.owner();
    m_fileinfo.strFilepath = info.filePath();
    if (document) {
        int major, minor;
        //document->getPdfVersion(&major, &minor);
        m_fileinfo.strFormat.arg("PDF v.%1.%2", major, minor);
//        m_fileinfo.strKeyword = document->info(QStringLiteral("Keywords"));
//        m_fileinfo.strTheme = document->title();
//        m_fileinfo.strProducter = document->producer();
//        m_fileinfo.strCreater = document->creator();
//        m_fileinfo.bsafe = document->isEncrypted();
//        m_fileinfo.iWidth = document->page(0)->pageSize().width();
//        m_fileinfo.iHeight = document->page(0)->pageSize().height();
//        m_fileinfo.iNumpages = document->numPages();

        /*---------djyu code--------*/
        for (int index = 0, count = ddjvu_document_get_filenum(document); index < count; ++index) {
            ddjvu_fileinfo_t fileinfo;

            if (ddjvu_document_get_fileinfo(document, index, &fileinfo) != DDJVU_JOB_OK || fileinfo.type != 'P') {
                continue;
            }

            const QString id = QString::fromUtf8(fileinfo.id);
            const QString name = QString::fromUtf8(fileinfo.name);
            const QString title = QString::fromUtf8(fileinfo.title);

            m_pageByName[id] = m_pageByName[name] = m_pageByName[title] = fileinfo.pageno + 1;

            if (!title.endsWith(".djvu", Qt::CaseInsensitive) && !title.endsWith(".djv", Qt::CaseInsensitive)) {
                m_titleByIndex[fileinfo.pageno] = title;
            }
        }

        m_pageByName.squeeze();
        m_titleByIndex.squeeze();
    }
}

//void DocummentDJVU::loadWordCache(int indexpage, PageBase *page)
//{
//    if (!document) {
//        return;
//    }
//    Poppler::Page *pp = document->page(indexpage);
//    QList<Poppler::TextBox *> textList;
//    pp->textList();
//    if (pp) {
//        textList = pp->textList();
//        const QSizeF s = pp->pageSizeF();
//    }
//    delete pp;

//    abstractTextPage(textList, page);
//    qDeleteAll(textList);
//    PageDJVU *ppdf = (PageDJVU *)page;
//    ppdf->loadLinks();
//}

//bool DocummentDJVU::abstractTextPage(const QList<Poppler::TextBox *> &text, PageBase *page)
//{
//    Poppler::TextBox *next;
//    PageDJVU *ppdf = (PageDJVU *)page;
//    QString s;
//    bool addChar;
//    foreach (Poppler::TextBox *word, text) {
//        const int qstringCharCount = word->text().length();
//        next = word->nextWord();
//        // if(next)
//        int textBoxChar = 0;
//        for (int j = 0; j < qstringCharCount; j++) {
//            const QChar c = word->text().at(j);
//            if (c.isHighSurrogate()) {
//                s = c;
//                addChar = false;
//            } else if (c.isLowSurrogate()) {
//                s += c;
//                addChar = true;
//            } else {
//                s = c;
//                addChar = true;
//            }

//            if (addChar) {
//                QRectF charBBox = word->charBoundingBox(textBoxChar);
//                //                qDebug() << "addChar s:" << s << " charBBox:" << charBBox;
//                stWord sword;
//                sword.s = s;
//                sword.rect = charBBox;
//                ppdf->appendWord(sword);
//                textBoxChar++;
//            }
//        }

//        if (word->hasSpaceAfter() && next) {
//            QRectF wordBBox = word->boundingBox();
//            QRectF nextWordBBox = next->boundingBox();
//            //            qDebug() << "hasSpaceAfter wordBBox:" << wordBBox << " nextWordBBox:" << nextWordBBox;
//            stWord sword;
//            sword.s = QStringLiteral(" ");
//            QRectF qrect;
//            qrect.setLeft(wordBBox.right());
//            qrect.setBottom(wordBBox.bottom());
//            qrect.setRight(nextWordBBox.left());
//            qrect.setTop(wordBBox.top());
//            sword.rect = qrect;
//            ppdf->appendWord(sword);
//        }
//    }
//    return true;
//}

//void DocummentDJVU::mouseSelectTextClear()
//{
//    for (int i = 0; i < m_pages.size(); i++) {
//        PageDJVU *ppdf = (PageDJVU *)m_pages.at(i);
//        ppdf->clearPageTextSelections();
//    }
//}

bool DocummentDJVU::bDocummentExist()
{
    if (!document) {
        return false;
    }
    return true;
}

//QPoint DocummentDJVU::global2RelativePoint(QPoint globalpoint)
//{
//    int x_offset = 0;
//    int y_offset = 0;
//    DScrollBar *scrollBar_X = horizontalScrollBar();
//    if (scrollBar_X)
//        x_offset = scrollBar_X->value();
//    DScrollBar *scrollBar_Y = verticalScrollBar();
//    if (scrollBar_Y)
//        y_offset = scrollBar_Y->value();
//    QPoint qpoint = QPoint(mapFromGlobal(globalpoint).x() + x_offset,
//                           mapFromGlobal(globalpoint).y() + y_offset);
//    //    qDebug() << "globalpoint:" << globalpoint << " relativepoint:" << qpoint;
//    return qpoint;
//}

//bool DocummentDJVU::setSelectTextStyle(QColor paintercolor, QColor pencolor, int penwidth)
//{
//    if (!document) {
//        return false;
//    }
//    for (int i = 0; i < m_pages.size(); i++) {
//        PageDJVU *ppdf = (PageDJVU *)m_pages.at(i);
//        ppdf->setSelectTextStyle(paintercolor, pencolor, penwidth);
//    }
//}

//bool DocummentDJVU::mouseSelectText(QPoint start, QPoint stop)
//{
//    if (!document) {
//        return false;
//    }
//    QPoint qstart = start;
//    QPoint qstop = stop;
//    qDebug() << "startpoint:" << start << " stoppoint:" << stop;
//    int startpagenum = -1, endpagenum = -1;

//    for (int i = 0; i < m_widgets.size(); i++) {
//        if (qstop.x() > m_widgets.at(i)->x() &&
//                qstop.x() <
//                (m_widgets.at(i)->width() + m_widgets.at(i)->x()) &&
//                qstop.y() > m_widgets.at(i)->y() &&
//                qstop.y() <
//                (m_widgets.at(i)->height() + m_widgets.at(i)->y())) {
//            qstop = QPoint(qstop.x() - m_widgets.at(i)->x(), qstop.y() - m_widgets.at(i)->y());
//            switch (m_viewmode) {
//            case ViewMode_SinglePage:
//                endpagenum = i;
//                break;
//            case ViewMode_FacingPage:
//                if (qstop.x() > m_pages.at(2 * i)->x() &&
//                        qstop.x() <
//                        (m_pages.at(2 * i)->width() + m_pages.at(2 * i)->x()) &&
//                        qstop.y() > m_pages.at(2 * i)->y() &&
//                        qstop.y() <
//                        (m_pages.at(2 * i)->height() + m_pages.at(2 * i)->y())) {
//                    endpagenum = 2 * i;
//                } else {
//                    endpagenum = 2 * i + 1;
//                    if (endpagenum >= m_pages.size()) {
//                        endpagenum = 2 * i;
//                        qstop = QPoint(m_pages.at(endpagenum)->width() + m_pages.at(endpagenum)->x(),
//                                       m_pages.at(endpagenum)->height() + m_pages.at(endpagenum)->y());
//                    }
//                }
//                break;
//            default:
//                break;
//            }
//            break;
//        }
//    }


//    for (int i = 0; i < m_widgets.size(); i++) {
//        if (qstart.x() > m_widgets.at(i)->x() &&
//                qstart.x() <
//                (m_widgets.at(i)->width() + m_widgets.at(i)->x()) &&
//                qstart.y() > m_widgets.at(i)->y() &&
//                qstart.y() <
//                (m_widgets.at(i)->height() + m_widgets.at(i)->y())) {
//            qstart = QPoint(qstart.x() - m_widgets.at(i)->x(), qstart.y() - m_widgets.at(i)->y());
//            switch (m_viewmode) {
//            case ViewMode_SinglePage:
//                startpagenum = i;
//                break;
//            case ViewMode_FacingPage:
//                if (qstart.x() > m_pages.at(2 * i)->x() &&
//                        qstart.x() <
//                        (m_pages.at(2 * i)->width() + m_pages.at(2 * i)->x()) &&
//                        qstart.y() > m_pages.at(2 * i)->y() &&
//                        qstart.y() <
//                        (m_pages.at(2 * i)->height() + m_pages.at(2 * i)->y())) {
//                    startpagenum = 2 * i;
//                } else {
//                    startpagenum = 2 * i + 1;
//                    if (startpagenum >= m_pages.size()) {
//                        startpagenum = 2 * i;
//                        qstart = QPoint(m_pages.at(startpagenum)->width() + m_pages.at(startpagenum)->x(),
//                                        m_pages.at(startpagenum)->height() + m_pages.at(startpagenum)->y());
//                    }
//                }
//                break;
//            default:
//                break;
//            }
//            break;
//        }
//    }

//    //    qDebug() << "startpagenum:" << startpagenum << " endpagenum:" << endpagenum;
//    if (-1 == startpagenum || -1 == endpagenum)
//        return false;
//    if (startpagenum > endpagenum) {
//        int mi = startpagenum;
//        startpagenum = endpagenum;
//        endpagenum = mi;
//        QPoint mp = qstart;
//        qstart = qstop;
//        qstop = mp;
//    }
//    //    qDebug() << "startpagenum:" << startpagenum << " endpagenum:" << endpagenum;
//    bool re = false;
//    for (int i = startpagenum; i < endpagenum + 1; i++) {
//        PageDJVU *ppdf = (PageDJVU *)m_pages.at(i);
//        QPoint pfirst = QPoint(m_pages.at(i)->x(), m_pages.at(i)->y());
//        QPoint plast = QPoint(m_pages.at(i)->width() + m_pages.at(i)->x(),
//                              m_pages.at(i)->height() + m_pages.at(i)->y());
//        switch (m_rotate) {
//        case RotateType_90:
//            pfirst = QPoint(m_pages.at(i)->x() + m_pages.at(i)->width(), m_pages.at(i)->y());
//            plast = QPoint(m_pages.at(i)->x(),
//                           m_pages.at(i)->height() + m_pages.at(i)->y());
//            break;
//        case RotateType_180:
//            pfirst = QPoint(m_pages.at(i)->x() + m_pages.at(i)->width(), m_pages.at(i)->y());
//            plast = QPoint(m_pages.at(i)->x(), m_pages.at(i)->y());
//            break;
//        case RotateType_270:
//            pfirst = QPoint(m_pages.at(i)->x(), m_pages.at(i)->height() + m_pages.at(i)->y());
//            plast = QPoint(m_pages.at(i)->x() + m_pages.at(i)->width(),
//                           m_pages.at(i)->y());
//            break;
//        default:
//            break;
//        }
//        if (i == startpagenum) {
//            if (startpagenum == endpagenum) {
//                re = ppdf->pageTextSelections(qstart, qstop);
//            } else {
//                re = ppdf->pageTextSelections(qstart,
//                                              plast);
//            }
//        } else if (i == endpagenum) {
//            re = ppdf->pageTextSelections(
//                     pfirst,
//                     qstop);
//        } else {
//            re = ppdf->pageTextSelections(pfirst,
//                                          plast);
//        }
//    }
//    return re;
//}

//int DocummentDJVU::pointInWhichPage(QPoint &qpoint)
//{
//    int pagenum = -1;
//    for (int i = 0; i < m_widgets.size(); i++) {
//        if (qpoint.x() > m_widgets.at(i)->x() &&
//                qpoint.x() <
//                (m_widgets.at(i)->width() + m_widgets.at(i)->x()) &&
//                qpoint.y() > m_widgets.at(i)->y() &&
//                qpoint.y() <
//                (m_widgets.at(i)->height() + m_widgets.at(i)->y())) {
//            qpoint = QPoint(qpoint.x() - m_widgets.at(i)->x(), qpoint.y() - m_widgets.at(i)->y());
//            switch (m_viewmode) {
//            case ViewMode_SinglePage:
//                pagenum = i;
//                //                qpoint = QPoint(qpoint.x() - m_widgets.at(i)->x(), qpoint.y() - m_widgets.at(i)->y());
//                break;
//            case ViewMode_FacingPage:
//                //                qpoint = QPoint(qpoint.x() - m_widgets.at(i)->x(), qpoint.y() - m_widgets.at(i)->y());
//                if (qpoint.x() > m_pages.at(2 * i)->x() &&
//                        qpoint.x() <
//                        (m_pages.at(2 * i)->width() + m_pages.at(2 * i)->x()) &&
//                        qpoint.y() > m_pages.at(2 * i)->y() &&
//                        qpoint.y() <
//                        (m_pages.at(2 * i)->height() + m_pages.at(2 * i)->y())) {
//                    pagenum = 2 * i;
//                } else {
//                    pagenum = 2 * i + 1;
//                    if (pagenum >= m_pages.size())
//                        return -1;
//                }
//                break;
//            default:
//                break;
//            }
//            break;
//        }
//    }
//    return pagenum;
//}

//bool DocummentDJVU::mouseBeOverText(QPoint point)
//{
//    if (!document) {
//        return false;
//    }
//    QPoint qpoint = point;
//    int pagenum = -1;
//    pagenum = pointInWhichPage(qpoint);
//    qDebug() << "mouseBeOverText pagenum:" << pagenum;
//    if (-1 != pagenum) {
//        PageDJVU *ppdf = (PageDJVU *)m_pages.at(pagenum);
//        return ppdf ->ifMouseMoveOverText(qpoint);
//    }
//    return false;
//}

bool DocummentDJVU::getImage(int pagenum, QImage &image, double width, double height)
{
    PageDJVU *ppdf = (PageDJVU *)m_pages.at(pagenum);
    return ppdf->getImage(image, width, height);
}
//int DocummentDJVU::getPageSNum()
//{
//    return m_pages.size();
//}

//bool DocummentDJVU::showMagnifier(QPoint point)
//{
//    if (!document || !m_magnifierwidget) {
//        return false;
//    }
//    QPoint qpoint = point;
//    int pagenum = -1;
//    int x_offset = 0;
//    int y_offset = 0;
//    DScrollBar *scrollBar_X = horizontalScrollBar();
//    if (scrollBar_X)
//        x_offset = scrollBar_X->value();
//    DScrollBar *scrollBar_Y = verticalScrollBar();
//    if (scrollBar_Y)
//        y_offset = scrollBar_Y->value();
//    QPoint gpoint = m_magnifierwidget->mapFromGlobal(mapToGlobal(QPoint(point.x() - x_offset, point.y() - y_offset)));
//    pagenum = pointInWhichPage(qpoint);
//    qDebug() << "showMagnifier pagenum:" << pagenum;
//    if (-1 != pagenum) {
//        if (pagenum != m_lastmagnifierpagenum && -1 != m_lastmagnifierpagenum) {
//            if (pagenum > m_lastmagnifierpagenum && m_lastmagnifierpagenum - 3 > 0) {
//                PageDJVU *ppdf = (PageDJVU *)m_pages.at(m_lastmagnifierpagenum - 3);
//                ppdf->clearMagnifierPixmap();
//                if (pagenum - m_lastmagnifierpagenum > 1) {
//                    ppdf = (PageDJVU *)m_pages.at(m_lastmagnifierpagenum - 2);
//                    ppdf->clearMagnifierPixmap();
//                }
//            } else if (pagenum < m_lastmagnifierpagenum && m_lastmagnifierpagenum + 3 < m_pages.size()) {
//                PageDJVU *ppdf = (PageDJVU *)m_pages.at(m_lastmagnifierpagenum + 3);
//                ppdf->clearMagnifierPixmap();
//                if ( m_lastmagnifierpagenum - pagenum > 1) {
//                    ppdf = (PageDJVU *)m_pages.at(m_lastmagnifierpagenum + 2);
//                    ppdf->clearMagnifierPixmap();
//                }
//            }
//            for (int i = pagenum - 3; i < pagenum + 4; i++) {
//                if (i > 0 && i < m_pages.size()) {
//                    PageDJVU *ppdf = (PageDJVU *)m_pages.at(i);
//                    ppdf->loadMagnifierCacheThreadStart(ppdf->width() *m_magnifierwidget->getMagnifierScale(), ppdf->height() *m_magnifierwidget->getMagnifierScale());
//                }
//            }
//        }
//        m_lastmagnifierpagenum = pagenum;
//        PageDJVU *ppdf = (PageDJVU *)m_pages.at(pagenum);
//        QPixmap pixmap;
//        if (ppdf ->getMagnifierPixmap(pixmap, qpoint, m_magnifierwidget->getMagnifierRadius(), ppdf->width() *m_magnifierwidget->getMagnifierScale(), ppdf->height() *m_magnifierwidget->getMagnifierScale())) {
//            m_magnifierwidget->setPixmap(pixmap);

//            m_magnifierwidget->setPoint(gpoint);
//            m_magnifierwidget->show();
//            m_magnifierwidget->update();
//        }
//    } else {
//        QPixmap pix(m_magnifierwidget->getMagnifierRadius() * 2, m_magnifierwidget->getMagnifierRadius() * 2);
//        pix.fill(Qt::transparent);
//        m_magnifierwidget->setPixmap(pix);
//        m_magnifierwidget->setPoint(gpoint);
//        m_magnifierwidget->show();
//        m_magnifierwidget->update();
//    }
//    int radius = m_magnifierwidget->getMagnifierRadius() - m_magnifierwidget->getMagnifierRingWidth();
//    int bigcirclex = gpoint.x() - radius;
//    int bigcircley = gpoint.y() - radius;
//    if (bigcircley < 0) {

//        if (scrollBar_Y)
//            scrollBar_Y->setValue(scrollBar_Y->value() + bigcircley);
//    } else if (bigcircley > m_magnifierwidget->height() - radius * 2) {
//        if (scrollBar_Y)
//            scrollBar_Y->setValue(scrollBar_Y->value() + bigcircley - (m_magnifierwidget->height() - radius * 2));
//    }
//    if (bigcirclex < 0) {
//        if (scrollBar_X)
//            scrollBar_X->setValue(scrollBar_X->value() + bigcirclex);
//    } else if (bigcirclex > m_magnifierwidget->width() - radius * 2) {
//        if (scrollBar_X)
//            scrollBar_X->setValue(scrollBar_X->value() + bigcirclex - (m_magnifierwidget->width() - radius * 2));
//    }
//    return false;
//}

//int DocummentDJVU::currentPageNo()
//{
//    if (m_bslidemodel) {
//        return m_slidepageno;
//    }
//    int pagenum = -1;
//    int x_offset = 0;
//    int y_offset = 0;
//    DScrollBar *scrollBar_X = horizontalScrollBar();
//    if (scrollBar_X)
//        x_offset = scrollBar_X->value();
//    DScrollBar *scrollBar_Y = verticalScrollBar();
//    if (scrollBar_Y)
//        y_offset = scrollBar_Y->value();
//    switch (m_viewmode) {
//    case ViewMode_SinglePage:
//        for (int i = 0; i < m_pages.size(); i++) {
//            if (y_offset < m_widgets.at(i)->y() + m_widgets.at(i)->height()) {
//                pagenum = i;
//                break;
//            }
//        }
//        break;
//    case ViewMode_FacingPage:
//        for (int i = 0; i < m_pages.size() / 2; i++) {
//            if (y_offset < m_widgets.at(i)->y() + m_widgets.at(i)->height()) {
//                if (x_offset < m_widgets.at(i)->x() + m_pages.at(i * 2)->x() + m_pages.at(i * 2)->width()) {
//                    pagenum = i * 2;
//                } else {
//                    pagenum = i * 2 + 1;
//                }
//                break;
//            }
//        }
//        if (-1 == pagenum && m_pages.size() % 2) {
//            if (y_offset < m_widgets.at(m_pages.size() / 2)->y() + m_widgets.at(m_pages.size() / 2)->height()) {
//                if (x_offset < m_widgets.at(m_pages.size() / 2)->x() + m_pages.at(m_pages.size() - 1)->x() + m_pages.at(m_pages.size() - 1)->width()) {
//                    pagenum = m_pages.size() - 1;
//                } else {
//                    pagenum = m_pages.size();
//                }
//                break;
//            }
//        }
//        break;
//    default:
//        break;
//    }
//    return pagenum;
//}

//bool DocummentDJVU::pageJump(int pagenum)
//{
//    if (pagenum < 0 || pagenum > m_pages.size())
//        return false;
//    if (m_bslidemodel) {
//        QImage image;
//        double width = m_slidewidget->width(), height = m_slidewidget->height();
//        PageDJVU *ppdf = (PageDJVU *)m_pages.at(pagenum);
//        if (!ppdf->getSlideImage(image, width, height)) {
//            return false;
//        }
//        if (-1 != m_slidepageno) {
//            DLabel *plabel = pslideanimationlabel;
//            pslideanimationlabel = pslidelabel;
//            pslidelabel = plabel;
//        }
//        pslidelabel->setGeometry((m_slidewidget->width() - width) / 2, (m_slidewidget->height() - height) / 2, width, height);
//        QPixmap map = QPixmap::fromImage(image);
//        pslidelabel->setPixmap(map);
//        pslidelabel->show();

//        if (-1 != m_slidepageno) {
//            QPropertyAnimation *animation = new QPropertyAnimation(pslideanimationlabel, "geometry");
//            animation->setDuration(500);

//            QPropertyAnimation *animation1 = new QPropertyAnimation(pslidelabel, "geometry");
//            animation1->setDuration(500);

//            if (m_slidepageno > pagenum) {
//                qDebug() << "pageJump previous pagenum:" << pagenum;
//                animation->setStartValue(pslideanimationlabel->geometry());
//                animation->setEndValue(QRect(m_slidewidget->width(), 10, pslideanimationlabel->width(), pslideanimationlabel->height()));

//                animation1->setStartValue(QRect(-m_slidewidget->width(), 10, pslidelabel->width(), pslidelabel->height()));
//                animation1->setEndValue(pslidelabel->geometry());
//            } else {
//                qDebug() << "pageJump next pagenum:" << pagenum;
//                animation->setStartValue(pslideanimationlabel->geometry());
//                animation->setEndValue(QRect(-m_slidewidget->width(), 10, pslideanimationlabel->width(), pslideanimationlabel->height()));

//                animation1->setStartValue(QRect(m_slidewidget->width(), 10, pslidelabel->width(), pslidelabel->height()));
//                animation1->setEndValue(pslidelabel->geometry());
//            }
//            QParallelAnimationGroup *group = new QParallelAnimationGroup;
//            group->addAnimation(animation);
//            group->addAnimation(animation1);

//            group->start(QAbstractAnimation::DeleteWhenStopped);
//        }

//        m_slidepageno = pagenum;
//    } else {
//        DScrollBar *scrollBar_X = horizontalScrollBar();
//        DScrollBar *scrollBar_Y = verticalScrollBar();
//        switch (m_viewmode) {
//        case ViewMode_SinglePage:
//            qDebug() << "-------pagenum:" << pagenum << " x():" << m_widgets.at(pagenum)->x() << " y():" << m_widgets.at(pagenum)->y();
//            if (scrollBar_X)
//                scrollBar_X->setValue(m_widgets.at(pagenum)->x());
//            if (scrollBar_Y)
//                scrollBar_Y->setValue(m_widgets.at(pagenum)->y());
//            break;
//        case ViewMode_FacingPage:
//            if (scrollBar_X)
//                scrollBar_X->setValue(m_widgets.at(pagenum / 2)->x() + m_pages.at(pagenum)->x());
//            if (scrollBar_Y)
//                scrollBar_Y->setValue(m_widgets.at(pagenum / 2)->y());
//            break;
//        default:
//            break;
//        }
//    }
//    return true;
//}

void DocummentDJVU::docBasicInfo(stFileInfo &info)
{
    info = m_fileinfo;
}

bool DocummentDJVU::annotationClicked(const QPoint &pos, QString &strtext)
{
    QPoint pt(pos);
    int ipage = pointInWhichPage(pt);
    if (ipage < 0) return  false;
    return static_cast<PageDJVU>(m_pages.at(ipage)).annotationClicked(pt, strtext);
}

void DocummentDJVU::title(QString &title)
{
//    title = document->title();
}

//void DocummentDJVU::slot_vScrollBarValueChanged(int value)
//{
//    qDebug() << "slot_vScrollBarValueChanged" << value;
//    if (!donotneedreloaddoc) {
//        int pageno = currentPageNo();
//        if (m_currentpageno != pageno) {
//            m_currentpageno = pageno;
//            emit signal_pageChange(m_currentpageno);
//        }
//        if (m_threadloaddoc.isRunning())
//            m_threadloaddoc.setRestart();
//        else
//            m_threadloaddoc.start();
//    }
//}

//void DocummentDJVU::slot_hScrollBarValueChanged(int value)
//{
//    qDebug() << "slot_hScrollBarValueChanged" << value;
//    if (!donotneedreloaddoc) {
//        int pageno = currentPageNo();
//        if (m_currentpageno != pageno) {
//            m_currentpageno = pageno;
//            emit signal_pageChange(m_currentpageno);
//        }
//        if (m_threadloaddoc.isRunning())
//            m_threadloaddoc.setRestart();
//        else
//            m_threadloaddoc.start();
//    }
//}

//Page::Link *DocummentDJVU::mouseBeOverLink(QPoint point)
//{
//    if (!document) {
//        return nullptr;
//    }
//    QPoint qpoint = point;
//    int pagenum = -1;
//    pagenum = pointInWhichPage(qpoint);
//    qDebug() << "mouseBeOverLink pagenum:" << pagenum;
//    if (-1 != pagenum) {
//        PageDJVU *ppdf = (PageDJVU *)m_pages.at(pagenum);
//        return ppdf ->ifMouseMoveOverLink(qpoint);
//    }
//    return nullptr;
//}

//bool DocummentDJVU::getSelectTextString(QString &st)
//{
//    if (!document) {
//        return false;
//    }
//    st = "";
//    bool bselectexit = false;
//    for (int i = 0; i < m_pages.size(); i++) {
//        PageDJVU *ppdf = (PageDJVU *)m_pages.at(i);
//        QString stpage = "";
//        if (ppdf->getSelectTextString(stpage)) {
//            bselectexit = true;
//            st += stpage;
//        }
//    }
//    return bselectexit;
//}

//bool DocummentDJVU::showSlideModel()
//{
//    if (!document) {
//        return false;
//    }
//    int curpageno = currentPageNo();
//    if (curpageno < 0) {
//        curpageno = 0;
//    }
//    m_bslidemodel = true;
//    this->hide();
//    m_slidewidget->show();
//    if (pageJump(curpageno)) {
//        return true;
//    }
//    m_slidepageno = -1;
//    m_bslidemodel = false;
//    this->show();
//    m_slidewidget->hide();
//    return false;
//}


ddjvu_document_t *DocummentDJVU::getDocument()
{
    return document;
}
ddjvu_context_t *DocummentDJVU::getContext()
{
    return m_context;
}
ddjvu_format_t *DocummentDJVU::getFormat()
{
    return m_format;
}
QHash< QString, int > DocummentDJVU::getPageByName()
{
    return m_pageByName;
}