// SPDX-FileCopyrightText: 2021 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef AUDIOPREVIEWPLUGIN_H
#define AUDIOPREVIEWPLUGIN_H

#include "previewplugin.h"
#include "libaudioviewer.h"

#include <QSharedPointer>

namespace GrandSearch {
namespace audio_preview {

class AudioView;
class AudioPreviewPlugin : public QObject, public PreviewPlugin
{
    Q_OBJECT
public:
    explicit AudioPreviewPlugin(QObject *parent = nullptr);
    ~AudioPreviewPlugin() Q_DECL_OVERRIDE;
    void init(QObject *proxyInter) Q_DECL_OVERRIDE;
    bool previewItem(const ItemInfo &item) Q_DECL_OVERRIDE;
    ItemInfo item() const Q_DECL_OVERRIDE;
    bool stopPreview() Q_DECL_OVERRIDE;
    QWidget *contentWidget() const Q_DECL_OVERRIDE;
    DetailInfoList getAttributeDetailInfo() const Q_DECL_OVERRIDE;
    QWidget *toolBarWidget() const Q_DECL_OVERRIDE;
    bool showToolBar() const Q_DECL_OVERRIDE;
    void setExtendParser(QSharedPointer<GrandSearch::LibAudioViewer> parser);
private:
    ItemInfo m_item;
    DetailInfoList m_detailInfos;
    AudioView *m_audioView = nullptr;
    QSharedPointer<GrandSearch::LibAudioViewer> m_parser;
};

}}

#endif // AUDIOPREVIEWPLUGIN_H
