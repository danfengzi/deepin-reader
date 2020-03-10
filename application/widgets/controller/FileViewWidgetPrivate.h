#ifndef FILEVIEWWIDGETPRIVATE_H
#define FILEVIEWWIDGETPRIVATE_H

#include <QObject>
#include <QMouseEvent>

#include "MsgHeader.h"
#include "docview/pagebase.h"

class Annotation;
class DocummentProxy;
class FileViewWidget;

class FileViewWidgetPrivate: public QObject
{
    Q_OBJECT

public:
    FileViewWidgetPrivate(FileViewWidget *parent = nullptr);

private slots:
    void slotDealWithMenu(const int &, const QString &);
    void SlotNoteViewMsg(const int &, const QString &);

private:
    void setProxy(DocummentProxy *);

private:
    void mouseMoveEvent(QMouseEvent *event);

    void __ShowMagnifier(const QPoint &clickPoint);
    void __FilePageMove(const QPoint &endPos);
    void __MouseSelectText(const QPoint &clickPoint);
    void __OtherMouseMove(const QPoint &globalPos);
    void __ShowPageNoteWidget(const QPoint &docPos);
    void __ShowFileNoteWidget(const QPoint &docPos);

private:
    void mousePressEvent(QMouseEvent *event);
    void __AddIconAnnotation(const QPoint &globalPos);
    void __HandlClicked(const QPoint &globalPos);
    void __OtherMousePress(const QPoint &globalPos);
    void __ClickPageLink(Page::Link *pLink);

private:
    void mouseReleaseEvent(QMouseEvent *event);

private:
    void AddHighLight(const QString &msgContent);
    void AddHighLightAnnotation(const QString &msgContent);
    void RemoveHighLight(const QString &msgContent);
    void ChangeAnnotationColor(const QString &msgContent);
    void RemoveAnnotation(const QString &msgContent);
    void UpdateAnnotationText(const QString &msgContent);
    void AddPageIconAnnotation(const QString &msgContent);
    void DeletePageIconAnnotation(const QString &msgContent);
    void UpdatePageIconAnnotation(const QString &msgContent);

private:
    bool        m_filechanged = false;
    bool        m_bSelectOrMove = false;                //  是否可以选中文字、移动
    bool        m_bIsHandleSelect = false;              //  是否可以选中
    QPoint      m_pMoveStartPoint;

    Annotation  *m_pAnnotation = nullptr;

protected:
    FileViewWidget *q_ptr;
    Q_DECLARE_PUBLIC(FileViewWidget)
};

#endif // FILEVIEWWIDGETPRIVATE_H
