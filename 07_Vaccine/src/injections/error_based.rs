use std::collections::HashMap;

use crate::cli::Method;
use crate::detection::contains_sql_error;
use crate::http::HttpClient;
use crate::injections::Injection;

const ERROR_PAYLOADS: &[&str] = &[
    "'",
    "\"",
    "`",
    "')",
    "'))",
    "\"))",
    "' OR '1'='1",
    "' OR 1=1--",
    "1'",
    "1\"",
    "\\",
    "' AND 1=1--",
    "'; --",
];

pub struct ErrorBased;

impl Injection for ErrorBased {
    fn name(&self) -> &'static str {
        "error-based"
    }

    fn detect(
        &self,
        client: &HttpClient,
        url: &str,
        method: &Method,
        params: &HashMap<String, String>,
        param: &str,
    ) -> Option<String> {
        let original = params.get(param).cloned().unwrap_or_default();

        for &payload in ERROR_PAYLOADS {
            let injected = format!("{}{}", original, payload);
            match client.send(url, method, params, param, &injected) {
                Ok((_, body, _)) => {
                    if contains_sql_error(&body) {
                        return Some(payload.to_string());
                    }
                }
                Err(_) => continue,
            }
        }
        None
    }
}
