#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
license_replacer.py

This script scans a root directory (and its subdirectories) for source files
(of types C/C++, Java, PHP, C#, Python) and replaces their existing license
headers with the company’s Apache 2.0 license, or adds a new header if none
is present. Supports excluding directories, individual files, or wildcard
patterns (e.g., "*Trace.h") as listed in except_dir.txt. Can run in “test”
mode (––test) to report actions without making any file changes. Logging and
summary statistics are generated accordingly. Replaced and added files are
logged in detail and grouped by their containing directory. The per-directory
summary is written to license_replace_summary.log. A progress bar is displayed
during processing. Finally, the local "./LICENSE" file will replace the
destination's LICENSE under the root directory (skipped in test mode).
"""

import os
import sys
import time
import re
import fnmatch
import shutil
import argparse

def load_template(path):
    """Read a license template file and return its Unicode contents."""
    try:
        with open(path, 'r') as f:
            return f.read().decode('utf-8')
    except Exception as e:
        sys.stderr.write("Error: could not read template file '%s': %s\n" % (path, str(e)))
        sys.exit(1)

def load_excludes(path, root_dir):
    """
    Read except_dir.txt and categorize entries into:
      - excl_dirs: exact directories to exclude
      - excl_files: exact files to exclude
      - excl_patterns: wildcard patterns (relative paths) to exclude

    Each line is trimmed. If it contains '*' or '?', it's treated as a wildcard:
    store the normalized relative pattern in excl_patterns. Otherwise, combine
    with root_dir: if it's an existing directory, add to excl_dirs; if it's an
    existing file, add to excl_files; else warn and skip.
    """
    excl_dirs = []
    excl_files = []
    excl_patterns = []
    try:
        with open(path, 'r') as f:
            for line in f:
                rel = line.strip()
                if not rel:
                    continue
                normalized = rel.replace('/', os.sep).replace('\\', os.sep)
                abs_path = os.path.normpath(os.path.join(root_dir, normalized))
                if '*' in rel or '?' in rel:
                    excl_patterns.append(normalized)
                elif os.path.isdir(abs_path):
                    excl_dirs.append(abs_path)
                elif os.path.isfile(abs_path):
                    excl_files.append(abs_path)
                else:
                    sys.stderr.write("Warning: exclude entry '%s' does not exist, skipping.\n" % abs_path)
    except Exception as e:
        sys.stderr.write("Error: could not read exclude-file '%s': %s\n" % (path, str(e)))
        sys.exit(1)
    return excl_dirs, excl_files, excl_patterns

def is_under_excluded_dir(path, excl_dirs):
    """
    Return True if 'path' is equal to or inside any directory in excl_dirs.
    """
    path = os.path.normpath(path)
    for d in excl_dirs:
        if path == d or path.startswith(d + os.sep):
            return True
    return False

def format_date(timestamp):
    """Format a timestamp (epoch seconds) as DD/MM/YYYY."""
    return time.strftime("%d/%m/%Y", time.localtime(timestamp))

def process_file(filepath, template1, template2, counters, error_list,
                 added_list, replaced_list, test_mode):
    """
    Check one file for an existing header. If found, replace it; otherwise add new header.
    Update counters: 'scanned', 'replaced', 'added', 'skipped', 'failed'.
    - Append (filepath, error_message) to error_list on read/write error.
    - Append filepath to added_list or replaced_list when determined.
    - If not test_mode, perform actual file write.
    Returns one of: 'skipped', 'replaced', 'added', or 'failed'.
    """
    counters['scanned'] += 1

    try:
        with open(filepath, 'r') as f:
            text = f.read().decode('utf-8', 'ignore')
    except Exception as e:
        counters['failed'] += 1
        error_list.append((filepath, "Could not read file: %s" % str(e)))
        return 'failed'

    # 1) If file contains “Licensed under the Apache License”, skip
    if "Licensed under the Apache License" in text:
        counters['skipped'] += 1
        return 'skipped'

    # 2) Check for /* ... */ at top
    stripped = text.lstrip()
    has_header = stripped.startswith("/*")
    header_end_idx = -1
    if has_header:
        idx = text.find("*/")
        if idx >= 0:
            header_end_idx = idx + 2
        else:
            header_end_idx = -1
            has_header = False

    if has_header and header_end_idx > 0:
        header = text[:header_end_idx]

        # Extract [Other]
        other_text = ""
        pos = header.find("Source File Name")
        if pos >= 0:
            other_text = header[pos:header_end_idx - 2].rstrip('\r\n')
            lines = other_text.splitlines()
            while lines and re.match(r'^\s*\*+\s*$', lines[-1]):
                lines.pop()
            other_text = "\n".join(lines)

        new_header = template1.replace("[Other]", other_text)
        if not new_header.endswith("\n"):
            new_header += "\n"

        rest = text[header_end_idx:]
        rest = rest.lstrip('\r\n')
        new_content = new_header + rest

        counters['replaced'] += 1
        replaced_list.append(filepath)

        if not test_mode:
            try:
                with open(filepath, 'w') as f:
                    f.write(new_content.encode('utf-8'))
            except Exception as e:
                counters['failed'] += 1
                error_list.append((filepath, "Failed writing replaced content: %s" % str(e)))
                return 'failed'
        return 'replaced'
    else:
        # No valid header, add new header
        basename = os.path.basename(filepath)
        try:
            mtime = os.path.getmtime(filepath)
            date_str = format_date(mtime)
        except Exception:
            date_str = ""

        new_header = template2.replace("[Source File Name]", basename)
        new_header = new_header.replace("[DD/MM/YYYY]", date_str)
        if not new_header.endswith("\n"):
            new_header += "\n"
        new_content = new_header + text

        counters['added'] += 1
        added_list.append(filepath)

        if not test_mode:
            try:
                with open(filepath, 'w') as f:
                    f.write(new_content.encode('utf-8'))
            except Exception as e:
                counters['failed'] += 1
                error_list.append((filepath, "Failed writing new header: %s" % str(e)))
                return 'failed'
        return 'added'

def print_progress(current, total, bar_length=40):
    """
    Display a progress bar in the terminal.
    current:  zero-based index (0 to total-1)
    total:    total number of items
    """
    percent = float(current + 1) / total
    filled = int(bar_length * percent)
    bar = '#' * filled + '-' * (bar_length - filled)
    sys.stdout.write('\rProcessing: [%s] %d/%d (%d%%)' %
                     (bar, current + 1, total, int(percent * 100)))
    sys.stdout.flush()

def main():
    parser = argparse.ArgumentParser(
        description="Batch replace/add license headers in source files with Apache 2.0."
    )
    parser.add_argument(
        "-r", "--root", dest="root_dir",
        default=os.path.normpath(os.path.join(os.getcwd(), "../../")),
        help="Root directory to scan (optional; default '../../')."
    )
    parser.add_argument(
        "-e", "--except-file", dest="except_file",
        default=os.path.normpath(os.path.join(os.getcwd(), "except_dir.txt")),
        help="Exclude-list file (one relative path or wildcard pattern per line) "
             "(optional; default './except_dir.txt')."
    )
    parser.add_argument(
        "-t", "--template", dest="template1",
        default=os.path.normpath(os.path.join(os.getcwd(), "license_template.txt")),
        help="License template file (license_template.txt) "
             "(optional; default './license_template.txt')."
    )
    parser.add_argument(
        "-a", "--alt-template", dest="template2",
        default=os.path.normpath(os.path.join(os.getcwd(), "license_template_2.txt")),
        help="Template for files without a header (license_template_2.txt) "
             "(optional; default './license_template_2.txt')."
    )
    parser.add_argument(
        "-l", "--log", dest="log_file",
        default=os.path.normpath(os.path.join(os.getcwd(), "license_replace.log")),
        help="Error log (and overall summary) output file (optional; default './license_replace.log')."
    )
    parser.add_argument(
        "--test", action="store_true", dest="test_mode",
        help="Test mode: identify files that would be replaced, added, or skipped, without writing."
    )

    args = parser.parse_args()

    root_dir       = os.path.normpath(args.root_dir)
    except_file    = os.path.normpath(args.except_file)
    template1_path = os.path.normpath(args.template1)
    template2_path = os.path.normpath(args.template2)
    log_path       = os.path.normpath(args.log_file)
    test_mode      = args.test_mode

    if not os.path.isdir(root_dir):
        sys.stderr.write("Error: root directory '%s' does not exist.\n" % root_dir)
        sys.exit(1)

    template1 = load_template(template1_path)
    template2 = load_template(template2_path)
    excl_dirs, excl_files, excl_patterns = load_excludes(except_file, root_dir)
    except_dir_count     = len(excl_dirs)
    except_file_count    = len(excl_files)
    except_pattern_count = len(excl_patterns)

    counters = {
        'scanned':  0,
        'replaced': 0,
        'added':    0,
        'skipped':  0,
        'failed':   0
    }
    error_list    = []
    added_list    = []
    replaced_list = []

    # Supported extensions: C/C++, Java, PHP, C#, Python
    exts = ('.c', '.cpp', '.h', '.hpp', '.java', '.php', '.cs', '.py')

    # First, gather all candidate files to process
    candidates = []
    for current_root, dirs, files in os.walk(root_dir):
        current_root = os.path.normpath(current_root)

        if is_under_excluded_dir(current_root, excl_dirs):
            dirs[:] = []
            continue

        for fname in files:
            fpath = os.path.join(current_root, fname)

            if fpath in excl_files:
                continue

            rel_path = os.path.normpath(os.path.relpath(fpath, root_dir))
            skip_by_pattern = False
            for pat in excl_patterns:
                if fnmatch.fnmatch(rel_path, pat):
                    skip_by_pattern = True
                    break
            if skip_by_pattern:
                continue

            if not fname.lower().endswith(exts):
                continue

            candidates.append(fpath)

    total_files = len(candidates)
    if total_files == 0:
        print("No files to process.")
    else:
        # Process each candidate with a progress bar
        for idx, fpath in enumerate(candidates):
            print_progress(idx, total_files)
            process_file(fpath, template1, template2, counters,
                         error_list, added_list, replaced_list, test_mode)
        sys.stdout.write('\n')

    # At this point, handle replacing the root LICENSE
    local_license = os.path.join(os.getcwd(), "LICENSE")
    dest_license = os.path.join(root_dir, "LICENSE")
    license_action = None
    if os.path.isfile(local_license):
        if test_mode:
            license_action = "Would copy local LICENSE to: %s" % dest_license
        else:
            try:
                shutil.copyfile(local_license, dest_license)
                license_action = "Copied local LICENSE to: %s" % dest_license
            except Exception as e:
                license_action = "Failed copying LICENSE: %s" % str(e)
                counters['failed'] += 1
                error_list.append((dest_license, "LICENSE copy error: %s" % str(e)))
    else:
        license_action = "Local LICENSE not found; skipped root LICENSE copy."
        # Not a failure; just skip

    # Build per-directory summary
    dir_summary = {}
    def accumulate(list_of_files, key):
        for fp in list_of_files:
            dirpath = os.path.normpath(os.path.dirname(fp))
            rel_dir = os.path.normpath(os.path.relpath(dirpath, root_dir))
            if rel_dir == ".":
                rel_dir = os.sep
            if rel_dir not in dir_summary:
                dir_summary[rel_dir] = {'added': 0, 'replaced': 0}
            dir_summary[rel_dir][key] += 1

    accumulate(added_list, 'added')
    accumulate(replaced_list, 'replaced')

    # Build aligned overall summary
    summary_lines = []
    summary_lines.append("===== License Replacement Summary =====")
    summary_lines.append("Root directory               : %s" % root_dir)
    summary_lines.append("Excluded directories count   : %d" % except_dir_count)
    summary_lines.append("Excluded files count         : %d" % except_file_count)
    summary_lines.append("Excluded patterns count      : %d" % except_pattern_count)
    summary_lines.append("Total files scanned          : %d" % counters['scanned'])
    summary_lines.append("  Skipped (already Apache)     : %d" % counters['skipped'])
    summary_lines.append("  Replaced existing headers    : %d" % counters['replaced'])
    summary_lines.append("  Added new headers            : %d" % counters['added'])
    summary_lines.append("Failed files                 : %d" % counters['failed'])
    summary_lines.append("Mode                          : %s" % ("TEST (no writes)" if test_mode else "EXECUTE"))
    summary_lines.append("Root LICENSE action           : %s" % license_action)
    overall_summary_text = "\n".join(summary_lines)

    # Build per-directory summary text
    dir_summary_lines = []
    dir_summary_lines.append("=== Per-Directory Summary ===")
    if dir_summary:
        for rel_dir in sorted(dir_summary):
            added_count = dir_summary[rel_dir]['added']
            replaced_count = dir_summary[rel_dir]['replaced']
            dir_summary_lines.append(
                "  %-30s : replaced=%-3d added=%-3d" %
                (rel_dir, replaced_count, added_count)
            )
    else:
        dir_summary_lines.append("  (No files were replaced or added)")
    dir_summary_text = "\n".join(dir_summary_lines)

    # Write to main log: Errors, Replaced list, Added list, Overall summary
    try:
        with open(log_path, 'w') as logf:
            if error_list:
                logf.write("=== Errors ===\n")
                for fp, errmsg in error_list:
                    logf.write("%s : %s\n" % (fp.encode('utf-8'), errmsg))
                logf.write("\n")
            if replaced_list:
                logf.write("=== Replaced Header ===\n")
                for fp in replaced_list:
                    logf.write("%s\n" % fp.encode('utf-8'))
                logf.write("\n")
            if added_list:
                logf.write("=== Added New Header ===\n")
                for fp in added_list:
                    logf.write("%s\n" % fp.encode('utf-8'))
                logf.write("\n")
            logf.write(overall_summary_text + "\n")
    except Exception as e:
        sys.stderr.write("Error: could not write log file '%s': %s\n" % (log_path, str(e)))

    # Print overall summary to console
    print(overall_summary_text)

    # Write per-directory summary to separate file
    summary_log = os.path.join(os.getcwd(), "license_replace_summary.log")
    try:
        with open(summary_log, 'w') as fsum:
            fsum.write(dir_summary_text + "\n")
    except Exception as e:
        sys.stderr.write("Error: could not write per-directory summary file '%s': %s\n" % (summary_log, str(e)))

if __name__ == "__main__":
    main()
