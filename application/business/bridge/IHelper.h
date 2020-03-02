/*
 * Copyright (C) 2019 ~ 2020 UOS Technology Co., Ltd.
 *
 * Author:     wangzhxiaun
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
#ifndef IHELPER_H
#define IHELPER_H

#include <QObject>

class HelperImpl;

class IHelper : public QObject
{
protected:
    explicit IHelper(QObject *parent = nullptr);

public:
    virtual ~IHelper() {}
    virtual QString qDealWithData(const int &, const QString &) = 0;
    virtual void qSetCurrentPath(const QString &) = 0;
};

class Helper : public IHelper
{
public:
    explicit Helper(QObject *parent = nullptr);
    QString qDealWithData(const int &msgType, const QString &msgContent) override;
    void qSetCurrentPath(const QString &) override;

private:
    HelperImpl  *m_pAnnotatinHelperImpl = nullptr;
    HelperImpl  *m_pDocHelperImpl = nullptr;
};


#endif // IHELPER_H
