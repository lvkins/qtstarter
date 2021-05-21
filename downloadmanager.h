/**
 * Copyright (C) Łukasz Szwedt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include <QTemporaryFile>
#include <QPair>
#include <QObject>
#include <QQueue>
#include <QTime>
#include <QUrl>
#include <QLabel>
#include <QProgressBar>
#include <QNetworkAccessManager>
#include <QWinTaskbarButton>
#include <QWinTaskbarProgress>

#include "stdafx.h"
#include "textprogressbar.h"
#include "qmessagebox.h"

// QNetworkReply err codes http://doc.qt.io/qt-5/qnetworkreply.html#NetworkError-enum

const QFileInfo emptyFile = QFileInfo();

class DownloadManager: public QObject
{
    Q_OBJECT
public:
    DownloadManager(QObject* parent = nullptr);
	~DownloadManager();

	void setupTaskbarProgress(QWidget* wnd);

    void append(const QStringList &urlList);
    void append(const QUrl &url, const QFileInfo& outputFile = emptyFile);

signals:
	void fail(const QNetworkReply::NetworkError);
	void progress(qint64, qint64, const QString& speed);
	void downloaded(const QString& fileName); // could've used finished(), but we need param
    void finished();

private slots:
    void startNextDownload();
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished();
    void downloadReadyRead();

private:
    QNetworkAccessManager manager;
    QQueue<QPair<QUrl, QFileInfo>> downloadQueue;
    QNetworkReply *currentDownload;
    QFile output;
    QTime downloadTime;
    TextProgressBar progressBar;

    int downloadedCount;
    int totalCount;

public:
	QWinTaskbarButton* taskbarBtn;

};

#endif