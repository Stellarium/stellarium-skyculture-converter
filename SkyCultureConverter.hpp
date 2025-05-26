#pragma once

#include <QString>
#include <QtCore/qnamespace.h>

/// A single function interface to convert a sky culture directory into JSON and write to an output directory.
namespace SkyCultureConverter
{

// Convert enum class to Q_ENUM for Qt meta-object system
// This allows the enum to be used in QML and other Qt features that require meta-information
Q_NAMESPACE
enum class ReturnValue
{
    CONVERT_SUCCESS,
    ERR_OUTPUT_DIR_EXISTS,
    ERR_INFO_INI_NOT_FOUND,
    ERR_OUTPUT_DIR_CREATION_FAILED,
    ERR_OUTPUT_FILE_WRITE_FAILED
};
Q_ENUM_NS(ReturnValue)

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
 * @return Return code indicating the result of the operation
 * @retval ReturnValue::CONVERT_SUCCESS                 - Conversion completed successfully
 * @retval ReturnValue::ERR_OUTPUT_DIR_EXISTS           -  Output directory already exists
 * @retval ReturnValue::ERR_INFO_INI_NOT_FOUND          - info.ini was not found in the input directory
 * @retval ReturnValue::ERR_OUTPUT_DIR_CREATION_FAILED  - Failed to create the output directory
 * @retval ReturnValue::ERR_OUTPUT_FILE_WRITE_FAILED    - Failed to write to an output file
 */
ReturnValue convert(
    const QString &inputDir,
    const QString &outputDir,
    const QString &poDir = QString(),
    const QString &nativeLocale = QString(),
    bool footnotesToRefs = false,
    bool genTranslatedMD = false,
    bool convertUntranslatableNamesToNative = false);

};
