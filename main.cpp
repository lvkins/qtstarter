/**
 * Copyright (C) £ukasz Szwedt
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

//#pragma comment (lib,"Advapi32.lib")
#include "stdafx.h"
#include "qtstarter.h"

#include <QApplication>
#include <QFileInfo>
#include <QTextCodec>
#include <QLoggingCategory>
#include <QTemporaryFile>

#include "downloadmanager.h"
#include "logger.h"
#include "utils.h"

#ifdef _RELSTATIC
#include <QtPlugin>
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);
#endif

#ifdef _DEBUG
#include "io.h"
#include <fcntl.h>
#include "qt_windows.h"
#endif

DownloadManager g_downloader;
static Logger g_logger(SET_PATH("patcher/starter.log"));

bool g_startedByPatcher = false;

void mainMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString& msg)
{
	// suppress https://bugreports.qt.io/browse/QTIFW-822
	// consider upgrading to qt>=5.6 which will require VS2013
	if (msg.contains("::stateChanged")) {
		return;
	}

	QString __n;

    switch (type) {
		case QtDebugMsg:	__n = "DEBUG";
#ifndef _DEBUG
			return;
#endif
		break;
		case QtInfoMsg:		__n = "INFO";		break;
		case QtWarningMsg:	__n = "WARNING";	break;
		case QtCriticalMsg: __n = "CRITICAL";	break;
		case QtFatalMsg:	__n = "FATAL";		break;
    }

	std::ostringstream oss;
	oss << "[" << __n.toStdString() << "]: " << msg.toStdString();

	if (type == QtDebugMsg || type == QtFatalMsg) {
		oss << " (" << context.file << ":" << context.line << " - " << context.function << ")";
	}

	g_logger.write(QString::fromStdString(oss.str()));
	std::cout << oss.str() << std::endl;

	if (type == QtFatalMsg) {
		abort();
	}
}

class Application final : public QApplication {
public:
    Application(int& argc, char** argv) : QApplication(argc, argv) {}
    virtual bool notify(QObject *receiver, QEvent *event) override {
		try {
			return QApplication::notify(receiver, event);
		} catch (std::exception &e) {
			FATAL(QStringLiteral("Wyst¹pi³ nieobs³ugiwany wyj¹tek!\n\n%1").arg(e.what()))
			qFatal("XError %s sending event %s to object %s (%s)", 
				e.what(), typeid(*event).name(), qPrintable(receiver->objectName()),
				typeid(*receiver).name());
		} catch (...) {
			qFatal("DError <unknown> sending event %s to object %s (%s)", 
				typeid(*event).name(), qPrintable(receiver->objectName()),
				typeid(*receiver).name());
		}        

		return false;
    }
};
/*
#undef assert
#define assert(x) QMessageBox::critical(nullptr,"Error", "Assert");
#define Q_ASSERT(cond) ((!(cond)) ? qt_assert(#cond,__FILE__,__LINE__) : qt_noop())*/

int main(int argc, char* argv[])
{
	// Suppress ssl warnings
	QLoggingCategory::setFilterRules("qt.network.ssl.warning=false");

	qInstallMessageHandler(mainMessageOutput);

    Application app(argc, argv);

#ifdef _DEBUG
	AllocConsole();
	int hCrt = _open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);
	FILE* hf = _fdopen(hCrt, "w");
	*stdout = *hf;
	*stderr = *hf;
#endif

	qInfo() << "--- Boot ---";

	if (!isStarterPath()) {
		FATAL("Klient nie istnieje.");
		exit(0);
	}

	QtStarter w;
	g_startedByPatcher = QApplication::arguments().size() == 3 &&
						 QApplication::arguments().at(1) == "patchDone";
	w.loadYaml();

    app.exec();
}
