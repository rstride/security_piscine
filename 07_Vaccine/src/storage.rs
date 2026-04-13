use std::fs;
use std::path::Path;

use crate::models::ScanResult;

const DEFAULT_OUTPUT: &str = "output.json";

pub fn save(result: &ScanResult, path: Option<&str>) {
    let output_path = path.unwrap_or(DEFAULT_OUTPUT);

    // Load existing results from file, or start with empty vec
    let mut all_results: Vec<ScanResult> = if Path::new(output_path).exists() {
        match fs::read_to_string(output_path) {
            Ok(content) => serde_json::from_str(&content).unwrap_or_default(),
            Err(_) => vec![],
        }
    } else {
        vec![]
    };

    all_results.push(result.clone());

    match serde_json::to_string_pretty(&all_results) {
        Ok(json) => match fs::write(output_path, &json) {
            Ok(_) => println!("\n[+] Results saved to: {}", output_path),
            Err(e) => eprintln!("[!] Failed to write output file '{}': {}", output_path, e),
        },
        Err(e) => eprintln!("[!] Failed to serialize results: {}", e),
    }
}
