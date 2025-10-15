#!/usr/bin/env python3
"""
PlatformIO Library Structure Validation Script
Validates that the painlessMesh library has proper structure for PlatformIO
"""

import os
import sys
import json
from pathlib import Path
from typing import List, Tuple, Dict

class Colors:
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    RESET = '\033[0m'

def print_check(message: str, passed: bool, details: str = ""):
    """Print a check result with color"""
    symbol = f"{Colors.GREEN}✓{Colors.RESET}" if passed else f"{Colors.RED}✗{Colors.RESET}"
    print(f"{symbol} {message}")
    if details:
        print(f"  {details}")

def check_library_json() -> Tuple[bool, str]:
    """Check if library.json exists and has required fields"""
    library_json_path = Path("library.json")
    
    if not library_json_path.exists():
        return False, "library.json not found in root directory"
    
    try:
        with open(library_json_path, 'r', encoding='utf-8') as f:
            config = json.load(f)
        
        required_fields = {
            'name': 'Library name',
            'version': 'Version number',
            'frameworks': 'Target frameworks',
            'platforms': 'Target platforms'
        }
        
        missing = []
        for field, description in required_fields.items():
            if field not in config:
                missing.append(f"{description} ({field})")
        
        if missing:
            return False, f"Missing fields: {', '.join(missing)}"
        
        # Check for srcDir
        has_srcdir = 'srcDir' in config
        has_includedir = 'includeDir' in config
        
        details = []
        if has_srcdir:
            details.append(f"srcDir: {config['srcDir']}")
        else:
            details.append(f"{Colors.YELLOW}Warning: srcDir not specified{Colors.RESET}")
        
        if has_includedir:
            details.append(f"includeDir: {config['includeDir']}")
        
        return True, " | ".join(details)
        
    except json.JSONDecodeError as e:
        return False, f"Invalid JSON: {e}"
    except Exception as e:
        return False, f"Error reading file: {e}"

