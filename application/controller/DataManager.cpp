#include "DataManager.h"
#include "application.h"

#include <QKeySequence>

DataManager::DataManager(QObject *parent) :
    QObject(parent)
{
    m_listColor.append(QColor("#FFA503"));
    m_listColor.append(QColor("#FF1C49"));
    m_listColor.append(QColor("#9023FC"));
    m_listColor.append(QColor("#3468FF"));
    m_listColor.append(QColor("#00C7E1"));
    m_listColor.append(QColor("#05EA6B"));
    m_listColor.append(QColor("#FEF144"));
    m_listColor.append(QColor("#D5D5D1"));

    m_selectColor = m_listColor.at(0);

    initKeyList();
}

void DataManager::initKeyList()
{
    m_pKeyList.append(QKeySequence::HelpContents);
    m_pKeyList.append(QKeySequence::Delete);
    m_pKeyList.append(QKeySequence::Find);
    m_pKeyList.append(QKeySequence::Open);
    m_pKeyList.append(QKeySequence::Print);
    m_pKeyList.append(QKeySequence::Save);
    m_pKeyList.append(QKeySequence::Copy);

    m_pKeyList.append(QKeySequence(Qt::Key_Space));
    m_pKeyList.append(QKeySequence(Qt::Key_F11));
    m_pKeyList.append(QKeySequence(Qt::Key_Escape));
    m_pKeyList.append(QKeySequence(Qt::Key_PageUp));
    m_pKeyList.append(QKeySequence(Qt::Key_PageDown));
    m_pKeyList.append(QKeySequence(Qt::Key_Down));
    m_pKeyList.append(QKeySequence(Qt::Key_Up));
    m_pKeyList.append(QKeySequence(Qt::Key_Left));
    m_pKeyList.append(QKeySequence(Qt::Key_Right));

    m_pKeyList.append(QKeySequence(Qt::ALT | Qt::Key_1));
    m_pKeyList.append(QKeySequence(Qt::ALT | Qt::Key_2));
    m_pKeyList.append(QKeySequence(Qt::ALT | Qt::Key_Z));

    m_pKeyList.append(QKeySequence(Qt::CTRL | Qt::Key_1));
    m_pKeyList.append(QKeySequence(Qt::CTRL | Qt::Key_2));
    m_pKeyList.append(QKeySequence(Qt::CTRL | Qt::Key_3));
    m_pKeyList.append(QKeySequence(Qt::CTRL | Qt::Key_B));
    m_pKeyList.append(QKeySequence(Qt::CTRL | Qt::Key_E));
    m_pKeyList.append(QKeySequence(Qt::CTRL | Qt::Key_I));
    m_pKeyList.append(QKeySequence(Qt::CTRL | Qt::Key_L));
    m_pKeyList.append(QKeySequence(Qt::CTRL | Qt::Key_M));
    m_pKeyList.append(QKeySequence(Qt::CTRL | Qt::Key_R));
    m_pKeyList.append(QKeySequence(Qt::CTRL | Qt::Key_Minus));      //  ctrl+-
    m_pKeyList.append(QKeySequence(Qt::CTRL | Qt::Key_Equal));      //  ctrl+=

    m_pKeyList.append(QKeySequence(Qt::CTRL | Qt::Key_Plus));       //  ctrl++

    m_pKeyList.append(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R));
    m_pKeyList.append(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
    m_pKeyList.append(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Slash));
}

QString DataManager::strOnlyFilePath() const
{
    return m_strOnlyFilePath;
}

void DataManager::setStrOnlyFilePath(const QString &strOnlyFilePath)
{
    if (strOnlyFilePath == "") {
        m_bIsUpdate = true;
    } else {
        m_bIsUpdate = false;
    }
    dApp->dbM->setStrFilePath(strOnlyFilePath);

    m_strOnlyFilePath = strOnlyFilePath;
}

int DataManager::CurShowState() const
{
    return m_nCurShowState;
}

void DataManager::setCurShowState(int nCurShowState)
{
    m_nCurShowState = nCurShowState;
}

bool DataManager::bIsUpdate() const
{
    return m_bIsUpdate;
}

void DataManager::setBIsUpdate(bool bIsUpdate)
{
    m_bIsUpdate = bIsUpdate;
}

QList<QColor> DataManager::getLightColorList()
{
    return m_listColor;
}

void DataManager::setMousePressLocal(const bool &highLight, const QPoint &point)
{
    m_bIsHighLight = highLight;

    QPoint t_point;
    int t_w = point.x();
    int t_h = point.y();

    int screenW =  m_screenRect.width();
    int screenH =  m_screenRect.height();

    int noteWidgetW = m_smallNoteSize.width();
    int noteWidgetH = m_smallNoteSize.height();

    if (t_h + noteWidgetH > screenH) {
        t_h = screenH - noteWidgetH;
    }

    if (t_w + noteWidgetW > screenW) {
        t_w -= noteWidgetW;
    }

    t_point.setX(t_w);
    t_point.setY(t_h);

    m_point = t_point;
}

QString DataManager::gettrCurrentTheme() const
{
    return m_strCurrentTheme;
}

void DataManager::settrCurrentTheme(const QString &strCurrentTheme)
{
    m_strCurrentTheme = strCurrentTheme;
}

QList<QKeySequence> DataManager::getPKeyList() const
{
    return m_pKeyList;
}
