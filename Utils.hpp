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
#include <QString>

inline const QString translatorsCommentPrefix = "TRANSLATORS:";
std::vector<int> parseReferences(const QString& inStr);
QString formatReferences(const std::vector<int>& refs);
QString jsonEscape(const QString& string, bool warnAboutSpecialChars = false);
inline QString jsonEscapeAndWarn(const QString& string) { return jsonEscape(string, true); }
