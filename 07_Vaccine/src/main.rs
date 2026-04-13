use clap::Parser;

mod cli;
mod detection;
mod extractor;
mod http;
mod injections;
mod models;
mod scanner;
mod storage;

fn main() {
    let args = cli::Args::parse();

    println!("╔══════════════════════════════════════╗");
    println!("║          vaccine - SQL injector       ║");
    println!("╚══════════════════════════════════════╝");

    let client = http::HttpClient::new(args.cookie.as_deref());
    let result = scanner::scan(&client, &args);

    if result.vulnerable_params.is_empty() {
        println!("\n[-] No SQL injection vulnerabilities found.");
    } else {
        println!(
            "\n[+] Found {} vulnerable parameter(s).",
            result.vulnerable_params.len()
        );
    }

    storage::save(&result, args.output.as_deref());
}
