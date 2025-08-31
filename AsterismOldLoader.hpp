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

#include <cmath>
#include <vector>
#include <QString>

class AsterismOldLoader
{
public:
	class Asterism
	{
	public:
		struct Star
		{
			int HIP = -1;
			double RA = NAN, DE = NAN;
			bool operator==(const Star& rhs) const
			{
				if (HIP > 0) return HIP == rhs.HIP;
				return RA == rhs.RA && DE == rhs.DE;
			}
		};
		QString getTranslatorsComments() const { return translatorsComments; }
		QString getEnglishName() const { return englishName; }
	private:
		//! International name (translated using gettext)
		QString nameI18;
		//! Name in english (second column in asterism_names.eng.fab)
		QString englishName;
		//! Extracted comments
		QString translatorsComments;
		//! Abbreviation
		//! A skyculture designer must invent it. (usually 2-5 letters)
		//! This MUST be filled and be unique within a sky culture.
		QString abbreviation;
		//! Context for name
		QString context;
		//! Number of segments in the lines
		unsigned int numberOfSegments;
		//! Type of asterism
		int typeOfAsterism;
		bool flagAsterism;

		std::vector<Star> asterism;
		std::vector<int> references;

		friend class AsterismOldLoader;
	public:
		bool read(const QString& record);
	};

	void load(const QString& skyCultureDir, const QString& cultureId);
	const Asterism* find(QString const& englishName) const;
	bool dumpJSON(std::ostream& s) const;
	auto begin() const { return asterisms.cbegin(); }
	auto end() const { return asterisms.cend(); }
private:
	QString cultureId;
	bool hasAsterism = false;
	std::vector<Asterism*> asterisms;

	Asterism* findFromAbbreviation(const QString& abbrev) const;
	void loadLines(const QString& fileName);
	void loadNames(const QString& namesFile);
};
