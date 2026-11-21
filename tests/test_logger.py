import unittest
from logger.data_logger import parse_row


class TestTelemetryParser(unittest.TestCase):
    def test_valid_zero_is_preserved(self):
        row = parse_row("CSV_ROW,0.000,0.00,0.00,0.000,25.00,0")
        self.assertEqual(row["voltage_V"], "0.000")
        self.assertEqual(row["adc0"], "0")
        self.assertEqual(row["temperature_C"], "25.00")

    def test_sensor_failure_sentinel_becomes_blank(self):
        row = parse_row("CSV_ROW,-999.000,-999.00,-999.00,0.000,-999.00,-1")
        for field in ["voltage_V", "current_mA", "power_mW", "temperature_C", "adc0"]:
            self.assertEqual(row[field], "")

    def test_wrong_prefix_ignored(self):
        self.assertIsNone(parse_row("debug,1,2,3,4,5,6"))

    def test_wrong_column_count_rejected(self):
        with self.assertRaises(ValueError):
            parse_row("CSV_ROW,1,2")


if __name__ == "__main__":
    unittest.main()
