/*
 * Stellarium Sky Culture Converter
 * Copyright (C) 2025 Mher Mnatsakanyan
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

#include "SkyCultureConverter.hpp"
#include "Utils.hpp"
#include "NamesOldLoader.hpp"
#include "AsterismOldLoader.hpp"
#include "DescriptionOldLoader.hpp"
#include "ConstellationOldLoader.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QRegularExpression>
#include <fstream>
#include <iostream>
#include <sstream>

namespace
{

QString convertLicense(const QString &license)
{
    auto parts = license.split("+");
    for (auto &p : parts)
        p = p.simplified();
    for (auto &lic : parts)
    {
        if (lic.startsWith("Free Art "))
            continue;

        lic.replace(QRegularExpression("(?: International)?(?: Publice?)? License"), "");
    }

    if (parts.size() == 1)
        return parts[0];
    if (parts.size() == 2)
    {
        if (parts[1].startsWith("Free Art ") && !parts[0].startsWith("Free Art "))
            return "Text and data: " + parts[0] + "\n\nIllustrations: " + parts[1];
        else if (parts[0].startsWith("Free Art ") && !parts[1].startsWith("Free Art "))
            return "Text and data: " + parts[1] + "\n\nIllustrations: " + parts[0];
    }
    std::cerr << "SkyCultureConverter::\tUnexpected combination of licenses, leaving them unformatted.\n";
    return license;
}

void convertInfoIni(const QString &dir, std::ostream &s, QString &boundariesType, QString &author, QString &credit, QString &license, QString &cultureId, QString &region, QString &englishName)
{
    QSettings pd(dir + "/info.ini", QSettings::IniFormat); // FIXME: do we really need StelIniFormat here instead?
    englishName = pd.value("info/name").toString();
    author = pd.value("info/author").toString();
    credit = pd.value("info/credit").toString();
    license = pd.value("info/license", "").toString();
    region = pd.value("info/region", "???").toString();
    const auto classification = pd.value("info/classification").toString();
    boundariesType = pd.value("info/boundaries", "none").toString();

    cultureId = QFileInfo(dir).fileName();

    // Now emit the JSON snippet
    s << "{\n"
            "  \"id\": \"" +
                cultureId.toStdString() + "\",\n"
                                        "  \"region\": \"" +
                region.toStdString() + "\",\n"
                                    "  \"classification\": [\"" +
                classification.toStdString() + "\"],\n"
                                            "  \"fallback_to_international_names\": false,\n";
}

void writeEnding(std::string &s)
{
    if (s.size() > 2 && s.substr(s.size() - 2) == ",\n")
        s.resize(s.size() - 2);
    s += "\n}\n";
}

}

namespace SkyCultureConverter
{

ReturnValue convert(
    const QString &inputDir,
    const QString &outputDir,
    const QString &poDir,
    const QString &nativeLocale,
    bool footnotesToRefs,
    bool genTranslatedMD,
    bool convertUntranslatableNamesToNative)
{
    // Ensure output does not already exist
    if (QFile(outputDir).exists())
    {
        std::cerr << "SkyCultureConverter::\tOutput directory already exists, won't touch it.\n";
        return ReturnValue::ERR_OUTPUT_DIR_EXISTS;
    }
    // Check for info.ini in input
    if (!QFile(inputDir + "/info.ini").exists())
    {
        std::cerr << "SkyCultureConverter::\tError: info.ini file wasn't found\n";
        return ReturnValue::ERR_INFO_INI_NOT_FOUND;
    }

    // Normalize input path
    QString inDir = QDir::fromNativeSeparators(inputDir);
    while (inDir.endsWith("/"))
        inDir.chop(1);

    // Read basic info
    std::stringstream out;
    QString boundariesType, author, credit, license, cultureId, region, englishName;
    convertInfoIni(inDir, out, boundariesType, author, credit, license,
                    cultureId, region, englishName);

    // Load data
    AsterismOldLoader aLoader;
    aLoader.load(inDir, cultureId);

    ConstellationOldLoader cLoader;
    cLoader.setBoundariesType(boundariesType.toStdString());
    cLoader.load(inDir, outputDir, nativeLocale);

    NamesOldLoader nLoader;
    nLoader.load(inDir, nativeLocale, convertUntranslatableNamesToNative);

    aLoader.dumpJSON(out);
    cLoader.dumpJSON(out);
    nLoader.dumpJSON(out);

    // Finalize and write JSON
    auto str = std::move(out).str();
    writeEnding(str);

    if (!QDir().mkpath(outputDir))
    {
        std::cerr << "SkyCultureConverter::\tFailed to create output directory\n";
        return ReturnValue::ERR_OUTPUT_DIR_CREATION_FAILED;
    }
    {
        std::ofstream outFile((outputDir + "/index.json").toStdString());
        outFile << str;
        outFile.flush();
        if (!outFile)
        {
            std::cerr << "SkyCultureConverter::\tFailed to write index.json\n";
            return ReturnValue::ERR_OUTPUT_FILE_WRITE_FAILED;
        }
    }

    // Description loader
    DescriptionOldLoader dLoader;
    license = convertLicense(license);
    dLoader.load(inDir, poDir, cultureId, englishName,
                    author, credit, license,
                    cLoader, aLoader, nLoader,
                    footnotesToRefs, genTranslatedMD);
    dLoader.dump(outputDir);

    return ReturnValue::CONVERT_SUCCESS;
}

}