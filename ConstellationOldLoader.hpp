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
#include <QSize>
#include <QString>

class ConstellationOldLoader
{
public:
	struct Constellation
	{
		QString abbreviation;
		QString englishName;
		QString nativeName;
		QString pronounce;
		std::vector<int> references;
		QString translatorsComments;
		QString artTexture;
		QSize textureSize;
		std::vector<int> points;
		struct Point
		{
			int hip;
			int x, y;
		} artP1, artP2, artP3;
		int beginSeason = 1;
		int endSeason = 12;
		bool seasonalRuleEnabled = false;

		bool read(QString const& record);
	};
private:
	QString skyCultureName;
	std::vector<Constellation> constellations;
	struct RaDec
	{
		double ra, dec;
	};
	struct BoundaryLine
	{
		std::vector<RaDec> points;
		QString cons1, cons2;
	};
	std::vector<BoundaryLine> boundaries;
	std::string boundariesType;

	Constellation* findFromAbbreviation(const QString& abbrev);
	void loadLinesAndArt(const QString &skyCultureDir, const QString& outDir);
	void loadBoundaries(const QString& skyCultureDir);
	void loadNames(const QString &skyCultureDir);
    void loadNativeNames(const QString& skyCultureDir, const QString& nativeLocale);
	void loadSeasonalRules(const QString& rulesFile);
	bool dumpBoundariesJSON(std::ostream& s) const;
	bool dumpConstellationsJSON(std::ostream& s) const;
public:
	void load(const QString &skyCultureDir, const QString& outDir, const QString& nativeLocale);
	const Constellation* find(QString const& englishName) const;
	bool dumpJSON(std::ostream& s) const;
	bool hasBoundaries() const { return !boundaries.empty(); }
	void setBoundariesType(std::string const& type) { boundariesType = type; }
	auto begin() const { return constellations.cbegin(); }
	auto end() const { return constellations.cend(); }
};
