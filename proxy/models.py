# ABOUTME: Data model for normalized 3D printer status.
# ABOUTME: Shared schema between all printer adapters and the HTTP API.

from dataclasses import dataclass, field, asdict


@dataclass
class PrinterStatus:
    id: str = ""
    name: str = ""
    type: str = ""
    model: str = ""
    state: str = "offline"
    progress: int = 0
    job: str = ""
    time_remaining: int = 0
    nozzle_temp: int = 0
    nozzle_target: int = 0
    bed_temp: int = 0
    bed_target: int = 0
    error: str = ""

    def to_dict(self) -> dict:
        return asdict(self)
