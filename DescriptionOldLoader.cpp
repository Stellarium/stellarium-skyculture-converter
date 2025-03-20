#include "DescriptionOldLoader.hpp"

#include <set>
#include <map>
#include <deque>
#include <cctype>
#include <string_view>
#include <unordered_map>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QFileInfo>
#include <QTextStream>
#include <QDomDocument>
#include <QRegularExpression>
#include <gettext-po.h>
#include <tidy.h>
#include <tidybuffio.h>
#include "NamesOldLoader.hpp"
#include "AsterismOldLoader.hpp"
#include "ConstellationOldLoader.hpp"

namespace
{

QString stripComments(QString markdown)
{
	static const QRegularExpression commentPat("<!--.*?-->");
	markdown.replace(commentPat, "");
	return markdown;
}

QString join(const std::set<QString>& strings)
{
	QString out;
	for(const auto& str : strings)
	{
		if(out.isEmpty())
			out = str;
		else if(out.endsWith('\n'))
			out += str;
		else
			out += "\n" + str;
	}
	return out;
}

#if (QT_VERSION>=QT_VERSION_CHECK(5, 14, 0))
constexpr auto SkipEmptyParts = Qt::SkipEmptyParts;
#else
constexpr auto SkipEmptyParts = QString::SkipEmptyParts;
#endif

const QRegularExpression htmlSimpleImageRegex(R"reg(<img\b[^>/]*(?:\s+alt="([^"]+)")?\s+src="([^"]+)"(?:\s+alt="([^"]+)")?\s*/?>)reg");
const QRegularExpression htmlGeneralImageRegex(R"reg(<img\b[^>/]*\s+src="([^"]+)"[^>/]*/?>)reg");

QString readReferencesFile(const QString& inDir)
{
	const auto path = inDir + "/reference.fab";
	if (!QFileInfo(path).exists())
	{
		qWarning() << "No reference file, assuming the references are in the description text.";
		return "";
	}
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "WARNING - could not open" << QDir::toNativeSeparators(path);
		return "";
	}
	QString record;
	// Allow empty and comment lines where first char (after optional blanks) is #
	static const QRegularExpression commentRx("^(\\s*#.*|\\s*)$");
	QString reference = "## References\n\n";
	int totalRecords=0;
	int readOk=0;
	int lineNumber=0;
	while(!file.atEnd())
	{
		record = QString::fromUtf8(file.readLine()).trimmed();
		lineNumber++;
		if (commentRx.match(record).hasMatch())
			continue;

		totalRecords++;
		static const QRegularExpression refRx("\\|");
		#if (QT_VERSION>=QT_VERSION_CHECK(5, 14, 0))
		QStringList ref = record.split(refRx, Qt::KeepEmptyParts);
		#else
		QStringList ref = record.split(refRx, QString::KeepEmptyParts);
		#endif
		// 1 - URID; 2 - Reference; 3 - URL (optional)
		if (ref.count()<2)
			qWarning() << "Error: cannot parse record at line" << lineNumber << "in references file" << QDir::toNativeSeparators(path);
		else if (ref.count()<3)
		{
			qWarning() << "Warning: record at line" << lineNumber << "in references file"
			           << QDir::toNativeSeparators(path) << " has wrong format (RefID: " << ref.at(0) << ")! Let's use fallback mode...";
			reference.append(QString(" - [#%1]: %2\n").arg(ref[0], ref[1]));
			readOk++;
		}
		else
		{
			if (ref.at(2).isEmpty())
				reference.append(QString(" - [#%1]: %2\n").arg(ref[0], ref[1]));
			else
				reference.append(QString(" - [#%1]: [%2](%3)\n").arg(ref[0], ref[1], ref[2]));
			readOk++;
		}
	}
	if(readOk != totalRecords)
		qDebug() << "Loaded" << readOk << "/" << totalRecords << "references";

	return reference;
}

void cleanupWhitespace(QString& markdown)
{
	// Clean too long chains of newlines
	markdown.replace(QRegularExpression("\n[ \t]*\n[ \t]*\n+"), "\n\n");
	// Same for such chains inside blockquotes
	markdown.replace(QRegularExpression("\n>[ \t]*(?:\n>[ \t]*)+\n"), "\n>\n");

	// Remove trailing spaces
	markdown.replace(QRegularExpression("[ \t]+\n"), "\n");

	// Make unordered lists a bit denser
	const QRegularExpression ulistSpaceListPattern("(\n -[^\n]+)\n+(\n \\-)");
	//  1. Remove space between odd and even entries
	markdown.replace(ulistSpaceListPattern, "\\1\\2");
	//  2. Remove space between even and odd entries (same replacement rule)
	markdown.replace(ulistSpaceListPattern, "\\1\\2");

	// Make ordered lists a bit denser
	const QRegularExpression olistSpaceListPattern("(\n 1\\.[^\n]+)\n+(\n 1)");
	//  1. Remove space between odd and even entries
	markdown.replace(olistSpaceListPattern, "\\1\\2");
	//  2. Remove space between even and odd entries (same replacement rule)
	markdown.replace(olistSpaceListPattern, "\\1\\2");

	const bool startsWithList = markdown.startsWith(" 1. ") || markdown.startsWith(" - ");
	markdown = (startsWithList ? " " : "") + markdown.trimmed() + "\n";
}

QString tidyHTML(const QString& html)
{
	TidyDoc tdoc = tidyCreate();
	TidyBuffer output = {};
	TidyBuffer errbuf = {};
	int rc = -1;
	if(tidyOptSetBool(tdoc, TidyXhtmlOut, yes))
		rc = tidySetErrorBuffer(tdoc, &errbuf);
	if(rc >= 0)
		rc = tidyParseString(tdoc, html.toStdString().c_str());
	if(rc > 0)
	{
		// QDomNode misbehaves with entities in HTML5 mode, so set XHTML 4.1 Transitional
		rc = tidyOptSetValue(tdoc, TidyDoctype, "loose") ? rc : -1;
	}
	if(rc > 0)
		rc = tidyOptSetInt(tdoc, TidyWrapLen, 999999) ? rc : -1;
	if(rc >= 0)
		rc = tidyCleanAndRepair(tdoc);
	if(rc > 1)
		rc = tidyOptSetBool(tdoc, TidyForceOutput, yes) ? rc : -1;
	if(rc >= 0)
		rc = tidySaveBuffer(tdoc, &output);

	QString out;
	if(rc >= 0)
		out = reinterpret_cast<const char*>(output.bp);
	else
		std::cerr << "ERROR: Failed to parse HTML with HTML Tidy:\n" << errbuf.bp << "\n";

	tidyBufFree(&output);
	tidyBufFree(&errbuf);
	tidyRelease(tdoc);

	return out;
}

QString nodeTypeName(const QDomNode::NodeType type)
{
	switch(type)
	{
	case QDomNode::ElementNode: return "ElementNode";
	case QDomNode::AttributeNode: return "AttributeNode";
	case QDomNode::TextNode: return "TextNode";
	case QDomNode::CDATASectionNode: return "CDATASectionNode";
	case QDomNode::EntityReferenceNode: return "EntityReferenceNode";
	case QDomNode::EntityNode: return "EntityNode";
	case QDomNode::ProcessingInstructionNode: return "ProcessingInstructionNode";
	case QDomNode::CommentNode: return "CommentNode";
	case QDomNode::DocumentNode: return "DocumentNode";
	case QDomNode::DocumentTypeNode: return "DocumentTypeNode";
	case QDomNode::DocumentFragmentNode: return "DocumentFragmentNode";
	case QDomNode::NotationNode: return "NotationNode";
	case QDomNode::BaseNode: return "BaseNode";
	case QDomNode::CharacterDataNode: return "CharacterDataNode";
	}
	return QString::number(type);
}

void formatImgInHTML(const QDomElement& n, QString& html)
{
	html += "<img";
	if(n.hasAttribute("width"))
		html += " width=\"" + n.attribute("width") + "\"";
	if(n.hasAttribute("height"))
		html += " height=\"" + n.attribute("height") + "\"";
	if(n.hasAttribute("src"))
		html += " src=\"" + n.attribute("src") + "\"";
	if(n.hasAttribute("alt"))
		html += " alt=\"" + n.attribute("alt") + "\"";
	html += "/>";
}

void addNewlineBeforeNodeIfNeeded(QString& markdown)
{
	if(not markdown.isEmpty() && not markdown[markdown.size() - 1].isSpace())
		markdown += '\n';
}

void formatSectionAsHTML(const QDomElement& secNode, QString& html)
{
	QString sec;
	QTextStream s(&sec);
	secNode.save(s, 1);
	addNewlineBeforeNodeIfNeeded(html);
	html += sec;
}

void formatListAsHTML(const QDomElement& listNode, QString& html)
{
	return formatSectionAsHTML(listNode, html);
}

bool processHTMLNode(const QDomNode& parentNode, bool insideTable, bool footnotesToRefs, bool& h1emitted, QString& markdown);
bool processTableRow(const QDomElement& rowNode, const bool firstRow, QString& markdown)
{
	std::vector<QString> columns;
	bool isHeader = false;
	for(auto n = rowNode.firstChild(); !n.isNull(); n = n.nextSibling())
	{
		switch(n.nodeType())
		{
		case QDomNode::ElementNode:
		{
			const auto el = n.toElement();
			const auto tagName = el.tagName().toLower();
			if(tagName == "td" || tagName == "th")
			{
				if(el.hasAttribute("colspan") || el.hasAttribute("rowspan"))
				{
					qWarning().nospace() << "Table colspan and rowspan are not supported in Markdown. Leaving the table in HTML format.";
					return false;
				}

				if(tagName == "th") isHeader = true;

				columns.push_back("");
				bool h1emitted = true;
				if(!processHTMLNode(el, true, false, h1emitted, columns.back()))
				{
					std::cerr << " in a table. Leaving the table in HTML format.\n";
					return false;
				}
			}
			else
			{
				qWarning().nospace() << "Unexpected tag inside <tr>: " << tagName
				                     << ". Leaving the table in HTML format.";
				return false;
			}
			break;
		}
		default:
			qWarning().nospace() << "Unexpected HTML node " << n.nodeType() << " in a table row. Leaving the table in HTML format.";
			return false;
		}
	}

	if(firstRow && !isHeader)
	{
		// Create an empty header
		markdown += "|";
		for(const auto& column : columns)
		{
			markdown += QString(column.size() + 2, QLatin1Char(' '));
			markdown += "|";
		}
		markdown += "\n|";
		for(const auto& column : columns)
		{
			markdown += QString(column.size() + 2, QLatin1Char('-'));
			markdown += "|";
		}
		markdown += "\n";
	}

	markdown += "|";
	for(const auto& column : columns)
	{
		markdown += " ";
		markdown += column;
		markdown += " |";
	}
	markdown += "\n";

	if(firstRow && isHeader)
	{
		markdown += "|";
		for(const auto& column : columns)
		{
			markdown += QString(column.size() + 2, QLatin1Char('-'));
			markdown += "|";
		}
		markdown += "\n";
	}
	return true;
}

void processTable(const QDomElement& tableNode, QString& markdown)
{
	// Let's try formatting it until we find an item that we can't have in a Markdown table
	QString table;
	bool processingFirstRow = true;
	for(auto n = tableNode.firstChild(); !n.isNull(); n = n.nextSibling())
	{
		switch(n.nodeType())
		{
		case QDomNode::ElementNode:
		{
			const auto el = n.toElement();
			const auto tagName = el.tagName().toLower();
			if(tagName == "tr")
			{
				if(el.hasAttribute("colspan") || el.hasAttribute("rowspan"))
				{
					qWarning().nospace() << "Table colspan and rowspan are not supported in Markdown. Leaving the table in HTML format.";
					formatSectionAsHTML(tableNode, markdown);
					return;
				}
				if(!processTableRow(el, processingFirstRow, table))
				{
					formatSectionAsHTML(tableNode, markdown);
					return;
				}
				processingFirstRow = false;
			}
			else
			{
				qWarning().nospace() << "Unexpected tag inside <table>: " << tagName << ". Leaving the table in HTML format.\n";
				formatSectionAsHTML(tableNode, markdown);
				return;
			}
			break;
		}
		default:
			qWarning().nospace() << "Unexpected HTML node " << n.nodeType() << " in a table. Leaving the table in HTML format.";
			formatSectionAsHTML(tableNode, markdown);
			return;
		}
	}
	markdown += table;
}

void processList(const QDomElement& listNode, QString& markdown)
{
	std::vector<QString> items;
	for(auto n = listNode.firstChild(); !n.isNull(); n = n.nextSibling())
	{
		switch(n.nodeType())
		{
		case QDomNode::ElementNode:
		{
			const auto el = n.toElement();
			const auto tagName = el.tagName().toLower();
			if(tagName == "li")
			{
				items.push_back("");
				bool h1emitted = true;
				if(!processHTMLNode(el, true, false, h1emitted, items.back()))
				{
					std::cerr << " in a list. Leaving the list in HTML format.\n";
					formatListAsHTML(listNode, markdown);
					return;
				}
			}
			else
			{
				qWarning().nospace() << "Unexpected tag inside a list: " << tagName
				                     << ". Leaving the list in HTML format.\n";
				formatListAsHTML(listNode, markdown);
				return;
			}
			break;
		}
		default:
			qWarning().nospace() << "Unexpected HTML node " << n.nodeType() << " in a list. Leaving the list in HTML format.";
			formatListAsHTML(listNode, markdown);
			return;
		}
	}

	const bool ordered = listNode.tagName() == "ol";
	if(not markdown.isEmpty() && markdown[markdown.size() - 1] != '\n')
		markdown += '\n';
	for(unsigned i = 0; i < items.size(); ++i)
	{
		if(ordered)
			markdown += QString(" %1. ").arg(i+1);
		else
			markdown += " - ";
		markdown += items[i];
		markdown += '\n';
	}
}

bool processHTMLNode(const QDomNode& parentNode, const bool insideTable, const bool footnotesToRefs, bool& h1emitted, QString& markdown)
{
	for(auto n = parentNode.firstChild(); !n.isNull(); n = n.nextSibling())
	{
		switch(n.nodeType())
		{
		case QDomNode::ElementNode:
		{
			const auto el = n.toElement();
			const auto tagName = el.tagName().toLower();
			if(tagName == "h1")
			{
				if(insideTable)
				{
					std::cerr << "WARNING: Unexpected <h1> tag";
					return false;
				}

				if(h1emitted)
				{
					std::cerr << "WARNING: Unexpected repeated <h1> tag. Demoting it to <h3>.\n";
					markdown += "\n### ";
					QString text;
					processHTMLNode(n, insideTable, footnotesToRefs, h1emitted, text);
					markdown += text.simplified();
					markdown += '\n';
				}
				else
				{
					markdown += "\n# ";
					QString text;
					processHTMLNode(n, insideTable, footnotesToRefs, h1emitted, text);
					markdown += text.simplified();
					markdown += '\n';
					h1emitted = true;
				}
			}
			else if(tagName.size() == 2 && tagName[0] == QLatin1Char('h') && '2' <= tagName[1] && tagName[1] <= '6')
			{
				if(insideTable)
				{
					std::cerr << "WARNING: Unexpected <h1> tag";
					return false;
				}

				if(!h1emitted)
					std::cerr << "ERROR: Unexpected <" << tagName.toStdString() << "> tag before any <h1> tag was found\n";

				const int level = tagName[1].toLatin1() - '0';
				markdown += '\n';
				markdown += QString(level, QLatin1Char('#'));
				markdown += ' ';
				QString text;
				processHTMLNode(n, insideTable, footnotesToRefs, h1emitted, text);
				markdown += text.simplified();
				markdown += '\n';
			}
			else if(tagName == "i" || tagName == "em" || tagName == "b")
			{
				const QString marking = tagName == "b" ? "**" : "*";

				QString text;
				processHTMLNode(n, insideTable, footnotesToRefs, h1emitted, text);
				markdown += marking;
				while(!text.isEmpty() && text[0].isSpace())
				{
					markdown += text[0];
					text.removeFirst();
				}
				QString trailingWS;
				while(!text.isEmpty() && text.back().isSpace())
				{
					trailingWS = text.back() + trailingWS;
					text.chop(1);
				}
				markdown += text;
				markdown += marking;
				markdown += trailingWS;
			}
			else if(tagName == "p")
			{
				if(el.hasAttribute("id"))
				{
					if(footnotesToRefs)
					{
						static const QRegularExpression footnote("^footnote-([0-9]+)$");
						if(const auto match = footnote.match(el.attribute("id")); match.isValid())
						{
							addNewlineBeforeNodeIfNeeded(markdown);
							markdown += " - [#";
							markdown += match.captured(1);
							markdown += "]: ";
							QString text;
							processHTMLNode(n, insideTable, footnotesToRefs, h1emitted, text);
							text = text.simplified();
							const QRegularExpression textToRemove("^\\[\\s*"+match.captured(1)+"\\s*\\]\\s*");
							text.replace(textToRemove, "");
							markdown += text;
							markdown += "\n";
							continue;
						}
					}

					formatSectionAsHTML(el, markdown);
				}
				else
				{
					markdown += "\n";
					processHTMLNode(n, insideTable, footnotesToRefs, h1emitted, markdown);
					markdown += "\n";
				}
			}
			else if(tagName == "img")
			{
				if(el.hasAttribute("width") || el.hasAttribute("height"))
				{
					formatImgInHTML(el, markdown);
				}
				else
				{
					markdown += "![";
					markdown += el.attribute("alt");
					markdown += "](";
					markdown += el.attribute("src");
					markdown += ")";
				}
			}
			else if(tagName == "a")
			{
				markdown += "[";
				QString content;
				processHTMLNode(n, insideTable, footnotesToRefs, h1emitted, content);
				if(content.contains("[") || content.contains("]"))
					qWarning() << "WARNING: found a link whose text contains square brackets. This may interfere with Markdown parsing.\n";
				markdown += content.trimmed();
				markdown += "](";
				markdown += el.attribute("href");
				markdown += ")";
			}
			else if(tagName == "br")
			{
				if(insideTable)
					markdown += "<br>";
				else
					markdown += "\n\n";
			}
			else if(tagName == "table")
			{
				if(el.attribute("class") == "layout")
				{
					// We can't add a class attribute to a Markdown table, so keep it in HTML
					qWarning() << "Markdown tables don't support class \"layout\", leaving such a table in HTML format.";
					formatSectionAsHTML(el, markdown);
				}
				else
				{
					processTable(el, markdown);
				}
			}
			else if(tagName == "ul" || tagName == "ol")
			{
				processList(el, markdown);
			}
			else if(tagName == "blockquote")
			{
				QString blockquote;
				processHTMLNode(n, insideTable, footnotesToRefs, h1emitted, blockquote);
				blockquote = blockquote.trimmed();
				blockquote.replace("\n", "\n> ");
				addNewlineBeforeNodeIfNeeded(markdown);
				markdown += "\n> ";
				markdown += blockquote;
				markdown += "\n";
			}
			else if(tagName == "sup")
			{
				if(const auto child = el.firstChild(); child.isElement())
				{
					if(footnotesToRefs)
					{
						if(const auto el = child.toElement(); el.tagName() == "a")
						{
							static const QRegularExpression footnote("^#footnote-([0-9]+)$");
							if(const auto match = footnote.match(el.attribute("href")); match.isValid())
							{
								markdown += "[#";
								markdown += match.captured(1);
								markdown += "]";
								continue;
							}
						}
					}
				}
				// Markdown doesn't have any special way to format this
				QString text;
				QTextStream s(&text);
				n.save(s, 0);
				markdown += text.simplified();
			}
			else if(tagName == "sub")
			{
				// Markdown doesn't have any special way to format this
				QString text;
				QTextStream s(&text);
				n.save(s, 0);
				markdown += text.simplified();
			}
			else if(tagName == "dl")
			{
				// Markdown doesn't have any special way to format this
				QString text;
				QTextStream s(&text);
				n.save(s, 1);
				markdown += text;
			}
			else
			{
				qDebug() << "WARNING: Unhandled HTML element:" << n.toElement().tagName();
			}
			break;
		}
		case QDomNode::TextNode:
			// This mustn't be simplified(), otherwise this call will break "text <i>like</i> this"
			markdown += n.toText().data().replace("\n", " ");
			break;
		case QDomNode::CommentNode:
			addNewlineBeforeNodeIfNeeded(markdown);
			markdown += "<!--";
			markdown += n.toComment().data();
			markdown += "-->";
			break;
		case QDomNode::EntityReferenceNode:
		{
			QString text;
			QTextStream s(&text);
			n.save(s, 1);
			markdown += text;
			break;
		}
		default:
		{
			qWarning().noquote().nospace() << "WARNING: Unhandled HTML node: " << nodeTypeName(n.nodeType()) << ". Formatting as HTML.";
			QString text;
			QTextStream s(&text);
			n.save(s, 1);
			markdown += text;
			break;
		}
		}
	}
	return true;
}

[[nodiscard]] QString convertHTMLToMarkdown(QString htmlIn, const bool footnotesToRefs)
{
	// Replace <notr> and </notr> tags with placeholders that
	// don't look like tags, so as not to confuse libTidy.
	const QString notrOpenPlaceholder = "{22c35d6a-5ec3-4405-aeff-e79998dc95f7}";
	const QString notrClosePlaceholder = "{2543be41-c785-4283-a4cf-ce5471d2c422}";
	htmlIn.replace(QRegularExpression("<notr\\s*>"), notrOpenPlaceholder);
	htmlIn.replace(QRegularExpression("</notr\\s*>"), notrClosePlaceholder);

	const auto html = tidyHTML(htmlIn);

	QString markdown;

	QDomDocument dom;
	dom.setContent(html);

	QDomNode n;
	bool bodyFound = false;
	for(n = dom.firstChild(); !n.isNull(); n = n.nextSibling())
	{
		if(n.isElement() && n.toElement().tagName() == "html")
			for(n = n.firstChild(); !n.isNull(); n = n.nextSibling())
				if(n.isElement() && n.toElement().tagName() == "body")
				{
					bodyFound = true;
					goto afterBodyFound;
				}
	}
	if(!bodyFound)
	{
		std::cerr << "Failed to found HTML <body> tag in tidied HTML\n";
		return {};
	}
afterBodyFound:

	bool h1emitted = false;
	processHTMLNode(n, false, footnotesToRefs, h1emitted, markdown);

	// Restore the reserved tags
	markdown.replace(notrOpenPlaceholder,  "<notr>");
	markdown.replace(notrClosePlaceholder, "</notr>");

	return markdown;
}

void addMissingTextToMarkdown(QString& markdown, const QString& inDir, const QString& author, const QString& credit, const QString& license)
{
	// Add missing "Introduction" heading if we have a headingless intro text
	if(!markdown.contains(QRegularExpression("^\\s*# [^\n]+\n+\\s*##\\s*Introduction\n")))
		markdown.replace(QRegularExpression("^(\\s*# [^\n]+\n+)(\\s*[^#])"), "\\1## Introduction\n\n\\2");
	if(!markdown.contains("\n## Description\n"))
	   markdown.replace(QRegularExpression("(\n## Introduction\n[^#]+\n)(\\s*#)"), "\\1## Description\n\n\\2");

	// Add some sections the info for which is contained in info.ini in the old format
	if(markdown.contains(QRegularExpression("\n##\\s+(?:References|External\\s+links)\\s*\n")))
		markdown.replace(QRegularExpression("(\n##[ \t]+)External[ \t]+links([ \t]*\n)"), "\\1References\\2");
	auto referencesFromFile = readReferencesFile(inDir);

	if(markdown.contains(QRegularExpression("\n##\\s+Authors?\\s*\n")))
	{
		qWarning() << "Authors section already exists, not adding the authors from info.ini";

		// But do add references before this section
		if(!referencesFromFile.isEmpty())
			markdown.replace(QRegularExpression("(\n##\\s+Authors?\\s*\n)"), "\n"+referencesFromFile + "\n\\1");
	}
	else
	{
		// First add references
		if(!referencesFromFile.isEmpty())
			markdown += referencesFromFile + "\n";

		if(credit.isEmpty())
			markdown += QString("\n## Authors\n\n%1\n").arg(author);
		else
			markdown += "\n## Authors\n\nAuthor is " + author + ". Additional credit goes to " + credit + "\n";
	}

	if(markdown.contains(QRegularExpression("\n##\\s+License\\s*\n")))
		qWarning() << "License section already exists, not adding the license from info.ini";
	else
		markdown += "\n## License\n\n" + license + "\n";

	cleanupWhitespace(markdown);
}

struct Section
{
	int level = -1;
	int levelAddition = 0;
	int headerLineStartPos = -1;
	int headerStartPos = -1; // including #..#
	int bodyStartPos = -1;
	QString title;
	QString body;
	std::deque<int> subsections;
};

std::vector<Section> splitToSections(const QString& markdown)
{
	const QRegularExpression sectionHeaderPattern("^[ \t]*((#+)\\s+(.*[^\\s])\\s*)$", QRegularExpression::MultilineOption);
	std::vector<Section> sections;
	for(auto matches = sectionHeaderPattern.globalMatch(markdown); matches.hasNext(); )
	{
		sections.push_back({});
		auto& section = sections.back();
		const auto& match = matches.next();
		section.headerLineStartPos = match.capturedStart(0);
		section.headerStartPos = match.capturedStart(1);
		section.level = match.captured(2).length();
		section.title = match.captured(3);
		section.bodyStartPos = match.capturedEnd(0) + 1/*\n*/;

		if(section.title.trimmed() == "Author")
			section.title = "Authors";
	}

	for(unsigned n = 0; n < sections.size(); ++n)
	{
		if(n+1 < sections.size())
			sections[n].body = markdown.mid(sections[n].bodyStartPos,
			                                std::max(0, sections[n+1].headerLineStartPos - sections[n].bodyStartPos))
			                           .replace(QRegularExpression("^\n*|\\s*$"), "");
		else
			sections[n].body = markdown.mid(sections[n].bodyStartPos).replace(QRegularExpression("^\n*|\\s*$"), "");
	}

	return sections;
}

bool isStandardTitle(const QString& title)
{
	return title == "Introduction" ||
	       title == "Description" ||
	       title == "Constellations" ||
	       title == "References" ||
	       title == "Authors" ||
	       title == "License";
}

void gettextpo_xerror(int severity, po_message_t message, const char *filename, size_t lineno, size_t column, int multiline_p, const char *message_text)
{
	(void)message;
	qWarning().nospace() << "libgettextpo: " << filename << ":" << lineno << ":" << column << ": " << (multiline_p ? "\n" : "") << message_text;
	if(severity == PO_SEVERITY_FATAL_ERROR)
		std::abort();
}

void gettextpo_xerror2(int severity,
                       po_message_t message1, const char *filename1, size_t lineno1, size_t column1, int multiline_p1, const char *message_text1,
                       po_message_t message2, const char *filename2, size_t lineno2, size_t column2, int multiline_p2, const char *message_text2)
{
	(void)message1;
	(void)message2;
	qWarning().nospace() << "libgettextpo: error with two messages:";
	qWarning().nospace() << "libgettextpo: message 1 error: " << filename1 << ":" << lineno1 << ":" << column1 << ": " << (multiline_p1 ? "\n" : "") << message_text1;
	qWarning().nospace() << "libgettextpo: message 2 error: " << filename2 << ":" << lineno2 << ":" << column2 << ": " << (multiline_p2 ? "\n" : "") << message_text2;
	if(severity == PO_SEVERITY_FATAL_ERROR)
		std::abort();
}
}

QString DescriptionOldLoader::translateSection(const QString& markdown, const qsizetype bodyStartPos,
                                               const qsizetype bodyEndPos, const QString& locale, const QString& sectionName)
{
	const auto comment = QString("Sky culture %1 section in markdown format").arg(sectionName.trimmed().toLower());
	auto text = markdown.mid(bodyStartPos, bodyEndPos - bodyStartPos);
	text.replace(QRegularExpression("^\n*|\n*$"), "");
	allMarkdownSections.insert(DictEntry{.comment = {comment}, .english = text, .translated = ""});
	for(const auto& entry : translations[locale])
	{
		if(entry.english == text)
		{
			text = stripComments(entry.translated);
			break;
		}
		if(entry.comment.find(comment) != entry.comment.end())
		{
			qWarning() << " *** BAD TRANSLATION ENTRY for section" << sectionName << "in locale" << locale;
			qWarning() << "  Entry msgid:" << entry.english;
			qWarning() << "  English section text:" << text << "\n";
			continue;
		}
	}
	return text;
}

QString DescriptionOldLoader::translateDescription(const QString& markdownInput, const QString& locale)
{
	const auto markdown = stripComments(markdownInput);

	const QRegularExpression headerPat("^# +(.+)$", QRegularExpression::MultilineOption);
	const auto match = headerPat.match(markdown);
	QString name;
	if (match.isValid())
	{
		name = match.captured(1);
	}
	else
	{
		qCritical().nospace() << "Failed to get sky culture name: got " << match.lastCapturedIndex() << " matches instead of 1";
		name = "Unknown";
	}

	QString text = "# " + name + "\n\n";
	const QRegularExpression sectionNamePat("^## +(.+)$", QRegularExpression::MultilineOption);
	QString prevSectionName;
	qsizetype prevBodyStartPos = -1;
	for (auto it = sectionNamePat.globalMatch(markdown); it.hasNext(); )
	{
		const auto match = it.next();
		const auto sectionName = match.captured(1);
		const auto nameStartPos = match.capturedStart(0);
		const auto bodyStartPos = match.capturedEnd(0);
		if (!prevSectionName.isEmpty())
		{
			const auto sectionText = translateSection(markdown, prevBodyStartPos, nameStartPos, locale, prevSectionName);
			text += "## " + prevSectionName + "\n\n";
			if (!sectionText.isEmpty())
				text += sectionText + "\n\n";
		}
		prevBodyStartPos = bodyStartPos;
		prevSectionName = sectionName;
	}
	if (prevBodyStartPos >= 0)
	{
		const auto sectionText = translateSection(markdown, prevBodyStartPos, markdown.size(), locale, prevSectionName);
		if (!sectionText.isEmpty())
		{
			text += "## " + prevSectionName + "\n\n";
			text += sectionText;
		}
	}

	return text;
}

void DescriptionOldLoader::addUntranslatedNames(const QString scName, const ConstellationOldLoader& consLoader,
                                                const AsterismOldLoader& astLoader, const NamesOldLoader& namesLoader)
{
	for(auto& dict : translations)
	{
		std::set<QString> translated;
		for(const auto& entry : dict)
			translated.insert(entry.english);
		std::map<QString/*msgid*/,unsigned/*position in dict*/> emittedNames;
		for(const auto& cons : consLoader)
		{
			if(cons.englishName.isEmpty())
				continue;
			if(translated.find(cons.englishName) != translated.end())
				continue;
			QString comments = scName+" constellation";
			if(!cons.nativeName.isEmpty())
				comments += ", native: "+cons.nativeName;
			comments += '\n' + cons.translatorsComments;
			if(const auto it = emittedNames.find(cons.englishName); it == emittedNames.end())
			{
				emittedNames[cons.englishName] = dict.size();
				dict.push_back({{comments}, cons.englishName, ""});
			}
			else
			{
				auto& entry = dict[it->second];
				entry.comment.insert(comments);
			}
		}
		for(const auto& ast : astLoader)
		{
			if(ast->getEnglishName().isEmpty())
				continue;
			if(translated.find(ast->getEnglishName()) != translated.end())
				continue;
			QString comments = scName+" asterism";
			comments += '\n' + ast->getTranslatorsComments();
			if(const auto it = emittedNames.find(ast->getEnglishName()); it == emittedNames.end())
			{
				emittedNames[ast->getEnglishName()] = dict.size();
				dict.push_back({{comments}, ast->getEnglishName(), ""});
			}
			else
			{
				auto& entry = dict[it->second];
				entry.comment.insert(comments);
			}
		}
		for(auto it = namesLoader.starsBegin(); it != namesLoader.starsEnd(); ++it)
		{
			for(const auto& star : it.value())
			{
				if(translated.find(star.englishName) != translated.end())
					continue;
				QString comments;
				if(star.nativeName.isEmpty())
					comments = QString("%1 name for HIP %2").arg(scName).arg(star.HIP);
				else
					comments = QString("%1 name for HIP %2, native: %3").arg(scName).arg(star.HIP).arg(star.nativeName);
				comments += '\n' + star.translatorsComments;
				if(const auto it = emittedNames.find(star.englishName); it == emittedNames.end())
				{
					emittedNames[star.englishName] = dict.size();
					dict.push_back({{comments}, star.englishName, ""});
				}
				else
				{
					auto& entry = dict[it->second];
					entry.comment.insert(comments);
				}
			}
		}
		for(auto it = namesLoader.planetsBegin(); it != namesLoader.planetsEnd(); ++it)
		{
			for(const auto& planet : it.value())
			{
				if(translated.find(planet.english) != translated.end())
					continue;
				QString comments;
				if(planet.native.isEmpty())
					comments = QString("%1 name for NAME %2").arg(scName).arg(planet.id);
				else
					comments = QString("%1 name for NAME %2, native: %3").arg(scName).arg(planet.id, planet.native);
				comments += '\n' + planet.translatorsComments;
				if(const auto it = emittedNames.find(planet.english); it == emittedNames.end())
				{
					emittedNames[planet.english] = dict.size();
					dict.push_back({{comments}, planet.english, ""});
				}
				else
				{
					auto& entry = dict[it->second];
					entry.comment.insert(comments);
				}
			}
		}
		for(auto it = namesLoader.dsosBegin(); it != namesLoader.dsosEnd(); ++it)
		{
			for(const auto& dso : it.value())
			{
				if(translated.find(dso.englishName) != translated.end())
					continue;
				QString comments;
				if(dso.nativeName.isEmpty())
					comments = QString("%1 name for NAME %2").arg(scName).arg(dso.id);
				else
					comments = QString("%1 name for NAME %2, native: %3").arg(scName).arg(dso.id, dso.nativeName);
				comments += '\n' + dso.translatorsComments;
				if(const auto it = emittedNames.find(dso.englishName); it == emittedNames.end())
				{
					emittedNames[dso.englishName] = dict.size();
					dict.push_back({{comments}, dso.englishName, ""});
				}
				else
				{
					auto& entry = dict[it->second];
					entry.comment.insert(comments);
				}
			}
		}
	}
}

void DescriptionOldLoader::loadTranslationsOfNames(const QString& poBaseDir, const QString& cultureIdQS, const QString& englishName,
                                                   const ConstellationOldLoader& consLoader, const AsterismOldLoader& astLoader,
                                                   const NamesOldLoader& namesLoader)
{
	po_xerror_handler handler = {gettextpo_xerror, gettextpo_xerror2};
	const auto cultureId = cultureIdQS.toStdString();

	const auto poDir = poBaseDir+"/stellarium-skycultures";
	if(!poBaseDir.isEmpty() && !QFile(poDir).exists())
		qWarning() << "Warning: no such directory" << poDir << "- will not load existing translations of names.";
	for(const auto& fileName : QDir(poDir).entryList({"*.po"}))
	{
		const QString locale = fileName.chopped(3);
		const auto file = po_file_read((poDir+"/"+fileName).toStdString().c_str(), &handler);
		if(!file) continue;

		const auto header = po_file_domain_header(file, nullptr);
		if(header) poHeaders[locale] = header;

		qDebug().nospace() << "Processing translations of names for locale " << locale << "...";
		auto& dict = translations[locale];
		std::unordered_map<QString/*msgid*/,int/*position*/> insertedNames;

		// First try to find translation for the name of the sky culture
		bool scNameTranslated = false;
		if(const auto scNameFile = po_file_read((poBaseDir+"/stellarium/"+fileName).toStdString().c_str(), &handler))
		{
			const auto domains = po_file_domains(scNameFile);
			for(auto domainp = domains; *domainp && !scNameTranslated; domainp++)
			{
				const auto domain = *domainp;
				po_message_iterator_t iterator = po_message_iterator(scNameFile, domain);

				for(auto message = po_next_message(iterator); message != nullptr; message = po_next_message(iterator))
				{
					const auto msgid = po_message_msgid(message);
					const auto msgstr = po_message_msgstr(message);
					const auto ctxt = po_message_msgctxt(message);
					if(ctxt && ctxt == std::string_view("sky culture") && msgid == englishName)
					{
						dict.insert(dict.begin(), {{"Sky culture name"}, msgid, msgstr});
						scNameTranslated = true;
						break;
					}
				}
				po_message_iterator_free(iterator);
			}
			po_file_free(scNameFile);
		}

		if(!scNameTranslated)
			qWarning() << "Couldn't find a translation for the name of the sky culture";

		const std::map<std::string, std::string_view> sourceFiles{
			{"skycultures/"+cultureId+"/star_names.fab", "star"},
			{"skycultures/"+cultureId+"/dso_names.fab", "dso"},
			{"skycultures/"+cultureId+"/planet_names.fab", "planet"},
			{"skycultures/"+cultureId+"/asterism_names.eng.fab", "asterism"},
			{"skycultures/"+cultureId+"/constellation_names.eng.fab", "constellation"},
		};
		QString comments;
		const auto domains = po_file_domains(file);
		for(auto domainp = domains; *domainp; domainp++)
		{
			const auto domain = *domainp;
			po_message_iterator_t iterator = po_message_iterator(file, domain);

			for(auto message = po_next_message(iterator); message != nullptr; message = po_next_message(iterator))
			{
				const auto msgid = po_message_msgid(message);
				const auto msgstr = po_message_msgstr(message);
				QString comments = po_message_extracted_comments(message);
				for(int n = 0; ; ++n)
				{
					const auto filepos = po_message_filepos(message, n);
					if(!filepos) break;
					const auto refFileName = po_filepos_file(filepos);
					const auto ref = sourceFiles.find(refFileName);
					if(ref == sourceFiles.end()) continue;
					const auto type = ref->second;
					if(type == "constellation")
					{
						const auto cons = consLoader.find(msgid);
						if(cons)
						{
							comments = englishName+" constellation";
							if(!cons->nativeName.isEmpty())
								comments += ", native: "+cons->nativeName;
							comments += '\n' + cons->translatorsComments;
						}
						else
						{
							continue;
						}
					}
					else if(type == "asterism")
					{
						if(const auto aster = astLoader.find(msgid))
						{
							comments = englishName+" asterism";
							comments += '\n' + aster->getTranslatorsComments();
						}
						else
						{
							continue;
						}
					}
					else if(type == "star")
					{
						const auto star = namesLoader.findStar(msgid);
						if(star && star->HIP > 0)
						{
							if(star->nativeName.isEmpty())
								comments = QString("%1 name for HIP %2").arg(englishName).arg(star->HIP);
							else
								comments = QString("%1 name for HIP %2, native: %3").arg(englishName).arg(star->HIP).arg(star->nativeName);
							comments += '\n' + star->translatorsComments;
						}
						else
						{
							continue;
						}
					}
					else if(type == "planet")
					{
						if(const auto planet = namesLoader.findPlanet(msgid))
						{
							if(planet->native.isEmpty())
								comments = QString("%1 name for NAME %2").arg(englishName).arg(planet->id);
							else
								comments = QString("%1 name for NAME %2, native: %3").arg(englishName).arg(planet->id, planet->native);
							comments += '\n' + planet->translatorsComments;
						}
						else
						{
							continue;
						}
					}
					else if(type == "dso")
					{
						if(const auto dso = namesLoader.findDSO(msgid))
						{
							if(dso->nativeName.isEmpty())
								comments = QString("%1 name for %2").arg(englishName).arg(dso->id);
							else
								comments = QString("%1 name for %2, native: %3").arg(englishName).arg(dso->id, dso->nativeName);
							comments += '\n' + dso->translatorsComments;
						}
						else
						{
							continue;
						}
					}
					if(const auto it = insertedNames.find(msgid); it != insertedNames.end())
					{
						auto& entry = dict[it->second];
						entry.comment.insert(comments);
						continue;
					}
					insertedNames[msgid] = dict.size();
					dict.push_back({{comments}, msgid, msgstr});
				}
			}
			po_message_iterator_free(iterator);
		}
		po_file_free(file);
	}
	addUntranslatedNames(englishName, consLoader, astLoader, namesLoader);
}

void DescriptionOldLoader::locateAndRelocateAllInlineImages(QString& html, const bool saveToRefs)
{
	for(auto matches = htmlGeneralImageRegex.globalMatch(html); matches.hasNext(); )
	{
		const auto& match = matches.next();
		auto path = match.captured(1);
		auto updatedPath = path;
		if(!path.startsWith("illustrations/"))
		{
			updatedPath = "illustrations/" + path;
			const auto imgTag = match.captured(0);
			const auto updatedImgTag = QString(imgTag).replace(path, updatedPath);
			html.replace(imgTag, updatedImgTag);
		}
		if(saveToRefs)
			imageHRefs.emplace_back(path, updatedPath);
	}
}

void DescriptionOldLoader::load(const QString& inDir, const QString& poBaseDir, const QString& cultureId, const QString& englishName,
                                const QString& author, const QString& credit, const QString& license,
                                const ConstellationOldLoader& consLoader, const AsterismOldLoader& astLoader, const NamesOldLoader& namesLoader,
                                const bool footnotesToRefs, const bool genTranslatedMD)
{
	inputDir = inDir;
	const auto englishDescrPath = inDir+"/description.en.utf8";
	QFile englishDescrFile(englishDescrPath);
	if(!englishDescrFile.open(QFile::ReadOnly))
	{
		qCritical().noquote() << "Failed to open file" << englishDescrPath;
		return;
	}
	QString html = englishDescrFile.readAll();
	locateAndRelocateAllInlineImages(html, true);
	qDebug() << "Processing English description...";
	markdown = convertHTMLToMarkdown(html, footnotesToRefs);

	auto englishSections = splitToSections(markdown);
	const int level1sectionCount = std::count_if(englishSections.begin(), englishSections.end(),
	                                             [](auto& s){return s.level==1;});
	if(level1sectionCount != 1)
	{
		qCritical().nospace() << "Unexpected number of level-1 sections in file " << englishDescrPath
		                      << " (expected 1, found " << level1sectionCount
		                      << "), will not convert the description";
		return;
	}

	// Mark all sections with level>2 to be subsections of the nearest preceding level<=2 sections
	std::deque<int> subsections;
	for(int n = signed(englishSections.size()) - 1; n >= 0; --n)
	{
		const bool hasStandardTitle = isStandardTitle(englishSections[n].title);
		if(hasStandardTitle && englishSections[n].level != 2)
		{
			qWarning() << "Warning: found a section titled" << englishSections[n].title
			           << "but having level" << englishSections[n].level << " instead of 2";
		}

		if(englishSections[n].level > 2 || (englishSections[n].level == 2 && !hasStandardTitle))
		{
			subsections.push_front(n);
		}
		else
		{
			englishSections[n].subsections = std::move(subsections);
			subsections.clear();
		}
	}

	// Increase the level of all level-2 sections and their subsections unless they have one of the standard titles
	for(auto& section : englishSections)
	{
		if(section.level != 2 || isStandardTitle(section.title)) continue;
		if(section.level == 2)
		{
			for(const int n : section.subsections)
				englishSections[n].levelAddition = 1;
		}
		section.levelAddition = 1;
	}

	if(englishSections.empty())
	{
		qCritical() << "No sections found in" << englishDescrPath;
		return;
	}

	if(englishSections[0].level != 1)
	{
		qCritical() << "Unexpected section structure: first section must have level 1, but instead has" << englishSections[0].level;
		return;
	}

	if(englishSections[0].title.trimmed().toLower() != englishName.toLower())
	{
		qWarning().nospace() << "English description caption is not the same as the name of the sky culture: "
		                     << englishSections[0].title << " vs " << englishName << ". Will change the caption to match the name.";
		englishSections[0].title = englishName;
	}

	const QRegularExpression localePattern("description\\.([^.]+)\\.utf8");

	// This will contain the final form of the English sections for use as a key
	// in translations as well as to reconstruct the main description.md
	std::vector<std::pair<QString/*title*/,QString/*section*/>> finalEnglishSections;
	bool finalEnglishSectionsDone = false;

	bool descrSectionExists = false;
	for(const auto& section : englishSections)
	{
		if(section.level + section.levelAddition != 2)
			continue;
		if(section.title.trimmed().toLower() == "description")
			descrSectionExists = true;
	}

	std::vector<QString> locales;
	for(const auto& fileName : QDir(inDir).entryList({"description.*.utf8"}))
	{
		if(fileName == "description.en.utf8") continue;

		const auto localeMatch = localePattern.match(fileName);
		if(!localeMatch.isValid())
		{
			qCritical() << "Failed to extract locale from file name" << fileName;
			continue;
		}
		const auto locale = localeMatch.captured(1);
		locales.push_back(locale);
		const auto path = inDir + "/" + fileName;
		QFile file(path);
		if(!file.open(QFile::ReadOnly))
		{
			qCritical().noquote() << "Failed to open file" << path << "\n";
			continue;
		}
		qDebug().nospace() << "Processing description for locale " << locale << "...";
		QString localizedHTML = file.readAll();
		locateAndRelocateAllInlineImages(localizedHTML, false);
		auto trMD0 = convertHTMLToMarkdown(localizedHTML, footnotesToRefs);
		const auto translationMD = trMD0.replace(QRegularExpression("<notr>([^<]+)</notr>"), "\\1");
		const auto translatedSections = splitToSections(translationMD);
		if(translatedSections.size() != englishSections.size())
		{
			qCritical().nospace().noquote() << "Number of sections (" << translatedSections.size()
			                                << ") in description for locale " << locale
			                                << " doesn't match that of the English description ("
			                                << englishSections.size() << "). Skipping this translation.";
			auto dbg = qDebug().nospace().noquote();
			dbg << " ** English section titles:\n";
			for(const auto& sec : englishSections)
				dbg << sec.level << ": " << sec.title << "\n";
			dbg << " ** Translated section titles:\n";
			for(const auto& sec : translatedSections)
				dbg << sec.level << ": " << sec.title << "\n";
			dbg << "\n____________________________________________\n";
			continue;
		}

		bool sectionsOK = true;
		for(unsigned n = 0; n < englishSections.size(); ++n)
		{
			if(translatedSections[n].level != englishSections[n].level)
			{
				qCritical() << "Section structure of English text and translation for"
				            << locale << "doesn't match, skipping this translation";
				auto dbg = qDebug().nospace();
				dbg << " ** English section titles:\n";
				for(const auto& sec : englishSections)
					dbg << sec.level << ": " << sec.title << "\n";
				dbg << " ** Translated section titles:\n";
				for(const auto& sec : translatedSections)
					dbg << sec.level << ": " << sec.title << "\n";
				dbg << "\n____________________________________________\n";
				sectionsOK = false;
				break;
			}
		}
		if(!sectionsOK) continue;

		TranslationDict dict;
		for(unsigned n = 0; n < englishSections.size(); ++n)
		{
			const auto& engSec = englishSections[n];
			if(engSec.level + engSec.levelAddition > 2) continue;

			QString key = engSec.body;
			QString value = translatedSections[n].body;
			auto titleForComment = engSec.title.contains(' ') ? '"' + engSec.title.toLower() + '"' : engSec.title.toLower();
			auto sectionTitle = engSec.title;
			bool insertDescriptionHeading = false;
			if(engSec.level == 1 && !key.isEmpty())
			{
				if(!finalEnglishSectionsDone)
					finalEnglishSections.emplace_back("Introduction", key);
				auto comment = QString("Sky culture introduction section in markdown format");
				dict.push_back({{std::move(comment)}, stripComments(key), std::move(value)});
				key = "";
				value = "";
				if(descrSectionExists) continue;

				titleForComment = "description";
				sectionTitle = "Description";
				insertDescriptionHeading = true;
			}

			for(const auto subN : engSec.subsections)
			{
				const auto& keySubSection = englishSections[subN];
				key += "\n\n";
				key += QString(keySubSection.level + keySubSection.levelAddition, QChar('#'));
				key += ' ';
				key += keySubSection.title;
				key += "\n\n";
				key += keySubSection.body;
				key += "\n\n";
				cleanupWhitespace(key);
				key.replace(QRegularExpression("^\n*|\\s*$"), "");

				const auto& valueSubSection = translatedSections[subN];
				value += "\n\n";
				value += QString(keySubSection.level + keySubSection.levelAddition, QChar('#'));
				value += ' ';
				value += valueSubSection.title;
				value += "\n\n";
				value += valueSubSection.body;
				value += "\n\n";
				cleanupWhitespace(value);
				value.replace(QRegularExpression("^\n*|\\s*$"), "");
			}
			if(!finalEnglishSectionsDone)
			{
				if((!sectionTitle.isEmpty() && engSec.level + engSec.levelAddition == 2) ||
				   insertDescriptionHeading)
					finalEnglishSections.emplace_back(sectionTitle, key);
			}
			if(!key.isEmpty())
			{
				auto comment = QString("Sky culture %1 section in markdown format").arg(titleForComment);
				dict.push_back({{std::move(comment)}, stripComments(key), std::move(value)});
			}
		}
		if(!finalEnglishSections.empty())
			finalEnglishSectionsDone = true;
		translations[locale] = std::move(dict);
	}

	// Reconstruct markdown from the altered sections
	if(finalEnglishSections.empty())
	{
		// A case where no translation exists
		markdown.clear();
		for(const auto& section : englishSections)
		{
			markdown += QString(section.level + section.levelAddition, QChar('#'));
			markdown += ' ';
			markdown += section.title.trimmed();
			markdown += "\n\n";
			markdown += section.body;
			markdown += "\n\n";
		}
	}
	else
	{
		markdown = "# " + englishSections[0].title + "\n\n";
		for(const auto& section : finalEnglishSections)
		{
			markdown += "## ";
			markdown += section.first;
			markdown += "\n\n";
			markdown += section.second;
			markdown += "\n\n";
		}
	}

	addMissingTextToMarkdown(markdown, inDir, author, credit, license);
	if(genTranslatedMD)
	{
		for(const auto& locale : locales)
			translatedMDs[locale] = translateDescription(markdown, locale);
	}

	loadTranslationsOfNames(poBaseDir, cultureId, englishName, consLoader, astLoader, namesLoader);
}

bool DescriptionOldLoader::dumpMarkdown(const QString& outDir) const
{
	const auto path = outDir+"/description.md";
	QFile file(path);
	if(!file.open(QFile::WriteOnly))
	{
		qCritical().noquote() << "Failed to open file" << path << "\n";
		return false;
	}

	if(markdown.isEmpty()) return true;

	if(file.write(markdown.toUtf8()) < 0 || !file.flush())
	{
		qCritical().noquote() << "Failed to write " << path << ": " << file.errorString() << "\n";
		return false;
	}

	for(const auto& img : imageHRefs)
	{
		const auto imgInPath = inputDir+"/"+img.inputPath;
		const auto imgInInfo = QFileInfo(imgInPath);
		if(!imgInInfo.exists())
		{
			qCritical() << "Failed to locate an image referenced in the description:" << img.inputPath;
			continue;
		}
		const auto imgOutPath = outDir + "/" + img.outputPath;
		const auto imgOutInfo = QFileInfo(imgOutPath);
		bool targetExistsAndDiffers = imgOutInfo.exists() && imgInInfo.size() != imgOutInfo.size();
		if(imgOutInfo.exists() && !targetExistsAndDiffers)
		{
			// Check that the contents are the same
			QFile in(imgInPath);
			if(!in.open(QFile::ReadOnly))
			{
				qCritical().noquote() << "Failed to open file" << imgInPath;
				continue;
			}
			QFile out(imgOutPath);
			if(!out.open(QFile::ReadOnly))
			{
				qCritical().noquote() << "Failed to open file" << imgOutPath;
				continue;
			}
			targetExistsAndDiffers = in.readAll() != out.readAll();
		}
		if(targetExistsAndDiffers)
		{
			qCritical() << "Image file names collide:" << img.inputPath << "and" << img.outputPath;
			continue;
		}

		if(imgOutInfo.exists())
		{
			// It exists, but is the same as the file we're trying to copy, so skip it
			continue;
		}

		const auto imgDir = QFileInfo(imgOutPath).absoluteDir().absolutePath();
		if(!QDir().mkpath(imgDir))
		{
			qCritical() << "Failed to create output directory for image file" << img.outputPath;
			continue;
		}

		if(!QFile(imgInPath).copy(imgOutPath))
		{
			qCritical() << "Failed to copy an image file referenced in the description:" << img.inputPath << "to" << img.outputPath;
			continue;
		}
	}

	if(!translatedMDs.isEmpty())
	{
		for(const auto& key : translatedMDs.keys())
		{
			const auto path = outDir+"/description."+key+".DO_NOT_COMMIT.md";
			QFile file(path);
			if(!file.open(QFile::WriteOnly))
			{
				qCritical().noquote() << "Failed to open file" << path << "\n";
				return false;
			}
			if(file.write(translatedMDs[key].toUtf8()) < 0 || !file.flush())
			{
				qCritical().noquote() << "Failed to write " << path << ": " << file.errorString() << "\n";
				return false;
			}
		}
	}

	return true;
}

bool DescriptionOldLoader::dump(const QString& outDir) const
{
	if(!dumpMarkdown(outDir)) return false;

	const auto poDir = outDir + "/po";
	if(!QDir().mkpath(poDir))
	{
		qCritical() << "Failed to create po directory\n";
		return false;
	}

	for(auto dictIt = translations.begin(); dictIt != translations.end(); ++dictIt)
	{
		const auto& locale = dictIt.key();
		const auto path = poDir + "/" + locale + ".po";

		const auto file = po_file_create();
		po_message_iterator_t iterator = po_message_iterator(file, nullptr);

		// I've found no API to *create* a header, so will try to emulate it with a message
		const auto headerIt = poHeaders.find(locale);
		static const auto defaultHeaderTemplate = QLatin1String(
			"Project-Id-Version: PACKAGE VERSION\n"
			"MIME-Version: 1.0\n"
			"Content-Type: text/plain; charset=UTF-8\n"
			"Content-Transfer-Encoding: 8bit\n"
			"Language: %1\n");
		const QString header = headerIt == poHeaders.end() ?
			defaultHeaderTemplate.arg(locale) :
			headerIt.value();
		const auto headerMsg = po_message_create();
		po_message_set_msgid(headerMsg, "");
		po_message_set_msgstr(headerMsg, header.toStdString().c_str());
		po_message_insert(iterator, headerMsg);

		std::set<DictEntry> emittedEntries;
		for(const auto& entry : dictIt.value())
		{
			const DictEntry untransEntry{.comment = entry.comment, .english = entry.english, .translated = ""};
			if(emittedEntries.find(untransEntry) != emittedEntries.end()) continue;
			const auto msg = po_message_create();
			if(!entry.comment.empty())
				po_message_set_extracted_comments(msg, join(entry.comment).toStdString().c_str());
			po_message_set_msgid(msg, entry.english.toStdString().c_str());
			po_message_set_msgstr(msg, entry.translated.toStdString().c_str());
			po_message_insert(iterator, msg);
			emittedEntries.insert(untransEntry);
		}

		// Add untranslated markdown entries
		for(const auto& entry : allMarkdownSections)
		{
			if(const auto found = emittedEntries.find(entry); found == emittedEntries.end())
			{
				const auto msg = po_message_create();
				po_message_set_msgid(msg, entry.english.toStdString().c_str());
				if(!entry.comment.empty())
					po_message_set_extracted_comments(msg, join(entry.comment).toStdString().c_str());
				po_message_insert(iterator, msg);
				emittedEntries.insert(entry);
			}
		}

		po_message_iterator_free(iterator);
		po_xerror_handler handler = {gettextpo_xerror, gettextpo_xerror2};
		po_file_write(file, path.toStdString().c_str(), &handler);
		po_file_free(file);
	}
	return true;
}
