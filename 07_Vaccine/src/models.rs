use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "lowercase")]
pub enum DbEngine {
    MySQL,
    SQLite,
    PostgreSQL,
    Unknown,
}

impl std::fmt::Display for DbEngine {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            DbEngine::MySQL => write!(f, "mysql"),
            DbEngine::SQLite => write!(f, "sqlite"),
            DbEngine::PostgreSQL => write!(f, "postgresql"),
            DbEngine::Unknown => write!(f, "unknown"),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "kebab-case")]
pub enum InjectionType {
    ErrorBased,
    BooleanBased,
    TimeBased,
}

impl std::fmt::Display for InjectionType {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            InjectionType::ErrorBased => write!(f, "error-based"),
            InjectionType::BooleanBased => write!(f, "boolean-based"),
            InjectionType::TimeBased => write!(f, "time-based"),
        }
    }
}

#[derive(Debug, Clone)]
pub struct InjectionPoint {
    pub param: String,
    pub payload: String,
    pub injection_type: InjectionType,
    pub db_engine: DbEngine,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct TableDump {
    pub name: String,
    pub columns: Vec<String>,
    pub rows: Vec<Vec<String>>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct DbDump {
    pub name: String,
    pub tables: Vec<TableDump>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ParamResult {
    pub param: String,
    pub payload: String,
    pub injection_type: String,
    pub db_engine: String,
    pub databases: Vec<DbDump>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ScanResult {
    pub url: String,
    pub method: String,
    pub vulnerable_params: Vec<ParamResult>,
}
