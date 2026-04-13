use crate::models::DbEngine;

/// Fingerprint the database engine from an HTTP response body.
/// Looks for engine-specific error strings in the response.
pub fn fingerprint(body: &str) -> DbEngine {
    let lower = body.to_lowercase();

    if is_mysql(&lower) {
        DbEngine::MySQL
    } else if is_sqlite(&lower) {
        DbEngine::SQLite
    } else if is_postgresql(&lower) {
        DbEngine::PostgreSQL
    } else {
        DbEngine::Unknown
    }
}

/// Check whether a response body contains any SQL error strings.
pub fn contains_sql_error(body: &str) -> bool {
    let lower = body.to_lowercase();
    is_mysql(&lower) || is_sqlite(&lower) || is_postgresql(&lower) || is_generic_sql_error(&lower)
}

fn is_mysql(lower: &str) -> bool {
    lower.contains("you have an error in your sql syntax")
        || lower.contains("warning: mysql")
        || lower.contains("mysql_fetch")
        || lower.contains("mysql_num_rows")
        || lower.contains("mysql_query")
        || lower.contains("supplied argument is not a valid mysql")
        || lower.contains("mysqlt_query")
        || lower.contains("com.mysql.jdbc")
        || lower.contains("org.gjt.mm.mysql")
        || lower.contains("mysql server version for the right syntax")
        || lower.contains("[mysql]")
        || lower.contains("mysql error")
        || lower.contains("mysqli_")
}

fn is_sqlite(lower: &str) -> bool {
    lower.contains("sqlite_query")
    || lower.contains("sqlite3::query")
    || lower.contains("sqliteexception")
    || lower.contains("sqlite error")
    || lower.contains("sqlite3_step")
    || lower.contains("unrecognized token")
    || lower.contains("sqlite3")
        && (lower.contains("syntax error") || lower.contains("no such table"))
    || lower.contains("sqlitedatabase")
}

fn is_postgresql(lower: &str) -> bool {
    lower.contains("pg_query()")
        || lower.contains("pg_exec()")
        || lower.contains("pg::error")
        || lower.contains("error: syntax error at or near")
        || lower.contains("unterminated quoted string at or near")
        || lower.contains("postgresql")
        || lower.contains("psql")
        || lower.contains("org.postgresql")
        || lower.contains("pgsql")
}

fn is_generic_sql_error(lower: &str) -> bool {
    lower.contains("sql syntax")
        || lower.contains("unclosed quotation mark")
        || lower.contains("quoted string not properly terminated")
        || lower.contains("odbc microsoft access")
        || lower.contains("syntax error in query expression")
        || lower.contains("data type mismatch in criteria expression")
        || lower.contains("[microsoft][odbc")
        || lower.contains("microsoft ole db provider for sql server")
        || lower.contains("invalid sql statement")
        || lower.contains("error converting data type")
}
