#include "Utils.hpp"
#include <iomanip>
#include <iostream>
#include <QDebug>
#include <QStringList>

std::vector<int> parseReferences(const QString& inStr)
{
	if(inStr.isEmpty()) return {};
	const auto strings = inStr.split(",");
	std::vector<int> refs;
	for(const auto& string : strings)
	{
		bool ok = false;
		const auto ref = string.toInt(&ok);
		if(!ok) 
		{
			qWarning() << "Failed to parse reference number" << string << "in" << inStr;
			continue;
		}
		refs.push_back(ref);
	}
	return refs;
}

QString formatReferences(const std::vector<int>& refs)
{
	QString out;
	for(const int ref : refs)
		out += QString("%1,").arg(ref);
	if(!out.isEmpty()) out.chop(1); // remove trailing comma
	return out;
}

void warnAboutSpecialChars(const QString& s, const QString& what)
{
    std::cerr << "WARNING: special character " << what.toStdString()
              << " found in string \"" << s.toStdString() << "\"\n";
}

QString jsonEscape(const QString& string, const bool warn)
{
	QString out;
	for(const QChar c : string)
	{
		const unsigned u = uint16_t(c.unicode());
		if(u == '\\')
        {
			out += "\\\\";
            if(warn) warnAboutSpecialChars(string, "\"backslash\"");
        }
		else if(u == '\n')
        {
			out += "\\n";
            if(warn) warnAboutSpecialChars(string, "\"line break\"");
        }
		else if(u == '"')
        {
			out += "\\\"";
            if(warn) warnAboutSpecialChars(string, "\"quotation mark\"");
        }
		else if(u < 0x20)
        {
			out += QString("\\u%1").arg(u, 4, 16, QLatin1Char('0'));
            if(warn) warnAboutSpecialChars(string, QString("0x%1").arg(u, 4, 16, QLatin1Char('0')));
        }
		else
        {
			out += c;
        }
	}
	return out;
}
