#include "docummentbase.h"
#include "publicfunc.h"
#include "utils/utils.h"
#include "pagebase.h"
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPainter>
#include <QPoint>
#include <QApplication>
#include <QThreadPool>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <poppler-qt5.h>
#include <qglobal.h>
static QMutex mutexlockloaddata;

DocummentBase::DocummentBase(DocummentBasePrivate *ptr, DWidget *parent): DScrollArea(parent),
    d_ptr(ptr ? ptr : new DocummentBasePrivate(this))
{
    qRegisterMetaType<stSearchRes>("&stSearchRes");
    Q_D(DocummentBase);
    setWidgetResizable(true);
    setFrameShape(QFrame::NoFrame);
    d->qwfather = parent;
    d->m_widget = new DWidget(this);
    setWidget(d->m_widget);
    d->showslidwaittimer = new QTimer(this);
    d->autoplayslidtimer = new QTimer(this);
    d->pblankwidget = new DLabel(this);
    d->pblankwidget->setMouseTracking(true);
    d->pblankwidget->hide();
    d->m_magnifierwidget = new MagnifierWidget(parent);
    d->m_slidewidget = new SlidWidget(parent);
    d->pslidelabel = new DLabel(d->m_slidewidget);
    d->pslideanimationlabel = new DLabel(d->m_slidewidget);
    d->pslidelabel->setGeometry(-200, -200, 100, 100);
    d->pslideanimationlabel->setGeometry(-200, -200, 100, 100);
    delete parent->layout();

    QGridLayout *gridlyout = new QGridLayout(parent);

    parent->setLayout(gridlyout);
    gridlyout->setMargin(0);
    gridlyout->setSpacing(0);
    gridlyout->addWidget(this, 0, 0);
    gridlyout->addWidget(d->m_magnifierwidget, 0, 0);
    gridlyout->addWidget(d->m_slidewidget, 0, 0);
    gridlyout->setMargin(0);

    d->m_slidewidget->hide();
    d->m_magnifierwidget->hide();
    d->m_vboxLayout = new QVBoxLayout;
    d->m_widget->setLayout(d->m_vboxLayout);
    d->m_vboxLayout->setMargin(0);
    d->m_vboxLayout->setSpacing(0);
    d->m_widget->setMouseTracking(true);
    d->pslidelabel->setMouseTracking(true);
    d->pslideanimationlabel->setMouseTracking(true);
    setMouseTracking(true);

    //设置背景黑色
    QPalette pal(d->m_slidewidget->palette());
    pal.setColor(QPalette::Background, Qt::black);
    d->m_slidewidget->setAutoFillBackground(true);
    d->m_slidewidget->setPalette(pal);


    d->m_searchTask = new SearchTask(this);

    connect(this->verticalScrollBar(), &QScrollBar::rangeChanged, this, [ = ](int min, int max) {
        Q_D(DocummentBase);
        DScrollBar *scrollBar_Y = verticalScrollBar();
        if (d->m_currentpageno < 0 || d->m_widgetrects.size() <= 0) {
            return;
        }
        d->donotneedreloaddoc = true;
        switch (d->m_viewmode) {
        case ViewMode_SinglePage:
            if (scrollBar_Y)
                scrollBar_Y->setValue(d->m_widgetrects.at(d->m_currentpageno).y());
            break;
        case ViewMode_FacingPage:
            if (scrollBar_Y)
                scrollBar_Y->setValue(d->m_widgetrects.at(d->m_currentpageno / 2).y());
            break;
        default:
            break;
        }
        d->donotneedreloaddoc = false;
    });
    connect(this->horizontalScrollBar(), &QScrollBar::rangeChanged, this, [ = ](int min, int max) {
        Q_D(DocummentBase);
        DScrollBar *scrollBar_X = horizontalScrollBar();
        if (d->m_currentpageno < 0) {
            return;
        }
        d->donotneedreloaddoc = true;
        switch (d->m_viewmode) {
        case ViewMode_SinglePage:
            if (scrollBar_X)
                scrollBar_X->setValue(d->m_widgetrects.at(d->m_currentpageno).x());
            break;
        case ViewMode_FacingPage:
            if (scrollBar_X)
                scrollBar_X->setValue(d->m_widgetrects.at(d->m_currentpageno / 2).x() + d->m_pages.at(d->m_currentpageno)->x());
            break;
        default:
            break;
        }
        d->donotneedreloaddoc = false;
    });
    connect(d->m_searchTask, SIGNAL(signal_resultReady(stSearchRes)), this, SLOT(slot_searchValueAdd(stSearchRes)));
    connect(d->m_searchTask, SIGNAL(finished()), SLOT(slot_searchover()));

    connect(this->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(slot_vScrollBarValueChanged(int)));
    connect(this->horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(slot_hScrollBarValueChanged(int)));
    connect(d, SIGNAL(signal_docummentLoaded(bool)), this, SLOT(slot_docummentLoaded(bool)));
    connect(this, SIGNAL(signal_loadDocumment(QString)), d, SLOT(loadDocumment(QString)));

    connect(&d->threadloaddata, SIGNAL(signal_dataLoaded(bool)), this, SLOT(slot_dataLoaded(bool)));
    connect(d->showslidwaittimer, SIGNAL(timeout()), this, SLOT(showSlideModelTimerOut()));
    connect(d->autoplayslidtimer, SIGNAL(timeout()), this, SLOT(autoplayslidTimerOut()));
}

DocummentBase::~DocummentBase()
{
    this->hide();
}

QPoint DocummentBase::global2RelativePoint(QPoint globalpoint)
{
    int x_offset = 0;
    int y_offset = 0;
    DScrollBar *scrollBar_X = horizontalScrollBar();
    if (scrollBar_X)
        x_offset = scrollBar_X->value();
    DScrollBar *scrollBar_Y = verticalScrollBar();
    if (scrollBar_Y)
        y_offset = scrollBar_Y->value();
    QPoint qpoint = QPoint(mapFromGlobal(globalpoint).x() + x_offset,
                           mapFromGlobal(globalpoint).y() + y_offset);
    return qpoint;
}

bool DocummentBase::setSelectTextStyle(QColor paintercolor, QColor pencolor, int penwidth)
{
    Q_D(DocummentBase);
    if (!bDocummentExist()) {
        return false;
    }
    for (int i = 0; i < d->m_pages.size(); i++) {
        d->m_pages.at(i)->setSelectTextStyle(paintercolor, pencolor, penwidth);
    }
    return true;
}

bool DocummentBase::mouseSelectText(QPoint start, QPoint stop)
{
    Q_D(DocummentBase);
    if (!bDocummentExist()) {
        return false;
    }
    QPoint qstart = start;
    QPoint qstop = stop;
//    bool mousedirectiondown = true;
//    if (stop.y() < start.y()) {
//        mousedirectiondown = false;
//    }
    // qDebug() << "startpoint:" << start << " stoppoint:" << stop;
    int startpagenum = -1, endpagenum = -1;

    for (int i = 0; i < d->m_widgets.size(); i++) {
        if (qstop.x() > d->m_widgets.at(i)->x() &&
                qstop.x() <
                (d->m_widgets.at(i)->width() + d->m_widgets.at(i)->x()) &&
                qstop.y() > d->m_widgets.at(i)->y() &&
                qstop.y() <
                (d->m_widgets.at(i)->height() + d->m_widgets.at(i)->y())) {
            qstop = QPoint(qstop.x() - d->m_widgets.at(i)->x(), qstop.y() - d->m_widgets.at(i)->y());
            switch (d->m_viewmode) {
            case ViewMode_SinglePage:
                endpagenum = i;
                break;
            case ViewMode_FacingPage:
                if (qstop.x() > d->m_pages.at(2 * i)->x() &&
                        qstop.x() <
                        (d->m_pages.at(2 * i)->width() + d->m_pages.at(2 * i)->x()) &&
                        qstop.y() > d->m_pages.at(2 * i)->y() &&
                        qstop.y() <
                        (d->m_pages.at(2 * i)->height() + d->m_pages.at(2 * i)->y())) {
                    endpagenum = 2 * i;
                } else {
                    endpagenum = 2 * i + 1;
                    if (endpagenum >= d->m_pages.size()) {
                        endpagenum = 2 * i;
                        qstop = QPoint(d->m_pages.at(endpagenum)->width() + d->m_pages.at(endpagenum)->x(),
                                       d->m_pages.at(endpagenum)->height() + d->m_pages.at(endpagenum)->y());
                    }
                }
                break;
            default:
                break;
            }
            break;
        }
    }


    for (int i = 0; i < d->m_widgets.size(); i++) {
        if (qstart.x() > d->m_widgets.at(i)->x() &&
                qstart.x() <
                (d->m_widgets.at(i)->width() + d->m_widgets.at(i)->x()) &&
                qstart.y() > d->m_widgets.at(i)->y() &&
                qstart.y() <
                (d->m_widgets.at(i)->height() + d->m_widgets.at(i)->y())) {
            qstart = QPoint(qstart.x() - d->m_widgets.at(i)->x(), qstart.y() - d->m_widgets.at(i)->y());
            switch (d->m_viewmode) {
            case ViewMode_SinglePage:
                startpagenum = i;
                break;
            case ViewMode_FacingPage:
                if (qstart.x() > d->m_pages.at(2 * i)->x() &&
                        qstart.x() <
                        (d->m_pages.at(2 * i)->width() + d->m_pages.at(2 * i)->x()) &&
                        qstart.y() > d->m_pages.at(2 * i)->y() &&
                        qstart.y() <
                        (d->m_pages.at(2 * i)->height() + d->m_pages.at(2 * i)->y())) {
                    startpagenum = 2 * i;
                } else {
                    startpagenum = 2 * i + 1;
                    if (startpagenum >= d->m_pages.size()) {
                        startpagenum = 2 * i;
                        qstart = QPoint(d->m_pages.at(startpagenum)->width() + d->m_pages.at(startpagenum)->x(),
                                        d->m_pages.at(startpagenum)->height() + d->m_pages.at(startpagenum)->y());
                    }
                }
                break;
            default:
                break;
            }
            break;
        }
    }

    //    qDebug() << "startpagenum:" << startpagenum << " endpagenum:" << endpagenum;
    if (-1 == startpagenum || -1 == endpagenum)
        return false;
    if (startpagenum > endpagenum) {
        int mi = startpagenum;
        startpagenum = endpagenum;
        endpagenum = mi;
        QPoint mp = qstart;
        qstart = qstop;
        qstop = mp;
    }
    //    qDebug() << "startpagenum:" << startpagenum << " endpagenum:" << endpagenum;
    bool re = false;
    for (int i = startpagenum; i < endpagenum + 1; i++) {
        //        PagePdf *ppdf = (PagePdf *)m_pages.at(i);
        QPoint pfirst = QPoint(d->m_pages.at(i)->x(), d->m_pages.at(i)->y());
        QPoint plast = QPoint(d->m_pages.at(i)->width() + d->m_pages.at(i)->x(),
                              d->m_pages.at(i)->height() + d->m_pages.at(i)->y());
        switch (d->m_rotate) {
        case RotateType_90:
            pfirst = QPoint(d->m_pages.at(i)->x() + d->m_pages.at(i)->width(), d->m_pages.at(i)->y());
            plast = QPoint(d->m_pages.at(i)->x(),
                           d->m_pages.at(i)->height() + d->m_pages.at(i)->y());
            break;
        case RotateType_180:
            pfirst = QPoint(d->m_pages.at(i)->x() + d->m_pages.at(i)->width(), d->m_pages.at(i)->y());
            plast = QPoint(d->m_pages.at(i)->x(), d->m_pages.at(i)->y());
            break;
        case RotateType_270:
            pfirst = QPoint(d->m_pages.at(i)->x(), d->m_pages.at(i)->height() + d->m_pages.at(i)->y());
            plast = QPoint(d->m_pages.at(i)->x() + d->m_pages.at(i)->width(),
                           d->m_pages.at(i)->y());
            break;
        default:
            break;
        }
        if (i == startpagenum) {
            if (startpagenum == endpagenum) {
                re = d->m_pages.at(i)->pageTextSelections(qstart, qstop);
            } else {
                re = d->m_pages.at(i)->pageTextSelections(qstart,
                                                          plast);
            }
        } else if (i == endpagenum) {
            re = d->m_pages.at(i)->pageTextSelections(
                     pfirst,
                     qstop);

        } else {
            re = d->m_pages.at(i)->pageTextSelections(pfirst,
                                                      plast);
        }
    }
    return re;
}