def check_library_properties() -> Tuple[bool, str]:
    """Check if library.properties exists for Arduino compatibility"""
    properties_path = Path("library.properties")
    
    if not properties_path.exists():
        return False, "library.properties not found (needed for Arduino IDE)"
    
    try:
        with open(properties_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        required = ['name=', 'version=', 'author=', 'maintainer=', 'sentence=']
        missing = [r.rstrip('=') for r in required if r not in content]
        
        if missing:
            return False, f"Missing properties: {', '.join(missing)}"
        
        # Extract includes= line
        includes_line = [line for line in content.split('\n') if line.startswith('includes=')]
        if includes_line:
            header = includes_line[0].split('=')[1]
            return True, f"Primary header: {header}"
        else:
            return True, "Valid (no includes= specified)"
            
    except Exception as e:
        return False, f"Error reading file: {e}"

def check_src_directory() -> Tuple[bool, str]:
    """Check if src directory exists and contains source files"""
    src_path = Path("src")
    
    if not src_path.exists():
        return False, "src directory not found"
    
    if not src_path.is_dir():
        return False, "src exists but is not a directory"
    
    # Count source files
    cpp_files = list(src_path.glob("*.cpp"))
    h_files = list(src_path.glob("*.h"))
    hpp_files = list(src_path.glob("*.hpp"))
    
    total = len(cpp_files) + len(h_files) + len(hpp_files)
    
    if total == 0:
        return False, "src directory is empty"
    
    details = f"{len(cpp_files)} .cpp, {len(h_files)} .h, {len(hpp_files)} .hpp files"
    return True, details

def check_no_root_cpp() -> Tuple[bool, str]:
    """Check that no .cpp files are in root directory"""
    root_cpp = list(Path(".").glob("*.cpp"))
    
    if root_cpp:
        files = ", ".join([f.name for f in root_cpp])
        return False, f"Found .cpp files in root: {files}"
    
    return True, "No .cpp files in root (correct)"

def check_duplicate_library_json() -> Tuple[bool, str]:
    """Check for duplicate library.json files"""
    all_library_json = list(Path(".").rglob("library.json"))
    
    # Filter out test directories
    relevant = [p for p in all_library_json 
                if 'test' not in str(p).lower() 
                and '.pio' not in str(p).lower()
                and 'node_modules' not in str(p).lower()]
    
    if len(relevant) > 1:
        paths = "\n    ".join([str(p) for p in relevant])
        return False, f"Multiple library.json found:\n    {paths}"
    
    if len(relevant) == 0:
        return False, "No library.json found"
    
    return True, f"Single library.json at: {relevant[0]}"

def check_header_files() -> Tuple[bool, str]:
    """Check for main header files"""
    src_path = Path("src")
    
    if not src_path.exists():
        return False, "src directory not found"
    
    headers = {
        'painlessMesh.h': src_path / 'painlessMesh.h',
        'AlteriomPainlessMesh.h': src_path / 'AlteriomPainlessMesh.h'
    }
    
    found = []
    missing = []
    
    for name, path in headers.items():
        if path.exists():
            found.append(name)
        else:
            missing.append(name)
    
    if not found:
        return False, "No main header files found"
    
    details = f"Found: {', '.join(found)}"
    if missing:
        details += f" | Missing: {', '.join(missing)}"
    
    return True, details

def check_examples_directory() -> Tuple[bool, str]:
    """Check if examples directory exists"""
    examples_path = Path("examples")
    
    if not examples_path.exists():
        return False, "examples directory not found"
    
    # Count .ino files
    ino_files = list(examples_path.rglob("*.ino"))
    
    if not ino_files:
        return False, "No .ino files found in examples"
    
    return True, f"Found {len(ino_files)} example sketches"

def check_platformio_structure() -> Tuple[bool, str]:
    """Check overall PlatformIO structure compliance"""
    checks = [
        ("Root has library.json", Path("library.json").exists()),
        ("Root has library.properties", Path("library.properties").exists()),
        ("Has src directory", Path("src").exists()),
        ("Has examples directory", Path("examples").exists()),
        ("No .cpp in root", len(list(Path(".").glob("*.cpp"))) == 0),
    ]
    
    passed = sum(1 for _, check in checks if check)
    total = len(checks)
    
    if passed == total:
        return True, f"All {total} structure checks passed"
    else:
        failed = [name for name, check in checks if not check]
        return False, f"{passed}/{total} passed. Failed: {', '.join(failed)}"

def main():
    """Run all validation checks"""
    print(f"\n{Colors.BLUE}{'='*60}{Colors.RESET}")
    print(f"{Colors.BLUE}PlatformIO Library Structure Validation{Colors.RESET}")
    print(f"{Colors.BLUE}{'='*60}{Colors.RESET}\n")
    
    # Check we're in the right directory
    if not Path("library.json").exists() and not Path("src").exists():
        print(f"{Colors.RED}Error: Not in a library root directory{Colors.RESET}")
        print("Please run this script from the painlessMesh repository root")
        sys.exit(1)
    
    checks = [
        ("Library JSON Exists", check_library_json),
        ("Library Properties Valid", check_library_properties),
        ("Source Directory Structure", check_src_directory),
        ("No Root Source Files", check_no_root_cpp),
        ("No Duplicate Metadata", check_duplicate_library_json),
        ("Header Files Present", check_header_files),
        ("Examples Directory", check_examples_directory),
        ("PlatformIO Compliance", check_platformio_structure),
    ]
    
    results = []
    
    for name, check_func in checks:
        try:
            passed, details = check_func()
            results.append(passed)
            print_check(name, passed, details)
        except Exception as e:
            results.append(False)
            print_check(name, False, f"Exception: {e}")
    
    # Summary
    passed_count = sum(results)
    total_count = len(results)
    percentage = (passed_count / total_count * 100) if total_count > 0 else 0
    
    print(f"\n{Colors.BLUE}{'='*60}{Colors.RESET}")
    print(f"Results: {passed_count}/{total_count} checks passed ({percentage:.1f}%)")
    
    if passed_count == total_count:
        print(f"{Colors.GREEN}✓ Library structure is valid for PlatformIO!{Colors.RESET}")
        print(f"\n{Colors.GREEN}Ready to use in platformio.ini:{Colors.RESET}")
        print(f"  lib_deps = ")
        print(f"      https://github.com/Alteriom/painlessMesh#copilot/start-phase-2-implementation")
        return 0
    else:
        print(f"{Colors.RED}✗ Library structure has issues{Colors.RESET}")
        print(f"{Colors.YELLOW}Please fix the issues above before using with PlatformIO{Colors.RESET}")
        return 1

if __name__ == "__main__":
    sys.exit(main())
