/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "SQLiteStatement.h"

#include "Logging.h"
#include "SQLValue.h"
#include <sqlite3.h>
#include <wtf/Assertions.h>
#include <wtf/Variant.h>
#include <wtf/text/StringView.h>

// SQLite 3.6.16 makes sqlite3_prepare_v2 automatically retry preparing the statement
// once if the database scheme has changed. We rely on this behavior.
#if SQLITE_VERSION_NUMBER < 3006016
#error SQLite version 3.6.16 or newer is required
#endif

namespace WebCore {

SQLiteStatement::SQLiteStatement(SQLiteDatabase& db, const String& sql)
    : m_database(db)
    , m_query(sql)
    , m_statement(0)
{
}

SQLiteStatement::~SQLiteStatement()
{
    finalize();
}

int SQLiteStatement::prepare()
{
    ASSERT(!m_isPrepared);

    LockHolder databaseLock(m_database.databaseMutex());

    CString query = m_query.stripWhiteSpace().utf8();
    
    LOG(SQLDatabase, "SQL - prepare - %s", query.data());

    // Pass the length of the string including the null character to sqlite3_prepare_v2;
    // this lets SQLite avoid an extra string copy.
    size_t lengthIncludingNullCharacter = query.length() + 1;

    const char* tail = nullptr;
    int error = sqlite3_prepare_v2(m_database.sqlite3Handle(), query.data(), lengthIncludingNullCharacter, &m_statement, &tail);

    if (error != SQLITE_OK)
        LOG(SQLDatabase, "sqlite3_prepare16 failed (%i)\n%s\n%s", error, query.data(), sqlite3_errmsg(m_database.sqlite3Handle()));

    if (tail && *tail)
        error = SQLITE_ERROR;

#if ASSERT_ENABLED
    m_isPrepared = error == SQLITE_OK;
#endif
    return error;
}

int SQLiteStatement::step()
{
    LockHolder databaseLock(m_database.databaseMutex());

    if (!m_statement)
        return SQLITE_OK;

    // The database needs to update its last changes count before each statement
    // in order to compute properly the lastChanges() return value.
    m_database.updateLastChangesCount();

    LOG(SQLDatabase, "SQL - step - %s", m_query.ascii().data());
    int error = sqlite3_step(m_statement);
    if (error != SQLITE_DONE && error != SQLITE_ROW) {
        LOG(SQLDatabase, "sqlite3_step failed (%i)\nQuery - %s\nError - %s", 
            error, m_query.ascii().data(), sqlite3_errmsg(m_database.sqlite3Handle()));
    }

    return error;
}
    
int SQLiteStatement::finalize()
{
#if ASSERT_ENABLED
    m_isPrepared = false;
#endif
    if (!m_statement)
        return SQLITE_OK;
    LOG(SQLDatabase, "SQL - finalize - %s", m_query.ascii().data());
    int result = sqlite3_finalize(m_statement);
    m_statement = 0;
    return result;
}

int SQLiteStatement::reset() 
{
    ASSERT(m_isPrepared);
    if (!m_statement)
        return SQLITE_OK;
    LOG(SQLDatabase, "SQL - reset - %s", m_query.ascii().data());
    int status = sqlite3_reset(m_statement);
    sqlite3_clear_bindings(m_statement);
    return status;
}

bool SQLiteStatement::executeCommand()
{
    if (!m_statement && prepare() != SQLITE_OK)
        return false;
    ASSERT(m_isPrepared);
    if (step() != SQLITE_DONE) {
        finalize();
        return false;
    }
    finalize();
    return true;
}

bool SQLiteStatement::returnsAtLeastOneResult()
{
    if (!m_statement && prepare() != SQLITE_OK)
        return false;
    ASSERT(m_isPrepared);
    if (step() != SQLITE_ROW) {
        finalize();
        return false;
    }
    finalize();
    return true;

}

int SQLiteStatement::bindBlob(int index, const void* blob, int size)
{
    ASSERT(m_isPrepared);
    ASSERT(index > 0);
    ASSERT(static_cast<unsigned>(index) <= bindParameterCount());
    ASSERT(blob || !size);
    ASSERT(size >= 0);

    if (!m_statement)
        return SQLITE_ERROR;

    return sqlite3_bind_blob(m_statement, index, blob, size, SQLITE_TRANSIENT);
}

int SQLiteStatement::bindBlob(int index, const String& text)
{
    // String::characters() returns 0 for the empty string, which SQLite
    // treats as a null, so we supply a non-null pointer for that case.
    auto upconvertedCharacters = StringView(text).upconvertedCharacters();
    UChar anyCharacter = 0;
    const UChar* characters;
    if (text.isEmpty() && !text.isNull())
        characters = &anyCharacter;
    else
        characters = upconvertedCharacters;

    return bindBlob(index, characters, text.length() * sizeof(UChar));
}

int SQLiteStatement::bindText(int index, const String& text)
{
    ASSERT(m_isPrepared);
    ASSERT(index > 0);
    ASSERT(static_cast<unsigned>(index) <= bindParameterCount());

    // String::characters() returns 0 for the empty string, which SQLite
    // treats as a null, so we supply a non-null pointer for that case.
    auto upconvertedCharacters = StringView(text).upconvertedCharacters();
    UChar anyCharacter = 0;
    const UChar* characters;
    if (text.isEmpty() && !text.isNull())
        characters = &anyCharacter;
    else
        characters = upconvertedCharacters;

    return sqlite3_bind_text16(m_statement, index, characters, sizeof(UChar) * text.length(), SQLITE_TRANSIENT);
}

int SQLiteStatement::bindInt(int index, int integer)
{
    ASSERT(m_isPrepared);
    ASSERT(index > 0);
    ASSERT(static_cast<unsigned>(index) <= bindParameterCount());

    return sqlite3_bind_int(m_statement, index, integer);
}

int SQLiteStatement::bindInt64(int index, int64_t integer)
{
    ASSERT(m_isPrepared);
    ASSERT(index > 0);
    ASSERT(static_cast<unsigned>(index) <= bindParameterCount());

    return sqlite3_bind_int64(m_statement, index, integer);
}

int SQLiteStatement::bindDouble(int index, double number)
{
    ASSERT(m_isPrepared);
    ASSERT(index > 0);
    ASSERT(static_cast<unsigned>(index) <= bindParameterCount());

    return sqlite3_bind_double(m_statement, index, number);
}

int SQLiteStatement::bindNull(int index)
{
    ASSERT(m_isPrepared);
    ASSERT(index > 0);
    ASSERT(static_cast<unsigned>(index) <= bindParameterCount());

    return sqlite3_bind_null(m_statement, index);
}

int SQLiteStatement::bindValue(int index, const SQLValue& value)
{
    return WTF::switchOn(value,
        [&] (const std::nullptr_t&) { return bindNull(index); },
        [&] (const String& string) { return bindText(index, string); },
        [&] (double number) { return bindDouble(index, number); }
    );
}

unsigned SQLiteStatement::bindParameterCount() const
{
    ASSERT(m_isPrepared);
    if (!m_statement)
        return 0;
    return sqlite3_bind_parameter_count(m_statement);
}

int SQLiteStatement::columnCount()
{
    ASSERT(m_isPrepared);
    if (!m_statement)
        return 0;
    return sqlite3_data_count(m_statement);
}

bool SQLiteStatement::isColumnNull(int col)
{
    ASSERT(col >= 0);
    if (!m_statement)
        if (prepareAndStep() != SQLITE_ROW)
            return false;
    if (columnCount() <= col)
        return false;

    return sqlite3_column_type(m_statement, col) == SQLITE_NULL;
}

bool SQLiteStatement::isColumnDeclaredAsBlob(int col)
{
    ASSERT(col >= 0);
    if (!m_statement) {
        if (prepare() != SQLITE_OK)
            return false;
    }
    return equalLettersIgnoringASCIICase(StringView(sqlite3_column_decltype(m_statement, col)), "blob");
}

String SQLiteStatement::getColumnName(int col)
{
    ASSERT(col >= 0);
    if (!m_statement)
        if (prepareAndStep() != SQLITE_ROW)
            return String();
    if (columnCount() <= col)
        return String();
    return String(reinterpret_cast<const UChar*>(sqlite3_column_name16(m_statement, col)));
}

SQLValue SQLiteStatement::getColumnValue(int col)
{
    ASSERT(col >= 0);
    if (!m_statement)
        if (prepareAndStep() != SQLITE_ROW)
            return nullptr;
    if (columnCount() <= col)
        return nullptr;

    // SQLite is typed per value. optional column types are
    // "(mostly) ignored"
    sqlite3_value* value = sqlite3_column_value(m_statement, col);
    switch (sqlite3_value_type(value)) {
        case SQLITE_INTEGER:    // SQLValue and JS don't represent integers, so use FLOAT -case
        case SQLITE_FLOAT:
            return sqlite3_value_double(value);
        case SQLITE_BLOB:       // SQLValue and JS don't represent blobs, so use TEXT -case
        case SQLITE_TEXT: {
            const UChar* string = reinterpret_cast<const UChar*>(sqlite3_value_text16(value));
            return StringImpl::create8BitIfPossible(string);
        }
        case SQLITE_NULL:
            return nullptr;
        default:
            break;
    }

    ASSERT_NOT_REACHED();
    return nullptr;
}

String SQLiteStatement::getColumnText(int col)
{
    ASSERT(col >= 0);
    if (!m_statement)
        if (prepareAndStep() != SQLITE_ROW)
            return String();
    if (columnCount() <= col)
        return String();
    return String(reinterpret_cast<const UChar*>(sqlite3_column_text16(m_statement, col)), sqlite3_column_bytes16(m_statement, col) / sizeof(UChar));
}
    
double SQLiteStatement::getColumnDouble(int col)
{
    ASSERT(col >= 0);
    if (!m_statement)
        if (prepareAndStep() != SQLITE_ROW)
            return 0.0;
    if (columnCount() <= col)
        return 0.0;
    return sqlite3_column_double(m_statement, col);
}

int SQLiteStatement::getColumnInt(int col)
{
    ASSERT(col >= 0);
    if (!m_statement)
        if (prepareAndStep() != SQLITE_ROW)
            return 0;
    if (columnCount() <= col)
        return 0;
    return sqlite3_column_int(m_statement, col);
}

int64_t SQLiteStatement::getColumnInt64(int col)
{
    ASSERT(col >= 0);
    if (!m_statement)
        if (prepareAndStep() != SQLITE_ROW)
            return 0;
    if (columnCount() <= col)
        return 0;
    return sqlite3_column_int64(m_statement, col);
}

String SQLiteStatement::getColumnBlobAsString(int col)
{
    ASSERT(col >= 0);

    if (!m_statement && prepareAndStep() != SQLITE_ROW)
        return String();

    if (columnCount() <= col)
        return String();

    const void* blob = sqlite3_column_blob(m_statement, col);
    if (!blob)
        return emptyString();

    int size = sqlite3_column_bytes(m_statement, col);
    if (size < 0)
        return String();

    ASSERT(!(size % sizeof(UChar)));
    return String(static_cast<const UChar*>(blob), size / sizeof(UChar));
}

void SQLiteStatement::getColumnBlobAsVector(int col, Vector<char>& result)
{
    ASSERT(col >= 0);

    if (!m_statement && prepareAndStep() != SQLITE_ROW) {
        result.clear();
        return;
    }

    if (columnCount() <= col) {
        result.clear();
        return;
    }

    const void* blob = sqlite3_column_blob(m_statement, col);
    if (!blob) {
        result.clear();
        return;
    }
        
    int size = sqlite3_column_bytes(m_statement, col);
    result.resize((size_t)size);
    for (int i = 0; i < size; ++i)
        result[i] = (static_cast<const unsigned char*>(blob))[i];
}

void SQLiteStatement::getColumnBlobAsVector(int col, Vector<uint8_t>& result)
{
    ASSERT(col >= 0);

    if (!m_statement && prepareAndStep() != SQLITE_ROW) {
        result.clear();
        return;
    }

    if (columnCount() <= col) {
        result.clear();
        return;
    }

    const void* blob = sqlite3_column_blob(m_statement, col);
    if (!blob) {
        result.clear();
        return;
    }
        
    int size = sqlite3_column_bytes(m_statement, col);
    result.resize((size_t)size);
    for (int i = 0; i < size; ++i)
        result[i] = (static_cast<const uint8_t*>(blob))[i];
}

bool SQLiteStatement::returnTextResults(int col, Vector<String>& v)
{
    ASSERT(col >= 0);

    v.clear();

    if (m_statement)
        finalize();
    if (prepare() != SQLITE_OK)
        return false;

    while (step() == SQLITE_ROW)
        v.append(getColumnText(col));
    bool result = true;
    if (m_database.lastError() != SQLITE_DONE) {
        result = false;
        LOG(SQLDatabase, "Error reading results from database query %s", m_query.ascii().data());
    }
    finalize();
    return result;
}

bool SQLiteStatement::returnIntResults(int col, Vector<int>& v)
{
    v.clear();

    if (m_statement)
        finalize();
    if (prepare() != SQLITE_OK)
        return false;

    while (step() == SQLITE_ROW)
        v.append(getColumnInt(col));
    bool result = true;
    if (m_database.lastError() != SQLITE_DONE) {
        result = false;
        LOG(SQLDatabase, "Error reading results from database query %s", m_query.ascii().data());
    }
    finalize();
    return result;
}

bool SQLiteStatement::returnInt64Results(int col, Vector<int64_t>& v)
{
    v.clear();

    if (m_statement)
        finalize();
    if (prepare() != SQLITE_OK)
        return false;

    while (step() == SQLITE_ROW)
        v.append(getColumnInt64(col));
    bool result = true;
    if (m_database.lastError() != SQLITE_DONE) {
        result = false;
        LOG(SQLDatabase, "Error reading results from database query %s", m_query.ascii().data());
    }
    finalize();
    return result;
}

bool SQLiteStatement::returnDoubleResults(int col, Vector<double>& v)
{
    v.clear();

    if (m_statement)
        finalize();
    if (prepare() != SQLITE_OK)
        return false;

    while (step() == SQLITE_ROW)
        v.append(getColumnDouble(col));
    bool result = true;
    if (m_database.lastError() != SQLITE_DONE) {
        result = false;
        LOG(SQLDatabase, "Error reading results from database query %s", m_query.ascii().data());
    }
    finalize();
    return result;
}

bool SQLiteStatement::isExpired()
{
    return !m_statement;
}

} // namespace WebCore