void DocummentBase::mouseSelectTextClear()
{
    Q_D(DocummentBase);
    for (int i = 0; i < d->m_pages.size(); i++) {
        d->m_pages.at(i)->clearPageTextSelections();
    }
}

bool DocummentBase::mouseBeOverText(QPoint point)
{
    Q_D(DocummentBase);
    if (!bDocummentExist()) {
        return false;
    }
    QPoint qpoint = point;
    int pagenum = -1;
    pagenum = pointInWhichPage(qpoint);
    if (-1 != pagenum) {
        return d->m_pages.at(pagenum)->ifMouseMoveOverText(qpoint);
    }
    return false;
}

int DocummentBase::pointInWhichPage(QPoint &qpoint)
{
    Q_D(DocummentBase);
    int pagenum = -1;
    for (int i = 0; i < d->m_widgets.size(); i++) {
        if (qpoint.x() > d->m_widgets.at(i)->x() &&
                qpoint.x() <
                (d->m_widgets.at(i)->width() + d->m_widgets.at(i)->x()) &&
                qpoint.y() > d->m_widgets.at(i)->y() &&
                qpoint.y() <
                (d->m_widgets.at(i)->height() + d->m_widgets.at(i)->y())) {
            qpoint = QPoint(qpoint.x() - d->m_widgets.at(i)->x(), qpoint.y() - d->m_widgets.at(i)->y());
            switch (d->m_viewmode) {
            case ViewMode_SinglePage:
                pagenum = i;
                //                qpoint = QPoint(qpoint.x() - m_widgets.at(i)->x(), qpoint.y() - m_widgets.at(i)->y());
                break;
            case ViewMode_FacingPage:
                //                qpoint = QPoint(qpoint.x() - m_widgets.at(i)->x(), qpoint.y() - m_widgets.at(i)->y());
                if (2 * i >= d->m_pages.size()) {
                    return pagenum;
                }
                if (qpoint.x() > d->m_pages.at(2 * i)->x() &&
                        qpoint.x() <
                        (d->m_pages.at(2 * i)->width() + d->m_pages.at(2 * i)->x()) &&
                        qpoint.y() > d->m_pages.at(2 * i)->y() &&
                        qpoint.y() <
                        (d->m_pages.at(2 * i)->height() + d->m_pages.at(2 * i)->y())) {
                    pagenum = 2 * i;
                } else {
                    pagenum = 2 * i + 1;
                    if (pagenum >= d->m_pages.size())
                        return -1;
                }
                break;
            default:
                break;
            }
            break;
        }
    }
    return pagenum;
}

void DocummentBase::slot_MagnifierPixmapCacheLoaded(int pageno)
{
    Q_D(DocummentBase);
    if (pageno == d->m_lastmagnifierpagenum) {
        showMagnifier(d->m_magnifierpoint);
    }
}

void DocummentBase::slot_searchValueAdd(stSearchRes res)
{
    Q_D(DocummentBase);
    if (d->m_bsearchfirst) {
        d->m_bsearchfirst = false;
        d->m_findcurpage = res.ipage;
        d->m_cursearch = 1;
        d->m_pagecountsearch.insert(res.ipage, res.listtext.size());
        emit signal_searchRes(res);
        findNext();
    } else {
        d->m_pagecountsearch.insert(res.ipage, res.listtext.size());
        emit signal_searchRes(res);
    }
}

void DocummentBase::slot_searchover()
{
    Q_D(DocummentBase);
    d->m_bsearchfirst = true;
    emit signal_searchover();
}

