#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Run with python3 tests/test_timefmt.py after building tree."""

import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
import xml.etree.ElementTree as ET


TREE = Path(__file__).resolve().parents[1] / "tree"


class TimeFormatTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        directory = self.root / "subdir"
        directory.mkdir()
        file = directory / "file.txt"
        file.write_text("example\n")
        for path in (file, directory, self.root):
            os.utime(path, (946684800, 946684800))

    def output(self, mode, timefmt):
        env = dict(os.environ, LC_ALL="C", TZ="UTC", STDDATA_FD="0")
        return subprocess.run(
            [str(TREE), mode, "--timefmt", timefmt, str(self.root)],
            env=env, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        ).stdout

    def test_json_timefmt(self):
        def timestamps(entries):
            for entry in entries:
                if "time" in entry:
                    yield entry["time"]
                yield from timestamps(entry.get("contents", []))

        for suffix in ("-01-01", ' "UTC"', "\\UTC", "\nUTC\t", "&<UTC>"):
            with self.subTest(suffix=suffix):
                data = json.loads(self.output("-J", "%Y" + suffix))
                self.assertEqual(list(timestamps(data)), ["2000" + suffix] * 3)

    def test_xml_timefmt(self):
        for suffix in ("-01-01", ' "UTC"', "&<UTC>", "'UTC'", "\\UTC"):
            with self.subTest(suffix=suffix):
                data = ET.fromstring(self.output("-X", "%Y" + suffix))
                times = [entry.attrib["time"] for entry in data.iter()
                         if "time" in entry.attrib]
                self.assertEqual(times, ["2000" + suffix] * 3)


if __name__ == "__main__":
    unittest.main()
