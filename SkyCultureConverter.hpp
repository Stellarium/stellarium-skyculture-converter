#pragma once

#include <QString>

/// A single function interface to convert a sky culture directory into JSON and write to an output directory.
namespace SkyCultureConverter

{
    /**
     * @brief Convert the sky culture from inputDir to outputDir.
     *
     * @param inputDir Path to the input directory containing the sky culture data.
     * @param outputDir Path to the output directory where the converted sky culture will be stored.
     * @param poDir Optional path to translations directory.
     * @param nativeLocale Optional locale code for native constellation names.
     * @param footnotesToRefs If true, converts footnotes to references.
     * @param genTranslatedMD If true, generates localized Markdown files.
     * @param convertUntranslatableNamesToNative If true, uses untranslatable names as native names.
     *
     * @return Return code indicating the result of the operation.
     * @retval 0 Conversion completed successfully. (CONVERT_SUCCESS)
     * @retval 1 Output directory already exists. (ERR_OUTPUT_DIR_EXISTS)
     * @retval 2 info.ini was not found in the input directory. (ERR_INFO_INI_NOT_FOUND)
     * @retval 3 Failed to create the output directory. (ERR_OUTPUT_DIR_CREATION_FAILED)
     * @retval 4 Failed to write to an output file. (ERR_OUTPUT_FILE_WRITE_FAILED)
     */
    int convert(
        const QString &inputDir,
        const QString &outputDir,
        const QString &poDir = QString(),
        const QString &nativeLocale = QString(),
        bool footnotesToRefs = false,
        bool genTranslatedMD = false,
        bool convertUntranslatableNamesToNative = false);
};