bool DocummentBase::showMagnifier(QPoint point)
{
    Q_D(DocummentBase);
    if (!bDocummentExist() || !d->m_magnifierwidget) {
        return false;
    }
    return  showMagnifierTest(point);
    QPoint qpoint = point;
    d->m_magnifierpoint = point;
    int pagenum = -1;
    int x_offset = 0;
    int y_offset = 0;
    DScrollBar *scrollBar_X = horizontalScrollBar();
    if (scrollBar_X)
        x_offset = scrollBar_X->value();
    DScrollBar *scrollBar_Y = verticalScrollBar();
    if (scrollBar_Y)
        y_offset = scrollBar_Y->value();
    QPoint gpoint = d->m_magnifierwidget->mapFromGlobal(mapToGlobal(QPoint(point.x() - x_offset, point.y() - y_offset)));
    pagenum = pointInWhichPage(qpoint);
    if (-1 != pagenum) {
        if (pagenum != d->m_lastmagnifierpagenum && -1 != d->m_lastmagnifierpagenum) {
            if (pagenum > d->m_lastmagnifierpagenum && d->m_lastmagnifierpagenum - 3 > 0) {
                PageBase *ppage = d->m_pages.at(d->m_lastmagnifierpagenum - 3);
                ppage->clearMagnifierPixmap();
                if (pagenum - d->m_lastmagnifierpagenum > 1) {
                    ppage = d->m_pages.at(d->m_lastmagnifierpagenum - 2);
                    ppage->clearMagnifierPixmap();
                }
            } else if (pagenum < d->m_lastmagnifierpagenum && d->m_lastmagnifierpagenum + 3 < d->m_pages.size()) {
                PageBase *ppage = d->m_pages.at(d->m_lastmagnifierpagenum + 3);
                ppage->clearMagnifierPixmap();
                if (d->m_lastmagnifierpagenum - pagenum > 1) {
                    ppage = d->m_pages.at(d->m_lastmagnifierpagenum + 2);
                    ppage->clearMagnifierPixmap();
                }
            }
            PageBase *ppage = nullptr;
            int ipageno = pagenum;
            double imagewidth = d->m_pages.at(pagenum)->width() * d->m_magnifierwidget->getMagnifierScale() * devicePixelRatioF();
            double imageheight = d->m_pages.at(pagenum)->height() * d->m_magnifierwidget->getMagnifierScale() * devicePixelRatioF();
            if (ipageno >= 0 && ipageno < d->m_pages.size()) {
                ppage = d->m_pages.at(ipageno);
                ppage->loadMagnifierCacheThreadStart(imagewidth, imageheight);
            }
            ipageno = pagenum + 1;
            if (ipageno >= 0 && ipageno < d->m_pages.size()) {
                ppage = d->m_pages.at(ipageno);
                ppage->loadMagnifierCacheThreadStart(imagewidth, imageheight);
            }
            ipageno = pagenum - 1;
            if (ipageno >= 0 && ipageno < d->m_pages.size()) {
                ppage = d->m_pages.at(ipageno);
                ppage->loadMagnifierCacheThreadStart(imagewidth, imageheight);
            }
            ipageno = pagenum + 2;
            if (ipageno >= 0 && ipageno < d->m_pages.size()) {
                ppage = d->m_pages.at(ipageno);
                ppage->loadMagnifierCacheThreadStart(imagewidth, imageheight);
            }
            ipageno = pagenum - 2;
            if (ipageno >= 0 && ipageno < d->m_pages.size()) {
                ppage = d->m_pages.at(ipageno);
                ppage->loadMagnifierCacheThreadStart(imagewidth, imageheight);
            }
            ipageno = pagenum + 3;
            if (ipageno >= 0 && ipageno < d->m_pages.size()) {
                ppage = d->m_pages.at(ipageno);
                ppage->loadMagnifierCacheThreadStart(imagewidth, imageheight);
            }
            ipageno = pagenum - 3;
            if (ipageno >= 0 && ipageno < d->m_pages.size()) {
                ppage = d->m_pages.at(ipageno);
                ppage->loadMagnifierCacheThreadStart(imagewidth, imageheight);
            }
        }
        d->m_lastmagnifierpagenum = pagenum;
        PageBase *ppage = d->m_pages.at(pagenum);
        QPixmap pixmap;
        d->m_magnifierpage = pagenum;

        double curwidth = d->m_scale * d->m_imagewidth;
        double curheight = d->m_scale * d->m_imageheight;
        double topspace = (d->m_widgets.at(pagenum)->height() - curheight) / 2.0;
        double left = 0.0;
        int imaginfierradius = d->m_magnifierwidget->getMagnifierRadius();
        if (d->m_viewmode == ViewMode_SinglePage) {
            left = (d->m_widgets.at(pagenum)->width() - curwidth) / 2.0;
            bool bcondition = (qpoint.x() < left - imaginfierradius || qpoint.x() > curwidth + left + imaginfierradius) && left > imaginfierradius;
            bool bcondition1 = (qpoint.x() < imaginfierradius - 10  || qpoint.x() > curwidth + left + 10) && left < imaginfierradius;
            if (bcondition || bcondition1) {
                QPixmap pix(imaginfierradius * 2, imaginfierradius * 2);
                pix.fill(Qt::transparent);
                d->m_magnifierwidget->setPixmap(pix);
                d->m_magnifierwidget->setPoint(gpoint);
                d->m_magnifierwidget->show();
                d->m_magnifierwidget->update();
            } else {
                if (ppage ->getMagnifierPixmap(pixmap, qpoint, d->m_magnifierwidget->getMagnifierRadius(), ppage->width()*d->m_magnifierwidget->getMagnifierScale(), ppage->height()*d->m_magnifierwidget->getMagnifierScale())) {
                    if (pixmap.size().width() < d->m_magnifierwidget->getMagnifierRadius() * 2) {
                        left = (d->m_widgets.at(pagenum)->width() - curwidth) / 2.0;
                        int ileft = qpoint.x() - left;
                        int iright = left + curwidth - qpoint.x();
                        int iwidth = d->m_magnifierwidget->getMagnifierRadius() * 2;
                        QPixmap pixres(QSize(iwidth, iwidth));
                        pixres.fill(Qt::transparent);

                        QPainter painter(&pixres);
                        {
                            //在右边
                            if (iright < 0 || ileft > iright) {
                                if (iright < 0) {
                                    pixmap = pixmap.copy(qpoint.x() - curwidth - left, 0, imaginfierradius - (qpoint.x() - curwidth - left), imaginfierradius * 2);
                                }
                                painter.drawPixmap(pixmap.rect(), pixmap);

                            } else {
                                if (qpoint.x() - left < imaginfierradius && ileft < 0) {
                                    pixmap = pixmap.copy(/*left - qpoint.x()*/0, 0, imaginfierradius - (left - qpoint.x()), imaginfierradius * 2);
                                } /*else if (qpoint.x() - left > 0 && qpoint.x() - left < 2 * imaginfierradius) {
                                    pixmap = pixmap.copy(0, 0, imaginfierradius - (left - qpoint.x()), imaginfierradius * 2);
                                }*/
                                int iorgwidth = pixmap.rect().width();
                                int iorgheight = pixmap.rect().height();
                                QRect rect(iwidth - iorgwidth, iwidth - iorgheight, iorgwidth, iorgheight);
                                painter.drawPixmap(rect, pixmap);
                            }
                        }

                        pixmap = pixres;
                    }
                    qDebug() << __FUNCTION__ << "############" << pixmap.size() << d->m_magnifierwidget->getMagnifierRadius();
                    d->m_magnifierwidget->setPixmap(pixmap);
                    d->m_magnifierwidget->setPoint(gpoint);
                    d->m_magnifierwidget->setShowState(true);
                    d->m_magnifierwidget->show();
                    d->m_magnifierwidget->update();
                }
            }
        } else {
            double curitemwidth = d->m_widgets.at(pagenum)->width();
            left = (curitemwidth - curwidth * 2.0) / 3.0;
            if ((qpoint.x() < left - d->m_magnifierwidget->getMagnifierRadius() ||
                    (qpoint.x() > curwidth + left + d->m_magnifierwidget->getMagnifierRadius() && qpoint.x() < curwidth + 2.0 * left) ||
                    qpoint.x() >  curwidth * 2.0 + left * 2 + d->m_magnifierwidget->getMagnifierRadius()) && qpoint.x() > 0 && qpoint.x() < d->m_widgets.at(pagenum)->width()) {
                QPixmap pix(d->m_magnifierwidget->getMagnifierRadius() * 2, d->m_magnifierwidget->getMagnifierRadius() * 2);
                pix.fill(Qt::transparent);
                d->m_magnifierwidget->setPixmap(pix);
                d->m_magnifierwidget->setPoint(gpoint);
                d->m_magnifierwidget->show();
                d->m_magnifierwidget->update();
            } else {
                if (ppage ->getMagnifierPixmap(pixmap, qpoint, d->m_magnifierwidget->getMagnifierRadius(), ppage->width()*d->m_magnifierwidget->getMagnifierScale(), ppage->height()*d->m_magnifierwidget->getMagnifierScale())) {
                    d->m_magnifierwidget->setPixmap(pixmap);
                    d->m_magnifierwidget->setPoint(gpoint);
                    d->m_magnifierwidget->setShowState(true);
                    d->m_magnifierwidget->show();
                    d->m_magnifierwidget->update();
                }
            }
        }
    } else {
        QPixmap pix(d->m_magnifierwidget->getMagnifierRadius() * 2, d->m_magnifierwidget->getMagnifierRadius() * 2);
        pix.fill(Qt::transparent);
        d->m_magnifierwidget->setPixmap(pix);
        d->m_magnifierwidget->setPoint(gpoint);
        d->m_magnifierwidget->show();
        d->m_magnifierwidget->update();
    }
    if (!d->m_magnifierwidget->showState())
        return false;
    int radius = d->m_magnifierwidget->getMagnifierRadius() - d->m_magnifierwidget->getMagnifierRingWidth();
    int bigcirclex = gpoint.x() - radius;
    int bigcircley = gpoint.y() - radius;
    if (bigcircley < 0) {

        if (scrollBar_Y)
            scrollBar_Y->setValue(scrollBar_Y->value() + bigcircley);
    } else if (bigcircley > d->m_magnifierwidget->height() - radius * 2) {
        if (scrollBar_Y)
            scrollBar_Y->setValue(scrollBar_Y->value() + bigcircley - (d->m_magnifierwidget->height() - radius * 2));
    }
    if (bigcirclex < 0) {
        if (scrollBar_X)
            scrollBar_X->setValue(scrollBar_X->value() + bigcirclex);
    } else if (bigcirclex > d->m_magnifierwidget->width() - radius * 2) {
        if (scrollBar_X)
            scrollBar_X->setValue(scrollBar_X->value() + bigcirclex - (d->m_magnifierwidget->width() - radius * 2));
    }
    return true;
}

