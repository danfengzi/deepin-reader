/*
* Copyright (C) 2019 ~ 2019 Deepin Technology Co., Ltd.
*
* Author:     leiyu <leiyu@live.com>
* Maintainer: leiyu <leiyu@deepin.com>
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef WORDWRAPLABEL_H
#define WORDWRAPLABEL_H

#include <DLabel>

DWIDGET_USE_NAMESPACE
class WordWrapLabel : public DLabel
{
public:
    WordWrapLabel(QWidget* parent = nullptr);
    void setText(const QString& text);
    void setMargin(int margin);

private:
    void adjustContent();

protected:
    void paintEvent(QPaintEvent *event);

private:
    QString m_text;
    int m_margin;
};

#endif // WORDWRAPLABEL_H
