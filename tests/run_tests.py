#!/usr/bin/env python3
"""
Test runner script for C++ tests.
Each test consists of a .cpp file and a .txt file with the same name.
The script compiles and runs each test, comparing output to the expected output.
"""

import os
import sys
import subprocess
import glob
import argparse
from pathlib import Path


class Colors:
    """ANSI color codes for terminal output."""
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    RESET = '\033[0m'
    BOLD = '\033[1m'


def find_tests(test_dir):
    """
    Find all test pairs (cpp + txt files with same name).
    Returns a dict: {test_name: (cpp_path, txt_path)}
    """
    tests = {}
    cpp_files = set()
    txt_files = set()
    
    # Find all cpp and txt files
    for cpp in glob.glob(os.path.join(test_dir, '*.cpp')):
        basename = os.path.basename(cpp)
        if basename.startswith('test_'):
            cpp_files.add(basename)
    
    for txt in glob.glob(os.path.join(test_dir, '*.txt')):
        basename = os.path.basename(txt)
        if basename.startswith('test_'):
            txt_files.add(basename)
    
    # Match pairs
    for cpp_file in cpp_files:
        test_name = cpp_file.replace('.cpp', '')
        txt_file = test_name + '.txt'
        
        if txt_file in txt_files:
            cpp_path = os.path.join(test_dir, cpp_file)
            txt_path = os.path.join(test_dir, txt_file)
            tests[test_name] = (cpp_path, txt_path)
    
    return tests


def compile_test(test_name, cpp_path, test_dir, parent_dir):
    """
    Compile a test. Returns (success: bool, error_msg: str)
    """

    # Flatten and expand glob patterns
    cmd = ['g++', '-Wall', '--std=c++17', f'-I{parent_dir}', '-o',
           os.path.join(test_dir, f'{test_name}.out')]
    
    # Add all cpp files from parent directory
    cpp_in_parent = glob.glob(os.path.join(parent_dir, '*.cpp'))
    cmd.extend(cpp_in_parent)
    
    # Add the test file itself
    cmd.append(cpp_path)
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
        if result.returncode != 0:
            return False, result.stderr
        return True, ""
    except subprocess.TimeoutExpired:
        return False, "Compilation timed out"
    except Exception as e:
        return False, str(e)


def run_test(test_name, test_dir):
    """
    Run a compiled test. Returns (success: bool, output: str, error_msg: str)
    """
    exe_path = os.path.join(test_dir, f'{test_name}.out')
    
    try:
        result = subprocess.run([exe_path], capture_output=True, text=True, timeout=10)
        if result.returncode != 0:
            return False, result.stdout, f"Exit code: {result.returncode}\nStderr: {result.stderr}"
        return True, result.stdout, ""
    except subprocess.TimeoutExpired:
        return False, "", "Test execution timed out"
    except Exception as e:
        return False, "", str(e)


def compare_output(actual, expected):
    """
    Compare actual output to expected output.
    Returns (match: bool, diff_msg: str)
    """
    actual_lines = actual.strip().split('\n') if actual.strip() else []
    expected_lines = expected.strip().split('\n') if expected.strip() else []
    
    if actual.strip() == expected.strip():
        return True, ""
    
    # Build diff message
    diff_msg = []
    max_lines = max(len(actual_lines), len(expected_lines))
    
    for i in range(max_lines):
        actual_line = actual_lines[i] if i < len(actual_lines) else "[missing]"
        expected_line = expected_lines[i] if i < len(expected_lines) else "[missing]"
        
        if actual_line != expected_line:
            diff_msg.append(f"  Line {i+1}:")
            diff_msg.append(f"    Expected: {expected_line}")
            diff_msg.append(f"    Actual:   {actual_line}")
    
    return False, "\n".join(diff_msg)


def print_header():
    """Print script header."""
    print(f"\n{Colors.BOLD}{Colors.BLUE}{'='*70}")
    print(f"  C++ Test Runner")
    print(f"{'='*70}{Colors.RESET}\n")


