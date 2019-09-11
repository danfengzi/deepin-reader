#include "BookMarkStateLabel.h"
#include "subjectObserver/ModuleHeader.h"

#include "controller/NotifySubject.h"

BookMarkStateLabel::BookMarkStateLabel(DWidget *parent)
    : QLabel ( parent)
{
    setFixedSize(QSize(39, 39));

    setMouseTracking(true);
}

void BookMarkStateLabel::mouseMoveEvent(QMouseEvent *event)
{
    if (m_bChecked) {
        setPixmapState(ImageModule::g_checked_state);
    } else {
        setPixmapState(ImageModule::g_hover_state);
    }

    DLabel::mouseMoveEvent(event);
}

void BookMarkStateLabel::mousePressEvent(QMouseEvent *event)
{
    m_bChecked = !m_bChecked;

    if (!m_bChecked) {
        setPixmapState(ImageModule::g_press_state);
    } else {
        setPixmapState(ImageModule::g_checked_state);
    }

    DLabel::mousePressEvent(event);
}

void BookMarkStateLabel::leaveEvent(QEvent *event)
{
    if (m_bChecked) {
        setPixmapState(ImageModule::g_checked_state);
    } else {
        this->clear();
    }

    DLabel::leaveEvent(event);
}

void BookMarkStateLabel::setPixmapState(const QString &state)
{
    QString ssPath = QString(":/resources/image/%1/bookmark.svg").arg(state);
    QPixmap pixmap(ssPath);
    this->setPixmap(pixmap);
}