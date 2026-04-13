use std::collections::HashMap;
use std::time::{Duration, Instant};

use reqwest::blocking::Client;
use reqwest::header::{HeaderMap, HeaderValue, COOKIE, USER_AGENT};

use crate::cli::Method;

#[derive(Debug)]
pub struct HttpError(pub String);

impl std::fmt::Display for HttpError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "HTTP error: {}", self.0)
    }
}

pub struct HttpClient {
    inner: Client,
}

impl HttpClient {
    pub fn new(cookie: Option<&str>) -> Self {
        let mut headers = HeaderMap::new();
        headers.insert(
            USER_AGENT,
            HeaderValue::from_static(
                "Mozilla/5.0 (X11; Linux x86_64; rv:109.0) Gecko/20100101 Firefox/115.0",
            ),
        );
        if let Some(cookie_str) = cookie {
            if let Ok(val) = HeaderValue::from_str(cookie_str) {
                headers.insert(COOKIE, val);
            }
        }

        let inner = Client::builder()
            .default_headers(headers)
            .cookie_store(true)
            .timeout(Duration::from_secs(15))
            .danger_accept_invalid_certs(true)
            .build()
            .expect("Failed to build HTTP client");

        Self { inner }
    }

    /// Send a request with `inject_param` set to `inject_value`.
    /// All other params keep their original values from `params`.
    /// Returns (status_code, body, elapsed_ms).
    pub fn send(
        &self,
        url: &str,
        method: &Method,
        params: &HashMap<String, String>,
        inject_param: &str,
        inject_value: &str,
    ) -> Result<(u16, String, u128), HttpError> {
        let mut modified = params.clone();
        modified.insert(inject_param.to_string(), inject_value.to_string());

        let start = Instant::now();

        let response = match method {
            Method::GET => {
                let built_url = build_url_with_params(url, &modified)
                    .map_err(|e| HttpError(e.to_string()))?;
                self.inner
                    .get(&built_url)
                    .send()
                    .map_err(|e| HttpError(e.to_string()))?
            }
            Method::POST => self
                .inner
                .post(url)
                .form(&modified)
                .send()
                .map_err(|e| HttpError(e.to_string()))?,
        };

        let elapsed = start.elapsed().as_millis();
        let status = response.status().as_u16();
        let body = response.text().map_err(|e| HttpError(e.to_string()))?;

        Ok((status, body, elapsed))
    }

    /// Send a raw request without parameter injection (for baseline).
    pub fn send_raw(&self, url: &str, method: &Method, params: &HashMap<String, String>) -> Result<(u16, String, u128), HttpError> {
        let start = Instant::now();

        let response = match method {
            Method::GET => {
                let built_url = build_url_with_params(url, params)
                    .map_err(|e| HttpError(e.to_string()))?;
                self.inner
                    .get(&built_url)
                    .send()
                    .map_err(|e| HttpError(e.to_string()))?
            }
            Method::POST => self
                .inner
                .post(url)
                .form(params)
                .send()
                .map_err(|e| HttpError(e.to_string()))?,
        };

        let elapsed = start.elapsed().as_millis();
        let status = response.status().as_u16();
        let body = response.text().map_err(|e| HttpError(e.to_string()))?;

        Ok((status, body, elapsed))
    }
}

fn build_url_with_params(
    base_url: &str,
    params: &HashMap<String, String>,
) -> Result<String, url::ParseError> {
    let mut parsed = url::Url::parse(base_url)?;
    {
        let mut pairs = parsed.query_pairs_mut();
        pairs.clear();
        for (k, v) in params {
            pairs.append_pair(k, v);
        }
    }
    Ok(parsed.to_string())
}

/// Parse parameters from a URL query string or a POST data string.
pub fn parse_params(input: &str) -> HashMap<String, String> {
    let mut map = HashMap::new();
    for pair in input.split('&') {
        if let Some((k, v)) = pair.split_once('=') {
            map.insert(
                urlencoding_decode(k),
                urlencoding_decode(v),
            );
        }
    }
    map
}

fn urlencoding_decode(s: &str) -> String {
    let replaced = s.replace('+', " ");
    let mut result = String::new();
    let mut chars = replaced.chars().peekable();
    while let Some(c) = chars.next() {
        if c == '%' {
            let h1 = chars.next().unwrap_or('0');
            let h2 = chars.next().unwrap_or('0');
            if let Ok(byte) = u8::from_str_radix(&format!("{}{}", h1, h2), 16) {
                result.push(byte as char);
            } else {
                result.push('%');
                result.push(h1);
                result.push(h2);
            }
        } else {
            result.push(c);
        }
    }
    result
}
