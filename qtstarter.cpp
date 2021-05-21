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
#include "qtstarter.h"
#include "downloadmanager.h"
#include "utils.h"

#include <fstream>
#include <QProcess>
#include <QCryptographicHash>

extern DownloadManager g_downloader;
extern bool g_startedByPatcher;

QtStarter::QtStarter(QWidget* parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	setWindowTitle(_SERVER_NAME);

	// Disable window resizing
	setFixedSize(size());
	//	statusBar()->setSizeGripEnabled(false);
}

QtStarter::~QtStarter() { }

void QtStarter::showEvent(QShowEvent* _event) {
    QWidget::showEvent(_event);
    g_downloader.setupTaskbarProgress(this);
} 

void QtStarter::loadYaml() {
	// Below exception will handle that
	//if (!fileExists(SET_PATH("patcher/config.yml"))) {
	//	FATAL("Plik konfiguracyjny nie istnieje! Upewnij się, że masz poprawny klient. Zawsze mozesz ściagnąć pełnego klienta z naszej strony na " + SERVER_URL + ".");
	//}

	qInfo() << "Loading configuration file...";

	YAML::Node config;

	try {
		// Load local configuration & test it
		config = YAML::LoadFile(SET_PATH("patcher/config.yml").toStdString());

		if (config.IsNull()) {
			throw YAML::BadFile();
		}

		if (!config["url"])
			throw YAML::BadFile();
	} catch(YAML::Exception) {
		// Failed loading local configuration file - its mostly likely damaged. Try to fix the file...
		std::string failCfg = "url: http://files." + std::string(_SERVER_URL) + "/patcher/\nversion: {client: 0, patcher: 1}";

		std::ofstream fout(SET_PATH("patcher/config.yml").toStdString());
		fout << failCfg;
		fout.close();

		qCritical() << "Fixing local configuration...";
		QERROR("Wystąpił błąd podczas ładowania pliku konfiguracyjnego.\nUruchom klient ponownie.");//\n" + QString(typeid(e).name()));
	}

	qInfo() << "Loaded...";

	setConfig(config);

	qInfo() << "Loading patchlist...";

	QDir tmpDir(SET_PATH("patcher/tmp"));
	if (!tmpDir.exists() && !tmpDir.mkdir(".")) {
		FATAL("Niepowodzenie podczas tworzenia tymczasowego katalogu.");
	}

	std::string str = config["url"].as<std::string>();
	g_downloader.append(QUrl(QString("%1patchlist.yml").arg(str.c_str())));

	QObject::connect(&g_downloader, &DownloadManager::downloaded, [=](const QString& fileName) {
		QObject::disconnect(&g_downloader, &DownloadManager::downloaded, nullptr, nullptr);

		YAML::Node patchlist;

		try {
			patchlist = YAML::LoadFile(fileName.toStdString());

			if (patchlist.IsNull()) {
				throw YAML::BadFile();
			}

			if (!patchlist["base"]) {
				throw YAML::BadFile();
			}
		} catch(YAML::Exception& e) {
			qCritical() << "Patchlist is invalid!";
			FATAL("#010 - Serwer może być w tej chwili nieosiągalny. Spróbuj ponownie później.\n" + QString(e.what()));
		}

		QFile::remove(fileName);
		setPatchlist(patchlist);

		qInfo() << "Loaded...";

		checkUpdate();
	});

	QObject::connect(&g_downloader, &DownloadManager::fail, [=](const QNetworkReply::NetworkError errCode) {
		FATAL("Brak dostępu do serwera. [#" + QString::number(errCode) + "].\nMoże być to spowodowane brakiem dostępu do internetu lub chwilową przerwą w działaniu serwera - sprawdź połączenie z internetem lub spróbuj ponownie za chwilę.");
	});
}

void QtStarter::checkUpdate() {
	YAML::Node& config = getConfig();
	const YAML::Node& patchList = getPatchlist();

	if (patchList["version"]["patcher"].as<int>() == config["version"]["patcher"].as<int>() &&
		QFile::exists(SET_PATH("patcher/patcher.exe"))) {
		qInfo() << "Version OK";
		startClient();
		return;
	}

	qInfo() << "Patcher update! (" << patchList["version"]["patcher"].as<int>() << ":" << config["version"]["patcher"].as<int>() << ")";

	show();

	g_downloader.append(QUrl(QString::fromStdString(patchList["base"].as<std::string>()) + "/patcher_new.exe"));

	// onProgress
	QObject::connect(&g_downloader, &DownloadManager::progress,
									[=](qint64 bytesReceived, qint64 bytesTotal, const QString& speed) {
		double percent = bytesReceived * 100 / static_cast<double>(bytesTotal);

		findChild<QProgressBar*>("progressBar")->setValue(percent);
		findChild<QLabel*>("updateLabel")->setText(QString("Aktualizacja... %1% (%2)")
			.arg(QString::number(percent, '.', 2), speed));
	});

	// onDownload
	QObject::connect(&g_downloader, &DownloadManager::downloaded, this, [=, &config] (const QString& downloadedFile) {
		findChild<QLabel*>("updateLabel")->setText("Proszę czekać...");

		QFile patchBin(SET_PATH("patcher/patcher.exe"));
		if (patchBin.exists() && !patchBin.remove()) {
			qCritical() << "Couldn't remove existing patcher.";
			FATAL("Niepowodzenie w aktualizacji patchera.");
		}

		QDir d;
		if (!d.rename(downloadedFile, d.absoluteFilePath(SET_PATH("patcher/patcher.exe")))) {
			qCritical() << "Cannot move updated file: " << d.absoluteFilePath(SET_PATH("patcher/patcher.exe"));
			FATAL("Niepowodzenie w aktualizacji patchera.");
		}

		config["version"]["patcher"] = patchList["version"]["patcher"];
		std::ofstream fout(SET_PATH("patcher/config.yml").toStdString());
		fout << config;
		fout.close();

		startClient();
	});
}

