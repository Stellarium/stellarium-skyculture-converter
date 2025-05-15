#include <iostream>
#include <QString>
#include <vector>
#include <string>
#include <QCoreApplication>
#include "Converter.hpp"
#include "Utils.hpp"

int usage(const char* argv0, const int ret)
{
	auto& out = ret ? std::cerr : std::cout;
	out << "Usage: " << argv0 << " [options...] skyCultureDir outputDir [skyCulturePoDir]\n"
	    << "Options:\n"
	    << "  --footnotes-to-references  Try to convert footnotes to references\n"
	    << "  --untrans-names-are-native Record untranslatable star/DSO names as native names\n"
	    << "  --native-locale LOCALE     Use *_names.LOCALE.fab as a source for \"native\" constellation names (the\n"
	       "                             middle column in *_names.eng.fab will be moved to the \"pronounce\" entry.\n"
	    << "  --translated-md            Generate localized Markdown files (for checking translations)\n";
	return ret;
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QString inDir, outDir, poDir, nativeLocale;
    bool footnotesToRefs = false, genTranslatedMD = false, convertUntranslatableNamesToNative = false;
    // parse arguments
    std::vector<std::string> args(argv+1, argv+argc);
    for(const auto& arg : args) {
        if(!arg.empty() && arg[0] != '-') {
            if(inDir.isEmpty()) inDir = arg.c_str();
            else if(outDir.isEmpty()) outDir = arg.c_str();
            else if(poDir.isEmpty()) poDir = arg.c_str();
            else return usage(argv[0], 1);
        } else if(arg == "--translated-md") genTranslatedMD = true;
        else if(arg == "--footnotes-to-references") footnotesToRefs = true;
        else if(arg == "--untrans-names-are-native") convertUntranslatableNamesToNative = true;
        else if(arg == "--native-locale") {
            static bool nextIsLocale = false;
            nextIsLocale = !nextIsLocale;
            if(nextIsLocale) continue;
            nativeLocale = arg.c_str();
        } else if(arg == "--help" || arg == "-h") {
            return usage(argv[0], 0);
        } else return usage(argv[0], 1);
    }
    // invoke converter
    return Converter::convert(inDir, outDir, poDir, nativeLocale,
                              footnotesToRefs, genTranslatedMD,
                              convertUntranslatableNamesToNative);
}
