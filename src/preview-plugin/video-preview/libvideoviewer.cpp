// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "libvideoviewer.h"

#include <QLibrary>
#include <QDebug>
#include <QJsonObject>
#include <QJsonValue>
#include <QSize>

using namespace GrandSearch;

LibVideoViewer::LibVideoViewer(QObject *parent) : QObject(parent)
{
}

LibVideoViewer::~LibVideoViewer()
{

}

bool LibVideoViewer::initLibrary()
{
    if (m_imageLib)
        return true;
    qInfo() << "load libimageviewer.so....";
    m_imageLib = new QLibrary("libimageviewer.so", this);
    if (!m_imageLib->load()) {
        qWarning() << "fail to load libimageviewer" << m_imageLib->errorString();
        delete m_imageLib;
        m_imageLib = nullptr;
        return false;
    }

    m_getMovieCoverFunc = reinterpret_cast<GetMovieCoverFunc>(m_imageLib->resolve("getMovieCover"));
    if (!m_getMovieCoverFunc) {
        qWarning() << "can not find getMovieCover.";
    }

    m_getMovieInfoFunc = reinterpret_cast<GetMovieInfoFunc>(m_imageLib->resolve("getMovieInfoByJson"));
    if (!m_getMovieInfoFunc) {
        qWarning() << "can not find getMovieInfoByJson.";
    }

    qInfo() << "load successfully";
    return true;
}

bool LibVideoViewer::getMovieInfo(const QUrl &url, QSize &dimension, qint64 &duration)
{
    if (!m_getMovieInfoFunc)
        return false;

    QJsonObject json;
    m_getMovieInfoFunc(url, &json);

    int ok = 0;
    if (json.contains("Base")) {
       const QJsonObject &base = json.value("Base").toObject();
       const QJsonValue &durationJson = base.value("Duration");
       if (durationJson.isString()) {
           QString timeStr = durationJson.toString();
           qDebug() << "get duration" << timeStr;
           QStringList times = timeStr.split(':');
           if (times.size() == 3) {
               duration = times.at(0).toInt() * 3600 + times.at(1).toInt() * 60 + times.at(2).toInt();
               ok = ok | 1;
           }
       }
       if (!(ok & 1)) {
           duration = -1;
           qDebug() << "get duration failed";
       }

       const QJsonValue &resolutionValue = base.value("Resolution");
       if (resolutionValue.isString()) {
           QString resolutionStr = resolutionValue.toString();
           qDebug() << "get resolution" << resolutionStr;
           QStringList strs = resolutionStr.split('x');
           if (strs.size() == 2) {
               dimension = QSize(strs.at(0).toInt(), strs.at(1).toInt());
               ok = ok | 2;
           }
       }

       if (!(ok & 2)) {
            dimension = QSize(-1, -1);
            qDebug() << "get dimension failed";
       }
    } else {
        return false;
    }

    return true;
}

bool LibVideoViewer::getMovieCover(const QUrl &url, QImage &image)
{
    if (!m_getMovieCoverFunc)
        return false;

    m_getMovieCoverFunc(url, "", &image);
    return true;
}