const bool QtStarter::needsPatcher() {
	const auto& config = getConfig();
	const auto& patchList = getPatchlist();

	if (config["version"]["client"].as<int>() != patchList["version"]["client"].as<int>()) {
		qInfo() << "Update available.";
		return true;
	}

	// Unnecessary files cleanup
	
	size_t removedFiles = 0;

	QDir dirMain(MAIN_DIR);
	dirMain.setNameFilters(QStringList() << "*.exe" << "*.dll" << "*.mix" << "*.flt");
	dirMain.setFilter(QDir::Files);

	QDir dirPack(SET_PATH("pack"));
	dirPack.setFilter(QDir::Files);
	dirPack.setSorting(QDir::Size | QDir::Reversed);

	QCryptographicHash hash(QCryptographicHash::Md5);

	foreach(const auto& fileInfo, dirMain.entryInfoList() + dirPack.entryInfoList()) {
		auto it = std::find_if(patchList["files"].begin(), patchList["files"].end(), [&fileInfo] (const YAML::Node& n) { 
			return n["path"].as<std::string>() == fileInfo.filePath().toStdString();//fileInfo.filePath().replace(MAIN_DIR, "").toStdString();
		});

		if (it == patchList["files"].end()) {
			const bool removed = QFile::remove(fileInfo.filePath());
			qInfo() << fileInfo.filePath() << " marked unnecessary... " << (removed ? "removed" : "cannot remove");
			++removedFiles;
		}

		if (g_startedByPatcher && fileInfo.path() == SET_PATH("pack")) {
			// Also prepare hash if it's about to start the client
			// qInfo() << fileInfo.fileName() << QString(", ") << QString::number(fileInfo.size()) << ", " << QString::number(fileInfo.lastModified().toMSecsSinceEpoch() / 1000);
			hash.addData(QString(fileInfo.fileName() + QString::number(fileInfo.size()) +
						 QString::number(fileInfo.lastModified().toMSecsSinceEpoch() / 1000)).toLocal8Bit());
		}
	}

	if (removedFiles > 0)
		qInfo() << "Removed " << removedFiles << " files.";

	// Check for missing files

	foreach(const YAML::Node& n, patchList["files"]) {
		if (n["type"].as<std::string>() == "NEW" && !QFile::exists(QString::fromStdString(n["path"].as<std::string>()))) {
			qInfo() << "File is missing: " << n["path"].as<std::string>().c_str();
			return true;
		}
	}
	
	// Check whether initial pack hash's matching current (if starter is begin run from patcher instance)

	if (g_startedByPatcher && (hash.result().toHex() != QApplication::arguments().at(2))) {
		qCritical() << "Security abort. (" << hash.result().toHex() << ":" << QApplication::arguments().at(2) << ")";
		return true;
	}
	
	return false;
}

/**

Odnosnie ladowania DLL - aby uniknac przypadku kiedy musimy dostarczyc takie same biblioteki dll na 2 aplikacji exe w innych folderach
wg. microsoftu, jesli juz uruchomiona aplikacja korzysta z danych bibliotek to system nie szuka ich tylko bierze je z pamieci...
Wiec wypadaloby najpierw uruchomic starter->aplikacja->:jesli uruchomiona->zamykamy starter
(If a DLL with the same module name is already loaded in memory, the system checks only for redirection and a manifest before resolving to the loaded DLL, no matter which directory it is in. The system does not search for the DLL.)
https://msdn.microsoft.com/en-us/library/windows/desktop/ms682586(v=vs.85).aspx
**/

void QtStarter::startClient() {
	const static bool ALWAYS_RUN_PATCHER=false;

	if (ALWAYS_RUN_PATCHER && !g_startedByPatcher || needsPatcher()) {
		qInfo() << "I need patcher! [SBP: " << (int)g_startedByPatcher << "]";

		if (!QProcess::startDetached(SET_PATH("patcher/patcher.exe"), QStringList(), SET_PATH("patcher"))) {
			qCritical() << "Patcher failed to start. Aborting.";
			FATAL("[P] Odmowa dostępu.\nCzy uruchamiasz aplikację jako administrator? Jeśli nie - zrób to.\n\nPamiętaj także, aby dodać cały folder klienta do wyjątków w swoim antywirusie!")
		}

		qApp->quit();
		return;
	}

	qInfo() << "Launching client...";

	if (!QProcess::startDetached(SET_PATH(CLIENT_NAME), QStringList(), MAIN_DIR)) {
		qCritical() << "Client failed to start. Aborting.";
		FATAL("[C] Odmowa dostępu.\nCzy uruchamiasz aplikację jako administrator? Jeśli nie - zrób to.\n\nPamiętaj także, aby dodać cały folder klienta do wyjątków w swoim antywirusie!")
	}

	qInfo() << "Client has been launched... Good bye";
	qApp->quit();
}