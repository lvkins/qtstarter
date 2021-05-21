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

#include "Logger.h"
 
Logger::Logger(QString fileName) {
	if (!fileName.isEmpty()) {
		file = new QFile;
		file->setFileName(fileName);
		file->open(QIODevice::Append | QIODevice::Text);
	}
}

Logger::~Logger() {
	if (file) {
		file->close();
		delete file;
		file = nullptr;
	}
}
 
void Logger::write(const QString& value) {
	QTextStream out(file);
	out.setCodec("UTF-8");
	//out.autoDetectUnicode();

	if (file) {
		out << QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss ") << value << "\n";
	}
}