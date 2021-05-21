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

#ifndef QTSTARTER_H
#define QTSTARTER_H

#include <QtWidgets/QMainWindow>
#include "ui_qtstarter.h"
#include "yaml-cpp/yaml.h"

class QtStarter : public QMainWindow
{
	Q_OBJECT

public:
	QtStarter(QWidget* parent = nullptr);
	~QtStarter();

	void showEvent(QShowEvent* _event);

	void loadYaml();
	void checkUpdate();
	
	void setConfig(const YAML::Node& cfg) {
		m_config = cfg;
	}
	void setPatchlist(const YAML::Node patchlist) {
		m_patchlist = patchlist;
	}
	YAML::Node& getConfig() {
		return m_config;
	}
	const YAML::Node& getPatchlist() const {
		return m_patchlist;
	}

	const bool needsPatcher();
	void startClient();

private slots:

private:
	Ui::QtStarterClass ui;
	YAML::Node m_config;
	YAML::Node m_patchlist;

};

#endif // QTSTARTER_H
