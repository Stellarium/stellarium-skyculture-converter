#pragma once

#include <QString>

/// A single function interface to convert a sky culture directory into JSON and write to an output directory.
class Converter

{
public:
    /**
     * @brief Convert the sky culture from inputDir to outputDir.
     *
     * @param inputDir Path to the input directory containing the sky culture data.
     * @param outputDir Path to the output directory where the converted sky culture will be stored.
     * @param poDir Path to translations directory (optional).
     * @param nativeLocale Locale code for native constellation names (optional).
     * @param footnotesToRefs If true, converts footnotes to references.
     * @param genTranslatedMD If true, generates localized Markdown files.
     * @param convertUntranslatableNamesToNative If true, uses untranslatable names as native names.
     *
     * @return int 0 on success, non-zero on failure.
     */
    static int convert(
        const QString &inputDir,
        const QString &outputDir,
        const QString &poDir = QString(),
        const QString &nativeLocale = QString(),
        bool footnotesToRefs = false,
        bool genTranslatedMD = false,
        bool convertUntranslatableNamesToNative = false);
};
