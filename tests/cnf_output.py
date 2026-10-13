#!/usr/bin/env python3
"""Byte-compare Satsuma's sink and file output, including bypass and units."""
from pathlib import Path
import subprocess
import sys
import tempfile

driver, cli, satsuma = sys.argv[1:]
settings = ["--silent", "--add-reduced-as-unit", "--preprocess-cnf-unit",
            "--proof-dense-crossover", "60", "--component-limit", "500000",
            "--order-model-limit", "750000", "--dense-model-limit", "20000000"]
count = 0
with tempfile.TemporaryDirectory() as temporary:
    fixtures = [Path(satsuma) / "examples" / name for name in
                ("php-015-014.cnf", "ramsey-3-5-14.cnf", "tseitin-100-5.cnf")]
    for name, text in {
        "empty": "p cnf 3 0\n",
        "units": "p cnf 5 4\n1 0\n-2 0\n-1 3 0\n2 -4 5 0\n",
        "conflict": "p cnf 2 2\n1 0\n-1 0\n",
        "empty-clause": "p cnf 2 1\n0\n",
        "duplicates": "p cnf 3 4\n1 1 2 0\n2 1 0\n-1 1 3 0\n-2 3 0\n",
    }.items():
        path = Path(temporary) / f"{name}.cnf"
        path.write_text(text)
        fixtures.append(path)
    for path in fixtures:
        for mode in ("lex", "fix"):
            for bypass in (False, True):
                source_args = ["--full-skip-limit", "0"] if bypass else []
                sink_args = ["skip"] if bypass else []
                original = subprocess.run([cli, mode, str(path), *settings, *source_args],
                                          capture_output=True, timeout=30, check=True).stdout
                memory = subprocess.run([driver, mode, str(path), *sink_args],
                                        capture_output=True, timeout=30, check=True).stdout
                assert memory == original, (path, mode, bypass, memory[:100], original[:100])
                count += 1
print(f"{count} CNF output comparisons passed")
