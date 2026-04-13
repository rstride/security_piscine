use clap::{Parser, ValueEnum};

#[derive(Debug, Clone, PartialEq, ValueEnum)]
pub enum Method {
    GET,
    POST,
}

impl std::fmt::Display for Method {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Method::GET => write!(f, "GET"),
            Method::POST => write!(f, "POST"),
        }
    }
}

#[derive(Parser, Debug)]
#[command(
    name = "vaccine",
    about = "SQL injection detection and exploitation tool",
    long_about = "vaccine scans a target URL for SQL injection vulnerabilities,\nidentifies the database engine, and extracts database structure and data."
)]
pub struct Args {
    /// Target URL to scan (include parameters in query string for GET)
    pub url: String,

    /// HTTP request method
    #[arg(short = 'X', long = "method", default_value = "GET", value_enum)]
    pub method: Method,

    /// Output file for results (JSON format)
    #[arg(short = 'o', long = "output")]
    pub output: Option<String>,

    /// Cookie string to include in requests (e.g. "PHPSESSID=abc; security=low")
    #[arg(short = 'c', long = "cookie")]
    pub cookie: Option<String>,

    /// POST data parameters (e.g. "user=admin&pass=test")
    #[arg(short = 'd', long = "data")]
    pub data: Option<String>,
}