bool DocummentBase::showMagnifierTest(QPoint point)
{
    Q_D(DocummentBase);
    if (!bDocummentExist() || !d->m_magnifierwidget) {
        return false;
    }
    QPoint qpoint = point;
    //  d->m_magnifierpoint = point;
    int pagenum = -1;
    int x_offset = 0;
    int y_offset = 0;
    DScrollBar *scrollBar_X = horizontalScrollBar();
    if (scrollBar_X)
        x_offset = scrollBar_X->value();
    DScrollBar *scrollBar_Y = verticalScrollBar();
    if (scrollBar_Y)
        y_offset = scrollBar_Y->value();
    QPoint gpoint = d->m_magnifierwidget->mapFromGlobal(mapToGlobal(QPoint(point.x() - x_offset, point.y() - y_offset)));
    pagenum = pointInWhichPage(qpoint);
    if (-1 != pagenum) {
        if (getMaxZoomratio() < 5) {
            d->m_magnifierwidget->setMagnifierScale(1.5);
        }
        d->m_magnifierwidget->showrectimage(d->m_pages.at(pagenum), d->m_scale, d->m_rotate, qpoint);
        d->m_magnifierwidget->setPoint(gpoint);
        d->m_magnifierwidget->show();
        d->m_magnifierwidget->update();

//        QImage resimage;
//        int imagewidth = d->m_magnifierwidget->getMagnifierRadius() * 2;
//        d->m_pages.at(pagenum)->getrectimage(resimage, imagewidth, d->m_scale * d->m_magnifierwidget->getMagnifierScale() * devicePixelRatioF(), d->m_magnifierwidget->getMagnifierScale(), qpoint);

//        QPixmap pix;
//        pix = pix.fromImage(resimage);
//        QMatrix leftmatrix;
//        switch (d->m_rotate) {
//        case RotateType_90:
//            leftmatrix.rotate(90);
//            break;
//        case RotateType_180:
//            leftmatrix.rotate(180);
//            break;
//        case RotateType_270:
//            leftmatrix.rotate(270);
//            break;
//        default:
//            break;
//        }
//        pix = pix.transformed(leftmatrix, Qt::SmoothTransformation);
//        pix.setDevicePixelRatio(devicePixelRatioF());
//        d->m_magnifierwidget->setPixmap(pix);
//        d->m_magnifierwidget->setPoint(gpoint);
//        d->m_magnifierwidget->setShowState(true);
//        d->m_magnifierwidget->show();
//        d->m_magnifierwidget->update();
    }

    if (!d->m_magnifierwidget->showState())
        return false;
    int radius = d->m_magnifierwidget->getMagnifierRadius() - d->m_magnifierwidget->getMagnifierRingWidth();
    int bigcirclex = gpoint.x() - radius;
    int bigcircley = gpoint.y() - radius;
    if (bigcircley < 0) {

        if (scrollBar_Y)
            scrollBar_Y->setValue(scrollBar_Y->value() + bigcircley);
    } else if (bigcircley > d->m_magnifierwidget->height() - radius * 2) {
        if (scrollBar_Y)
            scrollBar_Y->setValue(scrollBar_Y->value() + bigcircley - (d->m_magnifierwidget->height() - radius * 2));
    }
    if (bigcirclex < 0) {
        if (scrollBar_X)
            scrollBar_X->setValue(scrollBar_X->value() + bigcirclex);
    } else if (bigcirclex > d->m_magnifierwidget->width() - radius * 2) {
        if (scrollBar_X)
            scrollBar_X->setValue(scrollBar_X->value() + bigcirclex - (d->m_magnifierwidget->width() - radius * 2));
    }
    return true;
}

void DocummentBase::resizeEvent(QResizeEvent *e)
{
    DScrollArea::resizeEvent(e);
    loadPages(); //主要目的是解决文档刚加载的时候qwfather获取的尺寸不对，导致界面少显示页面的问题
}

bool DocummentBase::pageJump(int pagenum)
{
    Q_D(DocummentBase);
    if (pagenum < 0 || pagenum >= d->m_pages.size())
        return false;
    if (d->m_bslidemodel) {
        QImage image;
        double width = d->m_slidewidget->width(), height = d->m_slidewidget->height();
        if (!d->m_pages.at(pagenum)->getSlideImage(image, width, height)) {
            return false;
        }
        if (-1 != d->m_slidepageno) {
            DLabel *plabel = d->pslideanimationlabel;
            d->pslideanimationlabel = d->pslidelabel;
            d->pslidelabel = plabel;
        }
        d->pslidelabel->setGeometry((d->m_slidewidget->width() - width) / 2, (d->m_slidewidget->height() - height) / 2, width, height);
        QPixmap map = QPixmap::fromImage(image);
        d->pslidelabel->setPixmap(map);
        d->pslideanimationlabel->lower();
        d->pslidelabel->show();

        if (-1 != d->m_slidepageno) {
            if (!d->animationfirst) {
                delete d->animationfirst;
                d->animationfirst = nullptr;
            }
            d->animationfirst = new QPropertyAnimation(d->pslideanimationlabel, "geometry", this);
            d->animationfirst->setDuration(500);

            if (!d->animationsecond) {
                delete d->animationsecond;
                d->animationsecond = nullptr;
            }
            d->animationsecond = new QPropertyAnimation(d->pslidelabel, "geometry", this);
            d->animationsecond->setDuration(500);

            QRect rectslideanimationlabel = d->pslideanimationlabel->geometry();
            QRect rectslidelabel = d->pslidelabel->geometry();
            if (d->m_slidepageno > pagenum) {
                d->animationfirst->setStartValue(rectslideanimationlabel);
                d->animationfirst->setEndValue(QRect(d->m_slidewidget->width(), 10, d->pslideanimationlabel->width(), d->pslideanimationlabel->height()));

                d->animationsecond->setStartValue(QRect(-d->m_slidewidget->width(), 10, d->pslidelabel->width(), d->pslidelabel->height()));
                d->animationsecond->setEndValue(rectslidelabel);
            } else {
                d->animationfirst->setStartValue(rectslideanimationlabel);
                d->animationfirst->setEndValue(QRect(-d->m_slidewidget->width(), 10, d->pslideanimationlabel->width(), d->pslideanimationlabel->height()));

                d->animationsecond->setStartValue(QRect(d->m_slidewidget->width(), 10, d->pslidelabel->width(), d->pslidelabel->height()));
                d->animationsecond->setEndValue(rectslidelabel);
            }
            if (!d->animationgroup) {
                delete d->animationgroup;
                d->animationgroup = nullptr;
            }
            d->animationgroup = new QParallelAnimationGroup(this);
            d->animationgroup->addAnimation(d->animationfirst);
            d->animationgroup->addAnimation(d->animationsecond);
            d->animationgroup->start();
        }

        d->m_slidepageno = pagenum;
    } else {
        DScrollBar *scrollBar_X = horizontalScrollBar();
        DScrollBar *scrollBar_Y = verticalScrollBar();
        switch (d->m_viewmode) {
        case ViewMode_SinglePage:
            if (scrollBar_X)
                scrollBar_X->setValue(d->m_widgetrects.at(pagenum).x());
            if (scrollBar_Y)
                scrollBar_Y->setValue(d->m_widgetrects.at(pagenum).y());
            break;
        case ViewMode_FacingPage:
            if (scrollBar_X)
                scrollBar_X->setValue(d->m_widgetrects.at(pagenum / 2).x() + d->m_pages.at(pagenum)->x());
            if (scrollBar_Y)
                scrollBar_Y->setValue(d->m_widgetrects.at(pagenum / 2).y());
            break;
        default:
            break;
        }

        if (d->m_currentpageno != pagenum) {
            d->m_currentpageno = pagenum;
            emit signal_pageChange(d->m_currentpageno);
        }
    }
    return true;
}

void DocummentBase::setScaleRotateViewModeAndShow(double scale, RotateType_EM rotate, ViewMode_EM viewmode)
{
    Q_D(DocummentBase);

    double dscale = scale;

    if (viewmode != d->m_viewmode && (scale - d->m_scale < EPSINON && scale - d->m_scale > -EPSINON))
        dscale = 0;

    d->m_viewmode = viewmode;

    scaleAndShow(dscale, rotate);
}

void DocummentBase::scaleAndShow(double scale, RotateType_EM rotate)
{
    Q_D(DocummentBase);

    if (d->m_pages.size() < 1) {
        return;
    }

    if (scale - d->m_scale < EPSINON && scale - d->m_scale > -EPSINON && (rotate == RotateType_Normal || d->m_rotate == rotate)) {
        return;
    }
    if (scale > 0)
        d->m_scale = scale;

    if (rotate != RotateType_Normal)
        d->m_rotate = rotate;

    d->donotneedreloaddoc = true;

    for (int i = 0; i < d->m_pages.size(); i++) {
        d->m_pages.at(i)->setScaleAndRotate(d->m_scale, d->m_rotate);
    }

    setViewModeAndShow(d->m_viewmode);
}

int DocummentBase::getCurrentPageNo()
{
    Q_D(DocummentBase);

    if (d->m_bslidemodel) {
        return d->m_slidepageno;
    }

    if (d->m_currentpageno >= 0)
        return d->m_currentpageno;

    return currentPageNo();
}

int DocummentBase::currentPageNo()
{
    Q_D(DocummentBase);
    int pagenum = -1;
    int x_offset = 0;
    int y_offset = 0;
    DScrollBar *scrollBar_X = horizontalScrollBar();
    if (scrollBar_X)
        x_offset = scrollBar_X->value();
    DScrollBar *scrollBar_Y = verticalScrollBar();
    if (scrollBar_Y)
        y_offset = scrollBar_Y->value();
    switch (d->m_viewmode) {
    case ViewMode_SinglePage:
        for (int i = 0; i < d->m_widgets.size(); i++) {
            if (y_offset < d->m_widgetrects.at(i).y() + d->m_widgetrects.at(i).height()) {
                pagenum = i;
                break;
            }
        }
        break;
    case ViewMode_FacingPage:
        for (int i = 0; i < d->m_widgets.size() / 2; i++) {
            if (y_offset < d->m_widgetrects.at(i).y() + d->m_widgetrects.at(i).height()) {
                if (x_offset < d->m_widgetrects.at(i).x() + d->m_pages.at(i * 2)->x() + d->m_pages.at(i * 2)->width()) {
                    pagenum = i * 2;
                } else {
                    pagenum = i * 2 + 1;
                }
                break;
            }
        }
        if (-1 == pagenum && d->m_widgets.size() % 2) {
            if (y_offset < d->m_widgetrects.at(d->m_pages.size() / 2).y() + d->m_widgetrects.at(d->m_pages.size() / 2).height()) {
                if (x_offset < d->m_widgetrects.at(d->m_pages.size() / 2).x() + d->m_pages.at(d->m_pages.size() - 1)->x() + d->m_pages.at(d->m_pages.size() - 1)->width()) {
                    pagenum = d->m_pages.size() - 1;
                } else {
                    pagenum = d->m_pages.size();
                }
                break;
            }
        }
        break;
    default:
        break;
    }
    return pagenum;
}

