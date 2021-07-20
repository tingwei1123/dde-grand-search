/*
 * Copyright (C) 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     zhangyu<zhangyub@uniontech.com>
 *
 * Maintainer: zhangyu<zhangyub@uniontech.com>
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

#include "grandsearchinterface.h"
#include "grandsearchinterfaceprivate.h"
#include "maincontroller/maincontroller.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QFileInfo>
#include <QThread>
#include <QCoreApplication>
#include <QDebug>

#define CHECKINVOKER(ret) if (!d->isAccessable(message()))\
                            return ret

GrandSearchInterfacePrivate::GrandSearchInterfacePrivate(GrandSearchInterface *parent)
    : QObject(parent)
    , q(parent)
{
    //允许调用dbus接口的可执行程序名单
    m_permit.insert("/usr/bin/dde-grand-search", true);

    //for test
    m_permit.insert("/usr/bin/python3.7", true);
}

GrandSearchInterfacePrivate::~GrandSearchInterfacePrivate()
{
    if (m_main) {
        delete m_main;
        m_main = nullptr;
    }
}

bool GrandSearchInterfacePrivate::isAccessable(const QDBusMessage &msg) const
{
    QDBusConnectionInterface *ifs = QDBusConnection::sessionBus().interface();
    Q_ASSERT(ifs);

    //获取调用方的进程ID
    uint invokerPid = ifs->servicePid(msg.service()).value();

    //读取进程的可执行程序路径
    QFileInfo fileInfo(QString("/proc/%1/exe").arg(invokerPid));
    if (!fileInfo.exists())
        return false;
    auto invokerPath = fileInfo.canonicalFilePath();

    bool accessable = m_permit.value(invokerPath, false);
    if (!accessable)
        qWarning() << "invalid invoker:" << invokerPath << "pid" << invokerPid << accessable;

//调试使用
#ifdef QT_DEBUG
    return true;
#endif
    return accessable;
}

void GrandSearchInterfacePrivate::terminate()
{
    m_deadline.stop();

    //停止搜索
    if (m_main)
        m_main->terminate();
}

void GrandSearchInterfacePrivate::onMatched()
{
    //控制在主线程
    qInfo() << "notify matched" << m_session;
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    if (!m_session.isEmpty()) {
        emit q->Matched(m_session);
    }
}

void GrandSearchInterfacePrivate::onSearchCompleted()
{
    //控制在主线程
    qInfo() << "notify finished" << m_session;
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    if (!m_session.isEmpty()) {
        emit q->SearchCompleted(m_session);
    }
}

GrandSearchInterface::GrandSearchInterface(QObject *parent)
    : QObject(parent)
    , d(new GrandSearchInterfacePrivate(this))
{

}

GrandSearchInterface::~GrandSearchInterface()
{
}

bool GrandSearchInterface::init()
{
    Q_ASSERT(d->m_main == nullptr);

    //超时后强制结束任务
    d->m_deadline.setInterval(40000);
    d->m_deadline.setSingleShot(true);
    connect(&d->m_deadline, &QTimer::timeout, d, &GrandSearchInterfacePrivate::terminate);

    d->m_main = new MainController;
    //直连，防止被事件循环打乱时序
    connect(d->m_main, &MainController::matched, d, &GrandSearchInterfacePrivate::onMatched, Qt::DirectConnection);
    connect(d->m_main, &MainController::searchCompleted, d, &GrandSearchInterfacePrivate::onSearchCompleted, Qt::DirectConnection);

    return d->m_main->init();
}

bool GrandSearchInterface::Search(const QString &session, const QString &key)
{
    qDebug() << __FUNCTION__ << "session " << session;
    CHECKINVOKER(false);

    //会话ID检查，符合UUID宽度36。
    static int sessionLength = 36;
    //关键词检查，最大长度不超过128（todo 待确定）
    static int maxKeyLength = 128;
    if (session.size() != sessionLength || key.isEmpty() || key.size() > maxKeyLength) {
        return false;
    }

    //发起新的搜索
    if (d->m_main->newSearch(key)) {
        d->m_session = session;
        d->m_deadline.start();
        return true;
    }

    d->m_deadline.stop();
    d->m_session.clear();
    return false;
}

void GrandSearchInterface::Terminate()
{
    qDebug() << __FUNCTION__;
    CHECKINVOKER();
    d->terminate();
}

QByteArray GrandSearchInterface::MatchedResults(const QString &session)
{
    qDebug() << __FUNCTION__ << "session " << session;
    QByteArray ret;
    CHECKINVOKER(ret);

    if (d->vaildSession(session)) {
        ret = d->m_main->getResults();
    }

    return ret;
}

QByteArray GrandSearchInterface::MatchedBuffer(const QString &session)
{
    qDebug() << __FUNCTION__ << "session " << session;
    QByteArray ret;
    CHECKINVOKER(ret);

    if (d->vaildSession(session) && !d->m_main->isEmptyBuffer()) {
        ret = d->m_main->readBuffer();
    }

    return ret;
}

bool GrandSearchInterface::OpenWithPlugin(const QString &item)
{
    CHECKINVOKER(false);

    return false;
}

bool GrandSearchInterface::Configure(const QVariantMap &)
{
    CHECKINVOKER(false);

    return false;
}

QVariantMap GrandSearchInterface::Configuration() const
{
    return QVariantMap();
}

bool GrandSearchInterface::SetFeedBackStrategy(const QVariantMap &)
{
    CHECKINVOKER(false);

    return false;
}

QVariantMap GrandSearchInterface::FeedBackStrategy() const
{
    return QVariantMap();
}

bool GrandSearchInterface::KeepAlive(const QString &session)
{
    CHECKINVOKER(false);

    //重新计时
    if (d->vaildSession(session)) {
        d->m_deadline.start();
        return true;
    }
    return false;
}
