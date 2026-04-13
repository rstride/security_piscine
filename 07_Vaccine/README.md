# vaccine

A SQL injection detection and exploitation tool written in Rust.

## Overview

`vaccine` scans a target URL for SQL injection vulnerabilities. When a vulnerable parameter is found, it automatically:

- Identifies the database engine (MySQL, SQLite, PostgreSQL)
- Extracts database names, table names, and column names
- Dumps the complete contents of all tables
- Saves results to a JSON file

> **Legal notice**: Only use this tool against systems you own or have explicit written permission to test.

## Features

- **Injection methods**: Error-based, Boolean-based, Time-based
- **Database engines**: MySQL/MariaDB, SQLite, PostgreSQL
- **HTTP methods**: GET and POST
- **Persistent output**: Results are appended to a JSON file on each run
- **Cookie support**: Pass session cookies for authenticated targets (e.g. DVWA)

## Build

Requires Rust toolchain (`cargo`).

```bash
make
```

This compiles in release mode and copies the `vaccine` binary to the current directory.

```bash
make clean   # remove build artifacts
make fclean  # remove build artifacts + binary
make re      # full rebuild
```

## Usage

```
./vaccine [-o output_file] [-X GET|POST] [-c cookie] [-d post_data] URL
```

### Options

| Flag | Description | Default |
|------|-------------|---------|
| `-X` | HTTP method (`GET` or `POST`) | `GET` |
| `-o` | Output file path (JSON) | `output.json` |
| `-c` | Cookie string for authenticated sessions | — |
| `-d` | POST body data (`key=val&key2=val2`) | — |

### Examples

**Basic GET scan:**
```bash
./vaccine "http://target.com/page.php?id=1"
```

**GET scan with output file:**
```bash
./vaccine -o results.json "http://target.com/page.php?id=1"
```

**POST scan with form data:**
```bash
./vaccine -X POST -d "username=admin&password=test" "http://target.com/login.php"
```

**Authenticated scan (DVWA):**
```bash
./vaccine -c "PHPSESSID=abc123; security=low" \
  "http://localhost:8080/vulnerabilities/sqli/?id=1&Submit=Submit"
```

## Test Environment (DVWA)

A Docker Compose file is provided to spin up [DVWA](https://github.com/digininja/DVWA) locally:

```bash
docker-compose -f docker_compose.yml up -d
```

DVWA is available at `http://localhost:8080`.

**Setup steps:**
1. Open `http://localhost:8080` in a browser
2. Log in with `admin` / `password`
3. Go to **DVWA Security** and set level to **Low**
4. Go to **Setup / Reset DB** and click **Create / Reset Database**
5. Copy your `PHPSESSID` cookie from the browser's developer tools

**Run against DVWA:**
```bash
./vaccine -c "PHPSESSID=<your_session>; security=low" \
  "http://localhost:8080/vulnerabilities/sqli/?id=1&Submit=Submit"
```

## Output Format

Results are saved as a JSON array. Each scan appends to the existing file.

```json
[
  {
    "url": "http://localhost:8080/vulnerabilities/sqli/?id=1&Submit=Submit",
    "method": "GET",
    "vulnerable_params": [
      {
        "param": "id",
        "payload": "'",
        "injection_type": "error-based",
        "db_engine": "mysql",
        "databases": [
          {
            "name": "dvwa",
            "tables": [
              {
                "name": "users",
                "columns": ["user_id", "first_name", "last_name", "user", "password"],
                "rows": [
                  ["1", "admin", "admin", "admin", "5f4dcc3b5aa765d61d8327deb882cf99"]
                ]
              }
            ]
          }
        ]
      }
    ]
  }
]
```

## How It Works

1. **Parameter extraction** — Parses the URL query string (GET) or `-d` data (POST) to find injectable parameters.
2. **Detection** — For each parameter, three strategies are tried in order:
   - **Error-based**: Injects characters that trigger DB syntax errors; detects engine-specific error strings in the response.
   - **Boolean-based**: Compares responses for `AND 1=1` (true) vs `AND 1=2` (false) conditions; a significant difference indicates injection.
   - **Time-based**: Injects `SLEEP(3)` / `pg_sleep(3)` / `randomblob()` and measures response time.
3. **DB fingerprinting** — Error messages are matched against known MySQL, SQLite, and PostgreSQL signatures.
4. **Extraction** — Uses `ORDER BY` to find the column count, then `UNION SELECT` to extract data row by row, using `VSTART`/`VEND` markers to locate values in the HTML response.
5. **Storage** — Results are merged with any existing output file and written as pretty-printed JSON.