void DocummentBase::showSinglePage()
{
    Q_D(DocummentBase);
    d->m_widgetrects.clear();
    d->pblankwidget->hide();
    int rex = d->m_vboxLayout->margin(), rey = d->m_vboxLayout->margin();
    for (int i = 0; i < d->m_pages.size(); i++) {
        d->m_widgets.at(i)->layout()->addWidget(d->m_pages.at(i));
        d->m_widgets.at(i)->show();

        d->m_widgetrects.append(QRect(rex, rey, d->m_widgets.at(i)->layout()->margin() * 2 + d->m_pages.at(i)->width(), d->m_widgets.at(i)->layout()->margin() * 2 + d->m_pages.at(i)->height()));
        d->m_widgets.at(i)->setGeometry(rex, rey, d->m_widgets.at(i)->layout()->margin() * 2 + d->m_pages.at(i)->width(), d->m_widgets.at(i)->layout()->margin() * 2 + d->m_pages.at(i)->height());
        rey += d->m_widgets.at(i)->layout()->margin() * 2 + d->m_pages.at(i)->height() + d->m_vboxLayout->spacing();
    }
}
void DocummentBase::showFacingPage()
{
    Q_D(DocummentBase);
    d->m_widgetrects.clear();
    int rex = d->m_vboxLayout->margin(), rey = d->m_vboxLayout->margin();
    for (int i = 0; i < d->m_widgets.size(); i++) {
        d->m_widgets.at(i)->hide();
    }
    d->pblankwidget->hide();
    for (int i = 0; i < d->m_pages.size() / 2; i++) {
        d->m_widgets.at(i)->layout()->addWidget(d->m_pages.at(i * 2));
        d->m_widgets.at(i)->layout()->addWidget(d->m_pages.at(i * 2 + 1));
        d->m_widgets.at(i)->show();
        int reheight = 0;
        if (d->m_pages.at(i * 2)->height() < d->m_pages.at(i * 2 + 1)->height()) {
            reheight = d->m_pages.at(i * 2 + 1)->height();
        } else {
            reheight = d->m_pages.at(i * 2)->height();
        }
        d->m_widgets.at(i)->setGeometry(rex, rey, d->m_widgets.at(i)->layout()->margin() * 2 + d->m_widgets.at(i)->layout()->spacing() + d->m_pages.at(i * 2)->width() + d->m_pages.at(i * 2 + 1)->width(), d->m_widgets.at(i)->layout()->margin() * 2 + reheight);
        d->m_widgetrects.append(QRect(rex, rey, d->m_widgets.at(i)->layout()->margin() * 2 + d->m_widgets.at(i)->layout()->spacing() + d->m_pages.at(i * 2)->width() + d->m_pages.at(i * 2 + 1)->width(), d->m_widgets.at(i)->layout()->margin() * 2 + reheight));
        rey += d->m_widgets.at(i)->layout()->margin() * 2 + reheight + d->m_vboxLayout->spacing();

    }
    if (d->m_pages.size() % 2) {
        if (d->m_pages.size() > 0) {
            d->pblankwidget->setMaximumSize(QSize(d->m_pages.at(d->m_pages.size() - 1)->size()));
            d->pblankwidget->setMinimumSize(QSize(d->m_pages.at(d->m_pages.size() - 1)->size()));
        }
        d->pblankwidget->show();
        d->m_widgets.at(d->m_pages.size() / 2)->layout()->addWidget(d->m_pages.at(d->m_pages.size() - 1));
        d->m_widgets.at(d->m_pages.size() / 2)->layout()->addWidget(d->pblankwidget);
        d->m_widgets.at(d->m_pages.size() / 2)->show();
        int reheight = d->m_pages.at(d->m_pages.size() - 1)->height();
        d->m_widgetrects.append(QRect(rex, rey, d->m_widgets.at(d->m_widgets.size() / 2)->layout()->margin() * 2 + d->m_widgets.at(d->m_widgets.size() / 2)->layout()->spacing() + d->m_pages.at(d->m_pages.size() - 1)->width() * 2, d->m_widgets.at(d->m_widgets.size() / 2)->layout()->margin() * 2 + reheight));
        d->m_widgets.at(d->m_widgets.size() / 2)->setGeometry(rex, rey, d->m_widgets.at(d->m_widgets.size() / 2)->layout()->margin() * 2 + d->m_widgets.at(d->m_widgets.size() / 2)->layout()->spacing() + d->m_pages.at(d->m_pages.size() - 1)->width() * 2, d->m_widgets.at(d->m_widgets.size() / 2)->layout()->margin() * 2 + reheight);
    }
}

Page::Link *DocummentBase::mouseBeOverLink(QPoint point)
{
    Q_D(DocummentBase);
    if (!bDocummentExist()) {
        return nullptr;
    }
    QPoint qpoint = point;
    int pagenum = -1;
    pagenum = pointInWhichPage(qpoint);
    if (-1 != pagenum) {
        return d->m_pages.at(pagenum)->ifMouseMoveOverLink(qpoint);
    }
    return nullptr;
}

void DocummentBase::slot_vScrollBarValueChanged(int value)
{
    Q_D(DocummentBase);
    if (!d->donotneedreloaddoc) {
        int pageno = currentPageNo();
        if (d->m_currentpageno != pageno) {
            d->m_currentpageno = pageno;
            emit signal_pageChange(d->m_currentpageno);
        }
        loadPages();
    }
}

void DocummentBase::slot_hScrollBarValueChanged(int value)
{
    Q_D(DocummentBase);
    if (!d->donotneedreloaddoc) {
        int pageno = currentPageNo();
        if (d->m_currentpageno != pageno) {
            d->m_currentpageno = pageno;
            emit signal_pageChange(d->m_currentpageno);
        }
        loadPages();
    }
}

bool DocummentBase::getSelectTextString(QString &st)
{
    Q_D(DocummentBase);
    if (!bDocummentExist()) {
        return false;
    }
    st = "";
    bool bselectexit = false;
    for (int i = 0; i < d->m_pages.size(); i++) {
        QString stpage = "";
        if (d->m_pages.at(i)->getSelectTextString(stpage)) {
            bselectexit = true;
            st += stpage;
        }
    }
    return bselectexit;
}

bool DocummentBase::getSelectTextString(QString &st, int &page)
{
    Q_D(DocummentBase);
    if (!bDocummentExist()) {
        return false;
    }
    st = "";
    bool bselectexit = false;
    for (int i = 0; i < d->m_pages.size(); i++) {
        QString stpage = "";
        if (d->m_pages.at(i)->getSelectTextString(stpage)) {
            page = i;
            bselectexit = true;
            st += stpage;
        }
    }
    return bselectexit;
}

void DocummentBase::autoplayslidTimerOut()
{
    Q_D(DocummentBase);
    if (d->m_bslidemodel) {
        if (d->m_bautoplayslidreset && getCurrentPageNo() + 1 >= d->m_pages.size()) {
            pageJump(0);
            d->m_bautoplayslidreset = false;
        } else {
            pageJump(getCurrentPageNo() + 1);
            if (getCurrentPageNo() + 1 >= d->m_pages.size()) {
                d->autoplayslidtimer->stop();
                d->bautoplayslide = false;
                emit signal_autoplaytoend();
                d->m_bautoplayslidreset = true;
            }
        }
    } else {
        d->autoplayslidtimer->stop();
    }
}

void DocummentBase::showSlideModelTimerOut()
{
    Q_D(DocummentBase);
    d->showslidwaittimer->stop();
    int curpageno = getCurrentPageNo();
    if (curpageno < 0) {
        curpageno = 0;
    }
    d->m_bslidemodel = true;
    if (pageJump(curpageno)) {
        d->autoplayslidtimer->stop();
        if (d->bautoplayslide)
            d->autoplayslidtimer->start(d->m_autoplayslidmsec);
        return;
    }
    d->m_slidepageno = -1;
    d->m_bslidemodel = false;
    this->show();
    d->m_slidewidget->hide();
}

bool DocummentBase::showSlideModel()
{
    Q_D(DocummentBase);
    if (!bDocummentExist()) {
        return false;
    }
    this->hide();
    d->m_slidewidget->show();
    d->showslidwaittimer->start(100);
    return true;
}

