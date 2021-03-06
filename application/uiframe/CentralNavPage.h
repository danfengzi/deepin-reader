#ifndef CentralNavPage_H
#define CentralNavPage_H

#include "CustomControl/CustomWidget.h"

class CentralNavPage : public CustomWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(CentralNavPage)

public:
    explicit CentralNavPage(DWidget *parent = nullptr);

    ~CentralNavPage() override;

signals:
    void sigNeedOpenFilesExec();

protected:
    void initWidget() override;

private slots:
    void slotChooseBtnClicked();

    void slotUpdateTheme();

};

#endif // OPENFILEWIDGET_H
