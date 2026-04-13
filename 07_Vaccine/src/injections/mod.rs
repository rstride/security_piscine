use std::collections::HashMap;

use crate::cli::Method;
use crate::http::HttpClient;

pub mod boolean_based;
pub mod error_based;
pub mod time_based;

/// Common trait for all injection strategies.
pub trait Injection {
    fn name(&self) -> &'static str;

    /// Try to detect SQL injection vulnerability for `param`.
    /// Returns Some(confirmed_payload) if vulnerable, None otherwise.
    fn detect(
        &self,
        client: &HttpClient,
        url: &str,
        method: &Method,
        params: &HashMap<String, String>,
        param: &str,
    ) -> Option<String>;
}