void DocummentBase::cacularValueXY(int &xvalue, int &yvalue, int curpage, bool bsearch, QRectF rectsource)
{
    Q_D(DocummentBase);
    double curwidth = d->m_scale * d->m_imagewidth;
    double curheight = d->m_scale * d->m_imageheight;
    PageBase *pagebase = d->m_pages.at(curpage);
    QRectF rectorg;
    if (bsearch) {
        pagebase->setCurSearchShow(true);
        rectorg = pagebase->setCurHightLight(d->m_cursearch);
    } else {
        rectorg = rectsource;
    }

    rectorg = pagebase->translateRect(rectorg, d->m_scale, d->m_rotate);//translateRect执行后返回rect为缩放后的值
    if (d->m_viewmode == ViewMode_SinglePage) {
        switch (d->m_rotate) {
        case RotateType_0:
        case RotateType_180: {
            double topspace = (d->m_widgets.at(curpage)->height() - curheight) / 2;
            double leftspace = (d->m_widgets.at(curpage)->width() - curwidth) / 2;
            int widgetheight = frameRect().height();
            yvalue = d->m_widgets.at(curpage)->y() + topspace + rectorg.y() - widgetheight / 2;
            //横向有缩放
            if (frameRect().width() < curwidth) {
                int iwidth = rectorg.x() + leftspace;
                if (iwidth > (frameRect().width() / 2)) {
                    xvalue = iwidth - frameRect().width() / 2;
                } else {
                    xvalue = iwidth / 2;
                }
            }
        }
        break;
        case RotateType_90:
        case RotateType_270: {
            double topspace = (d->m_widgets.at(curpage)->height() - curwidth) / 2;
            double leftspace = (d->m_widgets.at(curpage)->width() - curheight) / 2;
            int widgetheight = frameRect().height();
            yvalue = d->m_widgets.at(curpage)->y() + topspace + rectorg.y() - widgetheight / 2;
            //横向有缩放
            if (frameRect().width() < curheight) {
                int iwidth = rectorg.x() + leftspace;
                if (iwidth > (frameRect().width() / 2)) {
                    xvalue = iwidth - frameRect().width() / 2;
                } else {
                    xvalue = iwidth / 2;
                }
            }
        }
        break;
        }
    } else if (d->m_viewmode == ViewMode_FacingPage) {
        int index = curpage / 2;
        int ispace = d->m_widgets.at(index)->layout()->spacing();
        switch (d->m_rotate) {
        case RotateType_0:
        case RotateType_180: {
            double topspace = (d->m_widgets.at(index)->height() - curheight) / 2;
            int widgetheight = frameRect().height();
            yvalue = d->m_widgets.at(index)->y() + topspace + rectorg.y() - widgetheight / 2;

            if (0 == curpage % 2) {
                //左侧
                double leftspace = (d->m_widgets.at(index)->width() - ispace) / 2 - curwidth;
                //横向有缩放
                if (frameRect().width() < 2 * curwidth) {
                    int iwidth = rectorg.x() + leftspace;
                    xvalue = iwidth - frameRect().width() / 2;
                }
            } else {
                //右侧
                double leftspace = d->m_widgets.at(index)->width() / 2 + ispace / 2;
                if (frameRect().width() < 2 * curwidth) {
                    int iwidth = rectorg.x() + leftspace;
                    xvalue = iwidth - frameRect().width() / 2;
                }
            }
        }
        break;
        case RotateType_90:
        case RotateType_270: {
            double topspace = (d->m_widgets.at(index)->height() - curwidth) / 2;
            int widgetheight = frameRect().height();
            yvalue = d->m_widgets.at(index)->y() + topspace + rectorg.y() - widgetheight / 2;
            if (0 == curpage % 2) {
                //左侧
                double leftspace = (d->m_widgets.at(index)->width() - ispace) / 2 - curheight;
                //横向有缩放
                if (frameRect().width() < 2 * curheight) {
                    int iwidth = rectorg.x() + leftspace;
                    xvalue = iwidth - frameRect().width() / 2;
                }
            } else {
                //右侧
                double leftspace = d->m_widgets.at(index)->width() / 2 + ispace / 2;
                if (frameRect().width() < 2 * curheight) {
                    int iwidth = rectorg.x() + leftspace;
                    xvalue = iwidth - frameRect().width() / 2;
                }
            }

        }
        break;
        }
    }
}

bool DocummentBase::setViewModeAndShow(ViewMode_EM viewmode)
{
    Q_D(DocummentBase);

    int currpageno = d->m_currentpageno;

    d->m_viewmode = viewmode;

    d->donotneedreloaddoc = true;

    switch (d->m_viewmode) {
    case ViewMode_SinglePage:
        showSinglePage();
        break;
    case ViewMode_FacingPage:
        showFacingPage();
        break;
    default:
        return false;
        break;
    }

    d->m_vboxLayout->update();

    pageJump(currpageno);

    loadPages();

    d->donotneedreloaddoc = false;

    return true;;
}

bool DocummentBase::loadPages()
{
    Q_D(DocummentBase);

    if (!bDocummentExist())
        return false;

    int firstpagenum = 0, lastpagenum = 0;

    int curheight = 1;

    if (d->m_rotate == RotateType_0 || d->m_rotate == RotateType_180 || d->m_rotate == RotateType_Normal) {
        curheight = d->m_scale * d->m_imageheight;
    } else {
        curheight = d->m_scale * d->m_imagewidth;
    }

    int icount = curheight > 0 ? viewport()->rect().height() / (curheight) : 0; //当前页一共能显示多少个
    icount = icount > 0 ? icount + 2 : 2;

    if (d->m_viewmode == ViewMode_SinglePage) {
        if (icount > 3 && icount <= 4) {
            firstpagenum = d->m_currentpageno - 2 >= 0 ? d->m_currentpageno - 2 : 0;
            lastpagenum = d->m_currentpageno + 2;
        } else if (icount > 4) {
            firstpagenum = d->m_currentpageno - (icount - 2) >= 0 ? d->m_currentpageno - (icount - 2) : 0;
            lastpagenum = d->m_currentpageno + icount;
        } else {
            firstpagenum = d->m_currentpageno - 1 >= 0 ? d->m_currentpageno - 1 : d->m_currentpageno;
            lastpagenum = d->m_currentpageno + icount - 1;
        }
    } else if (d->m_viewmode == ViewMode_FacingPage) {
        //当前显示比例较小或者视窗较大，因此可以多加载几页
        if (icount > 3 && icount <= 4) {
            firstpagenum = d->m_currentpageno - 2 * 2 >= 0 ? d->m_currentpageno - (icount - 2) * 2 : 0;
            lastpagenum = d->m_currentpageno + icount * 2;
        } else if (icount > 4) {
            firstpagenum = d->m_currentpageno - (icount - 2) * 2 >= 0 ? d->m_currentpageno - (icount - 2) * 2 : 0;
            lastpagenum = d->m_currentpageno + icount * 2;
        } else {
            firstpagenum = d->m_currentpageno - 2 >= 0 ? d->m_currentpageno - 2 : 0;
            lastpagenum = d->m_currentpageno + (icount - 1) * 2 + 1;
        }
    }

    for (int i = firstpagenum; i <= lastpagenum ; i++) {
        if (i >= 0 && i < d->m_pages.size())
            d->m_pages.at(i)->showImage(d->m_scale, d->m_rotate);
    }

    for (int i = 0; i < d->m_pages.size(); i++) {
        bool bshow = false;
        for (int j = firstpagenum ; j <= lastpagenum; j++) {
            if (i == j) {
                bshow = true;
                break;
            }
        }
        if (!bshow)
            d->m_pages.at(i)->clearImage();
    }

    update(viewport()->rect());

    return true;
}

double DocummentBase::adaptWidthAndShow(double width)
{
    Q_D(DocummentBase);

    if (!bDocummentExist() && d->m_pages.size() > 0)
        return -1;

    if (width < EPSINON) {
        return -1;
    }

    double imageoriginalheight = 0;
    double imageoriginalwidth = 0;
    foreach (PageBase *base, d->m_pages) {
        if (base->getOriginalImageWidth() > imageoriginalwidth)
            imageoriginalwidth = base->getOriginalImageWidth();
        if (base->getOriginalImageHeight() > imageoriginalheight)
            imageoriginalheight = base->getOriginalImageHeight();
    }

    RotateType_EM docrotatetype = d->m_rotate;
    ViewMode_EM docviewmode = d->m_viewmode;
    width = width - d->m_vboxLayout->margin() * 2 - d->m_widgets.at(0)->layout()->margin() * 2 - d->m_pages.at(0)->margin() * 2 - 50;
    double scale = 1;

    if (ViewMode_FacingPage == docviewmode) {
        width -= d->m_widgets.at(0)->layout()->spacing();
        scale = width / 2 / imageoriginalwidth;
        if (RotateType_90 == docrotatetype || RotateType_270 == docrotatetype)
            scale = width / 2 / imageoriginalheight;

    } else {
        scale = width / imageoriginalwidth;
        if (RotateType_90 == docrotatetype || RotateType_270 == docrotatetype)
            scale = width / imageoriginalheight;
    }

    scaleAndShow(scale, RotateType_Normal);
    return scale;
}
double DocummentBase::adaptHeightAndShow(double height)
{
    Q_D(DocummentBase);

    if (!bDocummentExist() && d->m_pages.size() > 0)
        return -1;

    if (height < EPSINON) {
        return -1;
    }

    double imageoriginalheight = 0;
    double imageoriginalwidth = 0;
    foreach (PageBase *base, d->m_pages) {
        if (base->getOriginalImageWidth() > imageoriginalwidth)
            imageoriginalwidth = base->getOriginalImageWidth();
        if (base->getOriginalImageHeight() > imageoriginalheight)
            imageoriginalheight = base->getOriginalImageHeight();
    }

    RotateType_EM docrotatetype = d->m_rotate;
    height = height - d->m_vboxLayout->margin() - d->m_widgets.at(0)->layout()->margin() - d->m_widgets.at(0)->layout()->spacing() - d->m_pages.at(0)->margin();

    double scale = 1;

    scale = height / imageoriginalheight;

    if (RotateType_90 == docrotatetype || RotateType_270 == docrotatetype)
        scale = height / imageoriginalwidth;

    scaleAndShow(scale, RotateType_Normal);

    return scale;
}