def print_test_header(test_name):
    """Print test header."""
    print(f"{Colors.BOLD}Test: {test_name}{Colors.RESET}")


def print_success(msg):
    """Print success message."""
    print(f"{Colors.GREEN}✓ {msg}{Colors.RESET}")


def print_failure(msg):
    """Print failure message."""
    print(f"{Colors.RED}✗ {msg}{Colors.RESET}")


def print_info(msg):
    """Print info message."""
    print(f"{Colors.YELLOW}→ {msg}{Colors.RESET}")


def run_tests(test_names=None):
    """
    Run tests. If test_names is None, run all tests.
    """
    test_dir = os.path.dirname(os.path.abspath(__file__))
    parent_dir = os.path.dirname(test_dir)
    
    # Find all available tests
    all_tests = find_tests(test_dir)
    
    if not all_tests:
        print(f"{Colors.RED}No tests found!{Colors.RESET}")
        return False
    
    # Filter tests if specific names provided
    if test_names:
        tests_to_run = {}
        for name in test_names:
            # Try exact match or prefix match
            if name in all_tests:
                tests_to_run[name] = all_tests[name]
            else:
                # Try as prefix
                matches = [k for k in all_tests.keys() if k.startswith(name)]
                if matches:
                    for match in matches:
                        tests_to_run[match] = all_tests[match]
                else:
                    print(f"{Colors.RED}Test not found: {name}{Colors.RESET}")
    else:
        tests_to_run = all_tests
    
    # Sort tests by name
    tests_to_run = dict(sorted(tests_to_run.items()))
    
    print_header()
    print(f"Running {len(tests_to_run)} test(s)...\n")
    
    results = {}
    
    for test_name in tests_to_run:
        cpp_path, txt_path = tests_to_run[test_name]
        print_test_header(test_name)
        
        # Compile
        print_info("Compiling...")
        success, error_msg = compile_test(test_name, cpp_path, test_dir, parent_dir)
        if not success:
            print_failure(f"Compilation failed")
            print(f"  {Colors.RED}{error_msg}{Colors.RESET}")
            results[test_name] = False
            print()
            continue
        print_success("Compilation successful")
        
        # Run
        print_info("Running...")
        success, output, error_msg = run_test(test_name, test_dir)
        if not success:
            print_failure(f"Execution failed")
            if error_msg:
                print(f"  {Colors.RED}{error_msg}{Colors.RESET}")
            results[test_name] = False
            print()
            continue
        print_success("Execution successful")
        
        # Compare output
        print_info("Comparing output...")
        with open(txt_path, 'r') as f:
            expected = f.read()
        
        match, diff_msg = compare_output(output, expected)
        if match:
            print_success("Output matches expected")
            results[test_name] = True
        else:
            print_failure("Output does not match")
            print(f"{Colors.RED}{diff_msg}{Colors.RESET}")
            results[test_name] = False
        
        print()
    
    # Print summary
    passed = sum(1 for v in results.values() if v)
    failed = len(results) - passed
    
    print(f"{Colors.BOLD}{Colors.BLUE}{'='*70}")
    print(f"  Summary")
    print(f"{'='*70}{Colors.RESET}")
    print(f"Passed: {Colors.GREEN}{passed}{Colors.RESET}")
    print(f"Failed: {Colors.RED}{failed}{Colors.RESET}")
    print(f"Total:  {len(results)}")
    print()
    
    return failed == 0


def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description='Run C++ tests. Each test is a .cpp file with a corresponding .txt file with expected output.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""Examples:
  python3 run_tests.py              # Run all tests
  python3 run_tests.py test_1       # Run test_1_kick_tires
  python3 run_tests.py test_1 test_2  # Run specific tests
        """
    )
    parser.add_argument('tests', nargs='*', help='Test name(s) to run (optional, runs all if not specified)')
    
    args = parser.parse_args()
    
    success = run_tests(args.tests if args.tests else None)
    sys.exit(0 if success else 1)


if __name__ == '__main__':
    main()
