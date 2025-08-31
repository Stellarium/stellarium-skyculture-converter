/*
 * Stellarium Sky Culture Converter
 * Copyright (C) 2025 Ruslan Kabatsayev
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#pragma once

#include <vector>
#include <iostream>
#include <QMap>
#include <QString>

class NamesOldLoader
{
public:
	struct StarName
	{
		int HIP;
		QString englishName;
		QString nativeName;
		QString pronounce;
		QString translatorsComments;
		std::vector<int> references;
	};
	struct DSOName
	{
		QString id;
		QString englishName;
		QString nativeName;
		QString pronounce;
		QString translatorsComments;
		std::vector<int> references;
	};
	struct PlanetName
	{
		QString id;
		QString english;
		QString native;
		QString translatorsComments;
	};
	void load(const QString& skyCultureDir, const QString& nativeLocale, bool convertUntranslatableNamesToNative);
	const StarName* findStar(QString const& englishName) const;
	const DSOName* findDSO(QString const& englishName) const;
	const PlanetName* findPlanet(QString const& englishName) const;
	bool dumpJSON(std::ostream& s) const;
	auto starsBegin() const { return starNames.cbegin(); }
	auto starsEnd() const { return starNames.cend(); }
	auto planetsBegin() const { return planetNames.cbegin(); }
	auto planetsEnd() const { return planetNames.cend(); }
	auto dsosBegin() const { return dsoNames.cbegin(); }
	auto dsosEnd() const { return dsoNames.cend(); }

private:
	void loadStarNames(const QString& skyCultureDir, const QString& nativeLocale, bool convertUntranslatableNamesToNative);
	void loadDSONames(const QString& skyCultureDir, const QString& nativeLocale, bool convertUntranslatableNamesToNative);
	void loadPlanetNames(const QString& skyCultureDir);
	QMap<int/*HIP*/, std::vector<StarName>> starNames;
	QMap<QString/*dsoId*/,std::vector<DSOName>> dsoNames;
	QMap<QString/*planetId*/,std::vector<PlanetName>> planetNames;
};