void DocummentBase::findNext()
{
    Q_D(DocummentBase);
    if (d->m_pagecountsearch.size() <= 0 || d->m_findcurpage < 0) return;
    if (d->m_findcurpage == d->m_pagecountsearch.lastKey() &&
            d->m_cursearch > d->m_pagecountsearch.find(d->m_findcurpage).value()) {
        d->m_findcurpage = d->m_pagecountsearch.firstKey();
        d->m_cursearch = 1;
    }

    int xvalue = 0, yvalue = 0;
    int curpagecount = d->m_pagecountsearch.find(d->m_findcurpage).value();
    if (curpagecount >= d->m_cursearch) {
        //从上一个切换至下一个，如果findPrev查找的m_cursearch为当前页第一个则此处需要判断是否越界
        if (!d->bfindnext && (d->m_cursearch <= 1 || d->m_cursearch + 2 > curpagecount)) { //前一次是向前查找
            if (d->m_cursearch + 2 > curpagecount) {
                d->bfindnext = true;
                d->m_cursearch += 2;
                findNext();
                return;
            } else {
                d->m_cursearch += 2;
            }
        }

        cacularValueXY(xvalue, yvalue, d->m_findcurpage);
        QScrollBar *scrollBar_Y = verticalScrollBar();
        if (scrollBar_Y)
            scrollBar_Y->setValue(yvalue);
        if (xvalue > 0) {
            QScrollBar *scrollBar_X = horizontalScrollBar();
            if (scrollBar_X)
                scrollBar_X->setValue(xvalue);
        }
        d->m_cursearch++;
    } else {
        d->m_pages.at(d->m_findcurpage)->setCurSearchShow(false);
        QMap<int, int>::const_iterator it = d->m_pagecountsearch.find(d->m_findcurpage);
        if (++it != d->m_pagecountsearch.end()) {
            d->m_cursearch = 1;
            d->m_findcurpage = it.key();
            cacularValueXY(xvalue, yvalue, d->m_findcurpage);

            QScrollBar *scrollBar_Y = verticalScrollBar();
            if (scrollBar_Y)
                scrollBar_Y->setValue(yvalue);

            if (xvalue > 0) {
                QScrollBar *scrollBar_X = horizontalScrollBar() ;
                if (scrollBar_X)
                    scrollBar_X->setValue(xvalue);
            }
            d->m_cursearch++;
        } else {
            qDebug() << __FUNCTION__ << "the end page";
        }
    }
    d->bfindnext = true;
    d->m_pages.at(d->m_findcurpage)->update();
}

void DocummentBase::findPrev()
{
    Q_D(DocummentBase);

    if (d->m_pagecountsearch.size() <= 0 || d->m_findcurpage < 0)
        return;

    if (d->m_findcurpage <= d->m_pagecountsearch.firstKey() &&
            d->m_cursearch < 1) {
        d->m_findcurpage = d->m_pagecountsearch.lastKey();
        d->m_cursearch = d->m_pagecountsearch.find(d->m_findcurpage).value();
    }

    int xvalue = 0, yvalue = 0;

    if (d->m_cursearch >= 1) {
        //从下一个切换至上一个，如果findNext查找的m_cursearch为当前页最后一个则此处需要判断是否越界
        if (d->bfindnext && d->m_cursearch >= 1) { //前一次是向后查找
            if (d->m_cursearch - 2 >= 1) {
                d->m_cursearch -= 2;
            } else {

                d->bfindnext = false;
                d->m_cursearch -= 2;
                findPrev();
                return;
            }
        }
        cacularValueXY(xvalue, yvalue, d->m_findcurpage);
        QScrollBar *scrollBar_Y = verticalScrollBar();
        if (scrollBar_Y)
            scrollBar_Y->setValue(yvalue);
        if (xvalue > 0) {
            QScrollBar *scrollBar_X = horizontalScrollBar();
            if (scrollBar_X)
                scrollBar_X->setValue(xvalue);
        }
        d->m_cursearch--;
    } else {
        d->m_pages.at(d->m_findcurpage)->setCurSearchShow(false);
        QMap<int, int>::const_iterator it = d->m_pagecountsearch.find(d->m_findcurpage);
        if (--it != d->m_pagecountsearch.end()) {
            d->m_cursearch = it.value();
            d->m_findcurpage = it.key();
            cacularValueXY(xvalue, yvalue, d->m_findcurpage);
            QScrollBar *scrollBar_Y = verticalScrollBar();
            if (scrollBar_Y)
                scrollBar_Y->setValue(yvalue);

            if (xvalue > 0) {
                QScrollBar *scrollBar_X = horizontalScrollBar() ;
                if (scrollBar_X)
                    scrollBar_X->setValue(xvalue);
            }
            d->m_cursearch--;
        } else {
            qDebug() << __FUNCTION__ << "first page";
        }
    }
    d->bfindnext = false;
    d->m_pages.at(d->m_findcurpage)->update();
}

void DocummentBase::initConnect()
{
    Q_D(DocummentBase);
    for (int i = 0; i < d->m_pages.size(); i++) {
        d->m_pages.at(i)->setScaleAndRotate(d->m_scale, d->m_rotate);
        connect(d->m_pages.at(i), &PageBase::sigBookMarkButtonClicked, this, [ = ](int page, bool state) {
            emit sigPageBookMarkButtonClicked(page, state);
        });
        connect(d->m_pages.at(i), SIGNAL(signal_MagnifierPixmapCacheLoaded(int)), this, SLOT(slot_MagnifierPixmapCacheLoaded(int)));
        QApplication::processEvents();
    }
}

void DocummentBase::wheelEvent(QWheelEvent *e)
{
    if (QApplication::keyboardModifiers() == Qt::ControlModifier) {
        e->ignore();
        return;
    }
    DScrollArea::wheelEvent(e);
}

void DocummentBase::slot_dataLoaded(bool result)
{
    Q_D(DocummentBase);
    emit signal_openResult(result);
}

void DocummentBase::slot_docummentLoaded(bool result)
{
    Q_D(DocummentBase);

    if (!result) {
        if (d->threadloaddata.isRunning()) {
            d->threadloaddata.requestInterruption();
            d->threadloaddata.wait();
        }
        d->threadloaddata.setDoc(nullptr);
        d->threadloaddata.start();
        return;
    }

    if (d->m_currentpageno > (d->m_pages.size() - 1))
        d->m_currentpageno = (d->m_pages.size() - 1);

    d->donotneedreloaddoc = false;
    if (d->threadloaddata.isRunning()) {
        d->threadloaddata.requestInterruption();
        d->threadloaddata.wait();
    }
    d->threadloaddata.setDoc(this);
    d->threadloaddata.start();

    d->m_widgets.clear();
    for (int i = 0; i < d->m_pages.size(); i++) {
        DWidget *qwidget = new DWidget(this);
        QHBoxLayout *qhblayout = new QHBoxLayout(qwidget);
        qhblayout->setAlignment(qwidget, Qt::AlignCenter);
        qwidget->setLayout(qhblayout);
        d->m_vboxLayout->addWidget(qwidget);
        d->m_vboxLayout->setAlignment(d->m_widget, Qt::AlignCenter);
        qwidget->setMouseTracking(true);
        d->m_widgets.append(qwidget);
        QApplication::processEvents();
    }

    initConnect();
    scaleAndShow(0, d->m_rotate);
}

int DocummentBase::getPageSNum()
{
    Q_D(DocummentBase);
    return d->m_pages.size();
}

bool DocummentBase::exitSlideModel()
{
    Q_D(DocummentBase);
    d->autoplayslidtimer->stop();
    d->m_slidewidget->hide();
    this->show();
    d->m_bslidemodel = false;
    d->m_slidepageno = -1;
    return true;
}

QVector<PageBase *> *DocummentBase::getPages()
{
    Q_D(DocummentBase);
    return &d->m_pages;
}


PageBase *DocummentBase::getPage(int index)
{
    Q_D(DocummentBase);
    if (d->m_pages.size() > index)
        return d->m_pages.at(index);
    return nullptr;
}

void DocummentBase::magnifierClear()
{
    Q_D(DocummentBase);
    if (d->m_magnifierwidget) {
        d->m_magnifierwidget->setPixmap(QPixmap());
        d->m_magnifierwidget->setShowState(false);
        d->m_magnifierpage = -1;
        d->m_magnifierwidget->hide();
    }
}

void DocummentBase::pageMove(double mvx, double mvy)
{
    DScrollBar *scrollBar_X = horizontalScrollBar();
    if (scrollBar_X)
        scrollBar_X->setValue(scrollBar_X->value() + mvx);
    DScrollBar *scrollBar_Y = verticalScrollBar();
    if (scrollBar_Y)
        scrollBar_Y->setValue(scrollBar_Y->value() + mvy);
}

bool DocummentBase::setMagnifierStyle(int magnifierradius, int magnifierringwidth, double magnifierscale)
{
    Q_D(DocummentBase);
    if (!d->m_magnifierwidget) {
        return false;
    }
    d->m_magnifierwidget->setMagnifierRadius(magnifierradius);
    d->m_magnifierwidget->setMagnifierScale(magnifierscale);
    d->m_magnifierwidget->setMagnifierRingWidth(magnifierringwidth);
    return true;
}

void DocummentBase::stopLoadPageThread()
{
    Q_D(DocummentBase);
    d->bcloseing = true;
    d->m_searchTask->cancel();
    for (int i = 0; i < d->m_pages.size(); i++) {
        d->m_pages.at(i)->quitThread();
    }

    if (d->threadloaddata.isRunning()) {
        d->threadloaddata.requestInterruption();
    }
}

void DocummentBase::waitThreadquit()
{
    Q_D(DocummentBase);
    if (d->threadloaddata.isRunning()) {
        d->threadloaddata.wait();
    }

    for (int i = 0; i < d->m_pages.size(); i++) {
        d->m_pages.at(i)->waitThread();
    }
    d->m_searchTask->wait();
    QThreadPool::globalInstance()->waitForDone();
}

