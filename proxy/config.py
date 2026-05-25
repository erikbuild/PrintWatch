# ABOUTME: YAML configuration loader for the PrintWatch proxy.
# ABOUTME: Validates printer entries and proxy settings from config.yaml.

from dataclasses import dataclass
from typing import Optional
import yaml


@dataclass
class PrinterConfig:
    id: str
    name: str
    type: str
    url: str
    username: str = ""
    password: str = ""
    api_key: str = ""


@dataclass
class ProxyConfig:
    host: str = "0.0.0.0"
    port: int = 8080
    poll_interval: int = 10
    printers: list[PrinterConfig] = None

    def __post_init__(self):
        if self.printers is None:
            self.printers = []


def load_config(path: str) -> ProxyConfig:
    with open(path) as f:
        raw = yaml.safe_load(f)

    proxy_raw = raw.get("proxy", {})
    config = ProxyConfig(
        host=proxy_raw.get("host", "0.0.0.0"),
        port=proxy_raw.get("port", 8080),
        poll_interval=proxy_raw.get("poll_interval", 10),
    )

    for p in raw.get("printers", []):
        if not p.get("id") or not p.get("type") or not p.get("url"):
            raise ValueError(f"Printer entry missing required fields: {p}")
        if p["type"] not in ("prusalink", "moonraker"):
            raise ValueError(f"Unknown printer type: {p['type']}")
        config.printers.append(PrinterConfig(
            id=p["id"],
            name=p.get("name", p["id"]),
            type=p["type"],
            url=p["url"].rstrip("/"),
            username=p.get("username", ""),
            password=p.get("password", ""),
            api_key=p.get("api_key", ""),
        ))

    return config
