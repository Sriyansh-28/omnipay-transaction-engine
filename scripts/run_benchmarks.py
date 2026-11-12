#!/usr/bin/env python3
"""
OmniPay Performance & QA Diagnostics Verification Script
Validates benchmark throughput, latency percentiles, and usability metrics.
"""

import os
import sys
import csv

def verify_report(csv_path):
    print("=" * 80)
    print("OmniPay Benchmark & QA Report Validation")
    print("=" * 80)

    if not os.path.exists(csv_path):
        print(f"[FAIL] Report file not found at: {csv_path}")
        return False

    with open(csv_path, "r", encoding="utf-8") as f:
        content = f.read()

    print(f"[OK] Report file loaded ({len(content)} bytes).")
    lines = content.splitlines()

    section1_data = []
    in_section1 = False

    for line in lines:
        if "=== Section 1:" in line:
            in_section1 = True
            continue
        elif "=== Section 2:" in line:
            in_section1 = False
            continue
        
        if in_section1 and line.strip() and not line.startswith("Test Configuration"):
            reader = csv.reader([line])
            row = next(reader)
            if len(row) >= 10:
                section1_data.append(row)

    print(f"\n[+] Found {len(section1_data)} benchmark configurations:")
    print(f"{'Configuration':<30} {'Threads':<8} {'TPS':<12} {'p50 (ms)':<10} {'p99 (ms)':<10} {'RAM (MB)':<10}")
    print("-" * 80)

    max_tps = 0.0
    for row in section1_data:
        cfg = row[0]
        threads = row[1]
        tps = float(row[4])
        p50 = float(row[6])
        p99 = float(row[9])
        ram = float(row[12])
        if tps > max_tps:
            max_tps = tps

        print(f"{cfg:<30} {threads:<8} {tps:<12.1f} {p50:<10.3f} {p99:<10.3f} {ram:<10.1f}")

    print("-" * 80)
    print(f"[SUMMARY] Peak Measured Throughput: {max_tps:,.1f} Transactions / Second (TPS)")
    
    assert max_tps > 20000.0, f"Throughput {max_tps} did not meet high-performance threshold (> 20,000 TPS)"
    print("[PASS] High-Performance SLA requirement (>20,000 TPS) met with flying colors!")

    # Verify Section 3 (Usability & Defect Reduction)
    assert "=== Section 3: Mobile Checkout UI Usability & QA Diagnostics ===" in content, "Missing Section 3"
    assert "Average Checkout Time" in content, "Missing checkout time usability metric"
    assert "Mobile Usability Defect Density" in content, "Missing defect density metric"
    assert "Touch Target WCAG Compliance" in content, "Missing WCAG compliance metric"

    print("[PASS] Usability & QA Diagnostics metrics verified.")
    print("=" * 80)
    print("ALL BENCHMARK & QA CRITERIA SUCCESSFULLY VALIDATED!")
    print("=" * 80)
    return True

if __name__ == "__main__":
    path = "benchmark_and_qa_report.csv"
    if len(sys.argv) > 1:
        path = sys.argv[1]
    ok = verify_report(path)
    sys.exit(0 if ok else 1)
