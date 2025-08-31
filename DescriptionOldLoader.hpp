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

#include <set>
#include <tuple>
#include <vector>
#include <QHash>
#include <QString>

class ConstellationOldLoader;
class AsterismOldLoader;
class NamesOldLoader;
class DescriptionOldLoader
{
	QString markdown;
	QHash<QString/*locale*/, QString/*markdown*/> translatedMDs;
	QString inputDir;
	struct ImageHRef
	{
		ImageHRef(QString const& inputPath, QString const& outputPath)
			: inputPath(inputPath), outputPath(outputPath)
		{}
		QString inputPath;
		QString outputPath;
	};
	std::vector<ImageHRef> imageHRefs;
	struct DictEntry
	{
		std::set<QString> comment;
		QString english;
		QString translated;
		bool operator<(const DictEntry& other) const
		{
			return std::tie(comment,english,translated) < std::tie(other.comment,other.english,other.translated);
		}
	};
	using TranslationDict = std::vector<DictEntry>;
	QHash<QString/*locale*/, TranslationDict> translations;
	QHash<QString/*locale*/, QString/*header*/> poHeaders;
	std::set<DictEntry> allMarkdownSections;
	bool dumpMarkdown(const QString& outDir) const;
	void locateAndRelocateAllInlineImages(QString& html, bool saveToRefs);
	void addUntranslatedNames(const QString scName, const ConstellationOldLoader& consLoader, const AsterismOldLoader& astLoader, const NamesOldLoader& namesLoader);
	void loadTranslationsOfNames(const QString& poBaseDir, const QString& cultureId, const QString& englishName,
	                             const ConstellationOldLoader& consLoader, const AsterismOldLoader& astLoader, const NamesOldLoader& namesLoader);
	QString translateSection(const QString& markdown, const qsizetype bodyStartPos, const qsizetype bodyEndPos, const QString& locale, const QString& sectionName);
	QString translateDescription(const QString& markdown, const QString& locale);
public:
	void load(const QString& inDir, const QString& poBaseDir, const QString& cultureId, const QString& englishName,
	          const QString& author, const QString& credit, const QString& license,
	          const ConstellationOldLoader& consLoader, const AsterismOldLoader& astLoader, const NamesOldLoader& namesLoader,
	          bool footnotesToRefs, bool genTranslatedMD);
	bool dump(const QString& outDir) const;
};