bool DocummentBase::openFile(QString filepath, unsigned int ipage, RotateType_EM rotatetype, double scale, ViewMode_EM viewmode)
{
    Q_D(DocummentBase);
    d->m_scale = scale;
    d->m_rotate = rotatetype;
    d->m_viewmode = viewmode;
    d->m_currentpageno = ipage;
    d->donotneedreloaddoc = true;

    if (!loadDocumment(filepath))
        return false;
    return true;
}

bool DocummentBase::setBookMarkState(int page, bool state)
{
    Q_D(DocummentBase);
    if (page < 0 || page >= d->m_pages.size()) {
        return false;
    }
    return d->m_pages.at(page)->setBookMarkState(state);
}

bool DocummentBase::getImage(int pagenum, QImage &image, double width, double height)
{
    Q_D(DocummentBase);
    if (pagenum < 0 || pagenum >= d->m_pages.size()) {
        return false;
    }
    qreal pixelratiof = d->m_pages.at(pagenum)->devicePixelRatioF();
    if (!d->m_pages.at(pagenum)->getImage(image, width * pixelratiof, height * pixelratiof)) {
        return false;
    }
    image.setDevicePixelRatio(d->m_pages.at(pagenum)->devicePixelRatioF());
    return true;
}

bool DocummentBase::getImage(int pagenum, QImage &image, double width)
{
    Q_D(DocummentBase);
    if (pagenum < 0 || pagenum >= d->m_pages.size()) {
        return false;
    }

    double height = d->m_pages.at(pagenum)->getOriginalImageHeight() / d->m_pages.at(pagenum)->getOriginalImageWidth() * width;

    if (height > width * 2)
        height = 2 * width;

    qreal pixelratiof = d->m_pages.at(pagenum)->devicePixelRatioF();

    if (!d->m_pages.at(pagenum)->getImage(image, width * pixelratiof, height * pixelratiof)) {
        return false;
    }
    image.setDevicePixelRatio(d->m_pages.at(pagenum)->devicePixelRatioF());
    return true;
}

bool DocummentBase::getImageMax(int pagenum, QImage &image, double max)
{
    Q_D(DocummentBase);
    if (pagenum < 0 || pagenum >= d->m_pages.size()) {
        return false;
    }

    double height = 0;
    double width  = 0;

    double oriHeight = d->m_pages.at(pagenum)->getOriginalImageHeight();
    double oriWidth  = d->m_pages.at(pagenum)->getOriginalImageWidth();

    if (oriHeight > oriWidth) {
        height = max;
        double ratio = d->m_pages.at(pagenum)->getOriginalImageWidth() / d->m_pages.at(pagenum)->getOriginalImageHeight();
        if (ratio < 0.5)
            ratio = 0.5;
        width  =  ratio * height;
    } else {
        width  = max;
        double ratio = d->m_pages.at(pagenum)->getOriginalImageHeight() / d->m_pages.at(pagenum)->getOriginalImageWidth();
        if (ratio < 0.5)
            ratio = 0.5;
        height = ratio * width;
    }

    qreal pixelratiof = d->m_pages.at(pagenum)->devicePixelRatioF();

    if (!d->m_pages.at(pagenum)->getImage(image, width * pixelratiof, height * pixelratiof)) {
        return false;
    }

    image.setDevicePixelRatio(d->m_pages.at(pagenum)->devicePixelRatioF());

    return true;
}

bool DocummentBase::save(const QString &)
{
    return false;
}

void DocummentBase::docBasicInfo(stFileInfo &info)
{
    Q_D(DocummentBase);
    info = *(d->m_fileinfo);
}


bool DocummentBase::loadData()
{
    Q_D(DocummentBase);
    if (!bDocummentExist())
        return false;
    QTime loadtime;
    loadtime.start();
    bool bfirst = true;
    for (int i = 0; i < d->m_pages.size(); i++) {
        if (QThread::currentThread()->isInterruptionRequested()) {
            break;
        }
        if (bfirst && (i >= 3 || d->m_pages.size() <= 3 || d->m_pages.size() <= 3)) {
            bfirst = false;
            emit signal_openResult(true);
        }
        d->m_pages.at(i)->getInterFace()->loadData();
    }
    return true;
}


bool DocummentBase::loadDoc(QString path)
{
    Q_D(DocummentBase);
    if (!bDocummentExist())
        return false;
    d->loadDocumment(path);
    return true;
}

void DocummentBase::selectAllText()
{
    Q_D(DocummentBase);
    if (!bDocummentExist())
        return;
    for (int i = 0; i < d->m_pages.size(); i++) {
        d->m_pages.at(i)->selectAllText();
    }
}

void DocummentBase::setAutoPlaySlide(bool autoplay, int timemsec)
{
    Q_D(DocummentBase);
    d->bautoplayslide = autoplay;
    d->m_autoplayslidmsec = timemsec;
    if (d->m_bslidemodel) {
        d->autoplayslidtimer->stop();
        if (d->bautoplayslide)
            d->autoplayslidtimer->start(timemsec);
    }
}

bool DocummentBase::getAutoPlaySlideStatu()
{
    Q_D(DocummentBase);
    return d->bautoplayslide;
}

void DocummentBase::setViewFocus()
{
    viewport()->setFocus();
}

double DocummentBase::getMaxZoomratio()
{
    Q_D(DocummentBase);
    return  d->m_maxzoomratio;
}

void DocummentBase::jumpToOutline(const qreal &realleft, const qreal &realtop, int ipage)
{
    Q_D(DocummentBase);
    if (d->m_rotate != RotateType_Normal && d->m_rotate != RotateType_0) {
        pageJump(ipage);
        return;
    }
    int xvalue, yvalue;
    xvalue = yvalue = 0;
    double curwidth = d->m_scale * d->m_imagewidth;
    double curheight = d->m_scale * d->m_imageheight;
    if (ipage < d->m_pages.size()) {
        if (d->m_viewmode == ViewMode_SinglePage) {
            switch (d->m_rotate) {
            case RotateType_Normal:
            case RotateType_0:
            case RotateType_180: {
                double topspace = (d->m_widgets.at(ipage)->height() - curheight) / 2;
                double leftspace = (d->m_widgets.at(ipage)->width() - curwidth) / 2;
                double leftposition = curwidth * realleft + leftspace;
                double topposition = curheight * realtop + topspace;
                yvalue = d->m_widgets.at(ipage)->y() + topposition;
                qDebug() << realleft << realtop << leftposition << topposition << yvalue;
                //横向有缩放
                if (frameRect().width() < curwidth) {
                    if (leftposition > frameRect().width()) {
                        xvalue = leftposition - frameRect().width();
                    } else {
                        xvalue = 0;
                    }
                }
            }
            break;
            case RotateType_90:
            case RotateType_270: {

            }
            break;
            }
        } else if (d->m_viewmode == ViewMode_FacingPage) {
            double curwidgetwidth = d->m_widgets.at(ipage / 2)->width();
            double topspace = (d->m_widgets.at(ipage)->height() - curheight) / 2;
            double leftspace = d->m_pages.at(ipage)->x();

            double leftposition = curwidth * realleft + leftspace ;
            double topposition = curheight * realtop + topspace;
            yvalue = d->m_widgets.at(ipage / 2)->y() + topposition;
            //横向有缩放
            if (frameRect().width() < curwidgetwidth && ipage % 2 == 1) {
                xvalue = leftposition;
            } else {
                xvalue = leftspace;
            }
        }
    }

    QScrollBar *scrollBar_X = horizontalScrollBar();
    if (scrollBar_X)
        scrollBar_X->setValue(xvalue);

    QScrollBar *scrollBar_Y = verticalScrollBar();
    if (scrollBar_Y)
        scrollBar_Y->setValue(yvalue);
}

QString DocummentBase::pagenum2label(int index)
{
    Q_D(DocummentBase);
    auto it = d->m_pagenum2label.find(index);
    return it != d->m_pagenum2label.end() ? it.value() : QString();
}

int DocummentBase::label2pagenum(QString label)
{
    Q_D(DocummentBase);
    auto it = d->m_label2pagenum.find(label);
    return it != d->m_label2pagenum.end() ? it.value() : -1;
}

bool DocummentBase::haslabel()
{
    Q_D(DocummentBase);
    return d->m_label2pagenum.size() > 0 ? true : false;
}

QPoint DocummentBase::transformPoint(const QPoint &pt, RotateType_EM type, double scale)
{
    Q_D(DocummentBase);
    QPoint pos;
    double curwidth, curheight;
    switch (type) {
    case RotateType_0:
    case RotateType_Normal: {
        curwidth = d->m_imagewidth * scale;
        curheight = d->m_imageheight * scale;
        pos.setX(pt.x());
        pos.setY(pt.y());
    }
    break;
    case RotateType_90: {
        curwidth = d->m_imageheight  * scale;
        curheight = d->m_imagewidth * scale;
        pos.setX(pt.y());
        pos.setY(curwidth - pt.x());
    }
    break;
    case RotateType_180: {
        curwidth = d->m_imagewidth * scale;
        curheight = d->m_imageheight * scale;
        pos.setX(curwidth - pt.x());
        pos.setY(curheight - pt.y());
    }
    break;
    case RotateType_270: {
        curwidth = d->m_imageheight  * scale;
        curheight = d->m_imagewidth * scale;
        pos.setX((curheight - pt.y()));
        pos.setY(pt.x());
    }
    break;
    }
    return  pos;
}


