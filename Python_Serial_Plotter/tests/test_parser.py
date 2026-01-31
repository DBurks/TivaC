import json
import pytest


# Mock function representing your data parsing logic
def parse_telemetry(payload):
    try:
        data = json.loads(payload)
        # Ensure 'temp' (temperature) and 'time' (timestamp) exist
        if "temp" in data and "time" in data:
            return data["temp"], data["time"]
    except json.JSONDecodeError:
        return None
    return None


def test_valid_json_with_extra_fields():
    """Verify that adding humidity/source doesn't break temperature extraction."""
    sample = '{"time": 101, "temp": 25.5, "humidity": 45.2, "source": "AHT20"}'
    temp, timestamp = parse_telemetry(sample)
    assert temp == 25.5
    assert timestamp == 101


def test_corrupted_json():
    """Verify the parser handles malformed strings gracefully."""
    sample = '{"time": 101, "temp": 25.'  # Incomplete JSON
    result = parse_telemetry(sample)
    assert result is None
