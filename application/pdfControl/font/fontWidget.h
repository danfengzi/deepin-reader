﻿#ifndef FONTWIDGET_H
#define FONTWIDGET_H

#include <DLabel>
#include <DSlider>

#include <QStringList>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPixmap>
#include <QPainter>
#include <QRectF>
#include <QtMath>

#include "CustomControl/CustomWidget.h"
#include "CustomControl/CustomClickLabel.h"
#include "docview/docummentproxy.h"

/**
 * @brief The FontWidget class
 * 字体调整窗体
 */
class FontWidget : public CustomWidget
{
    Q_OBJECT
public:
    FontWidget(CustomWidget *parent = nullptr);

signals:
    void sigWidgetHide();
    void sigOpenFileOk();

public:
    int dealWithData(const int &, const QString &) Q_DECL_OVERRIDE;

protected:
    void initWidget() Q_DECL_OVERRIDE;

private slots:
    void slotUpdateTheme();
    void slotReset();

private:
    void rotateFileView(bool isRight = true);
    void scaleAndRotate(int);
    void setShowSuitHIcon();
    void setShowSuitWIcon();
    void initConnection();

    DLabel *getLabelLineH();

    void initScaleLabel();
    void initScaleSlider();
    void initDowbleShow();
    void initAdaptateHeight();
    void initAdaptateWidght();

private slots:
    void slotSetChangeVal(int);
    void slotSetDoubPageViewCheckIcon();
    void slotSetSuitHCheckIcon();
    void slotSetSuitWCheckIcon();
    void slotSetRotateLeftCheckIcon();
    void slotSetRotateRightCheckIcon();

private:
    QHBoxLayout     *m_pDoubleShowLayout = nullptr;
    QHBoxLayout     *m_pAdaptateHeightLayout = nullptr;
    QHBoxLayout     *m_pAdaptateWidghtLayout = nullptr;

    CustomClickLabel *m_pEnlargeLab = nullptr;         // 缩放比例
    DSlider *m_pEnlargeSlider = nullptr;     // 缩放比例slider

    CustomClickLabel      *m_pDoubPageViewLabelIcon = nullptr;    // 双页显示ICON标签
    CustomClickLabel      *m_pSuitHLabelIcon = nullptr;           // 自适应高度ICON标签
    CustomClickLabel      *m_pSuitWLabelIcon = nullptr;           // 自适应宽度ICON标签

    int m_rotate = 0;                        // 旋转角度
    RotateType_EM m_rotateType = RotateType_Normal;    // 旋转类型
    bool m_bSuitH = false;                   // 自适应高度
    bool m_bSuitW = false;                   // 自适应宽度
    bool m_isDoubPage = false;               //  双页显示标志
    bool m_bIsAdaptMove = false;
};

#endif // FONTWIDGET_H
