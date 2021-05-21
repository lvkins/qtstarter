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

#include "stdafx.h"
#include "downloadmanager.h"

#include <QFileInfo>
#include <QNetworkRequest>
#include <QStringList>
#include <QTimer>
#include <iostream>

#include "utils.h"

DownloadManager::DownloadManager(QObject* parent)
    : downloadedCount(0), totalCount(0), taskbarBtn(nullptr) { }

DownloadManager::~DownloadManager() {
	QDir dir(TMP_DOWNLOAD_PATH);
	dir.setNameFilters(QStringList() << "tmp.*");
	dir.setFilter(QDir::Files);
	foreach(QString dirFile, dir.entryList()) {
		dir.remove(dirFile);
	}
}

void DownloadManager::setupTaskbarProgress(QWidget* wnd) {
	taskbarBtn = new QWinTaskbarButton(wnd);
    taskbarBtn->setWindow(wnd->windowHandle());
}

void DownloadManager::append(const QStringList &urlList)
{
    foreach (QString url, urlList)
        append(QUrl::fromEncoded(url.toLocal8Bit()));

    if (downloadQueue.isEmpty())
        QTimer::singleShot(0, this, SIGNAL(finished()));
}

void DownloadManager::append(const QUrl& url, const QFileInfo& outputFile)
{
	if (!outputFile.absoluteDir().exists()) {
		outputFile.absoluteDir().mkpath(".");
	}

    if (downloadQueue.isEmpty()) {
        QTimer::singleShot(0, this, SLOT(startNextDownload()));
	}

	downloadQueue.enqueue(qMakePair(url, outputFile));
    ++totalCount;
}

void DownloadManager::startNextDownload()
{
    if (downloadQueue.isEmpty()) {
        printf("%d/%d files downloaded successfully\n", downloadedCount, totalCount);
		if (taskbarBtn) {
			taskbarBtn->progress()->setVisible(false);
		}
        emit finished();
        return;
    }

	const auto& queue = downloadQueue.dequeue();

	QUrl url = queue.first;

	if (queue.second.fileName().isEmpty()) {
		const auto downloadsPath = QDir(QString("%1/tmp.%2").arg(TMP_DOWNLOAD_PATH, getRandomString(5)));
		output.setFileName(downloadsPath.absolutePath());
	} else {
		output.setFileName(queue.second.absoluteFilePath());
	}

	//output.setFileTemplate(downloadsPath.absolutePath());
	// NOTE:
	// we cant rely on the file template here, since its being
	// set along with unique name on construct - and we are using a global instance
	// /////////////////////////////

    if (!output.open(QIODevice::WriteOnly)) {
		qCritical() << "Problem opening save file '" << output.fileName() << "' for download '" << url.toEncoded().constData() << "': " << output.errorString();
		downloadQueue.clear(); // finish
        startNextDownload();
		emit fail(QNetworkReply::NoError);
        return;
    }

    QNetworkRequest request(url);
    currentDownload = manager.get(request);
    connect(currentDownload, SIGNAL(downloadProgress(qint64,qint64)),
            SLOT(downloadProgress(qint64,qint64)));
    connect(currentDownload, SIGNAL(finished()),
            SLOT(downloadFinished()));
    connect(currentDownload, SIGNAL(readyRead()),
            SLOT(downloadReadyRead()));

    // prepare the output
	if (!url.fileName().endsWith(".yml"))
		qInfo() << "Downloading " << url.fileName();
    downloadTime.start();
}

void DownloadManager::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    progressBar.setStatus(bytesReceived, bytesTotal);

	if (taskbarBtn) {
		QWinTaskbarProgress* progress = taskbarBtn->progress();
		progress->setVisible(true);
		progress->setValue(bytesReceived * 100 / static_cast<double>(bytesTotal));
	}

    // calculate the download speed
    double speed = bytesReceived * 1000.0 / downloadTime.elapsed();
    QString unit;
    if (speed < 1024) {
        unit = "bytes/sec";
    } else if (speed < 1024*1024) {
        speed /= 1024;
        unit = "kB/s";
    } else {
        speed /= 1024*1024;
        unit = "MB/s";
    }

	QString finalSpeed = QString::fromLatin1("%1 %2")
                           .arg(speed, 3, 'f', 1).arg(unit);

	emit progress(bytesReceived, bytesTotal, finalSpeed);
    progressBar.setMessage(finalSpeed);
    progressBar.update();
}

void DownloadManager::downloadFinished()
{
    progressBar.clear();

    if (currentDownload->error()) {
		if (taskbarBtn) {
			taskbarBtn->progress()->setVisible(false);
		}
		// download failed
		output.remove();
		output.close();

		auto errorCode = currentDownload->error();

		qCritical() << static_cast<int>(errorCode) << "," << currentDownload->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() << currentDownload->errorString();

		currentDownload->deleteLater();
		emit fail(errorCode);
		return;
    }

	output.write(currentDownload->readAll());

	auto exists = output.exists();
	auto size = output.size();
	auto name = output.fileName();
	
	output.close();

	if (exists && size > 0) {
		emit downloaded(name);
		++downloadedCount;
	} else {
		// not that perfect, huh
		output.remove();
		qCritical() << "File size error " << size;
		emit fail(QNetworkReply::NoError);
	}
	
	currentDownload->deleteLater();
    startNextDownload();
}

void DownloadManager::downloadReadyRead()
{
	//
}