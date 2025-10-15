#!/usr/bin/env python3
"""
Validation script for Alteriom CI/CD implementation

This script validates the CI/CD setup without performing actual builds.
It checks for:
- PlatformIO configuration validity
- Environment definitions
- Required files and directories
- Workflow syntax
"""

import os
import sys
import json
import subprocess
import yaml
import urllib.request
import urllib.error
from pathlib import Path

def check_platformio_config():
    """Check if platformio.ini is valid and get environment list"""
    try:
        result = subprocess.run(
            ["pio", "project", "config", "--json-output"],
            capture_output=True,
            text=True,
            timeout=30
        )
        
        if result.returncode != 0:
            print("❌ PlatformIO config validation failed")
            return False, []
        
        # Parse configuration
        config_data = json.loads(result.stdout)
        environments = []
        
        for section in config_data:
            section_name = section[0]
            if section_name.startswith("env:") and not section_name.startswith("env:base_"):
                env_name = section_name[4:]  # Remove "env:" prefix
                environments.append(env_name)
        
        environments.sort()
        print(f"✅ PlatformIO config valid - found {len(environments)} buildable environments")
        return True, environments
        
    except subprocess.TimeoutExpired:
        print("❌ PlatformIO config check timed out")
        return False, []
    except subprocess.CalledProcessError as e:
        print(f"❌ PlatformIO config check failed: {e}")
        return False, []
    except json.JSONDecodeError as e:
        print(f"❌ Failed to parse PlatformIO config: {e}")
        return False, []
    except Exception as e:
        print(f"❌ Unexpected error checking PlatformIO config: {e}")
        return False, []

def check_workflow_files():
    """Check if workflow files exist and are valid YAML"""
    workflows_dir = Path(".github/workflows")
    if not workflows_dir.exists():
        print("❌ .github/workflows directory does not exist")
        return False
    
    required_workflows = ["ci.yml", "release.yml"]
    all_valid = True
    
    for workflow_file in required_workflows:
        workflow_path = workflows_dir / workflow_file
        if not workflow_path.exists():
            print(f"❌ Workflow file missing: {workflow_file}")
            all_valid = False
            continue
        
        try:
            # Force UTF-8 to handle emojis and non-ASCII characters in workflow files on Windows
            with open(workflow_path, 'r', encoding='utf-8', errors='strict') as f:
                yaml.safe_load(f)
            print(f"✅ Workflow file valid: {workflow_file}")
        except yaml.YAMLError as e:
            print(f"❌ Invalid YAML in {workflow_file}: {e}")
            all_valid = False
        except Exception as e:
            # Retry with permissive error handling to provide a more graceful diagnostic
            try:
                with open(workflow_path, 'r', encoding='utf-8', errors='ignore') as f:
                    yaml.safe_load(f)
                print(f"✅ Workflow file valid (after tolerant read): {workflow_file}")
            except Exception as e2:
                print(f"❌ Error reading {workflow_file}: {e2}")
                all_valid = False
            all_valid = False
    
    return all_valid

def check_build_script():
    """Check if build script exists and is executable"""
    script_path = Path("scripts/build_all_environments.py")
    
    if not script_path.exists():
        print("❌ Build script does not exist")
        return False
    
    if not os.access(script_path, os.X_OK):
        print("⚠️ Build script is not executable")
        try:
            os.chmod(script_path, 0o755)
            print("✅ Made build script executable")
        except Exception as e:
            print(f"❌ Failed to make build script executable: {e}")
            return False
    else:
        print("✅ Build script exists and is executable")
    
    # Test script help
    try:
        result = subprocess.run(
            ["python", str(script_path), "--help"],
            capture_output=True,
            text=True,
            timeout=10
        )
        if result.returncode == 0:
            print("✅ Build script help works")
            return True
        else:
            print("❌ Build script help failed")
            return False
    except Exception as e:
        print(f"❌ Error testing build script: {e}")
        return False

def check_gitignore():
    """Check if .gitignore includes build artifacts"""
    gitignore_path = Path(".gitignore")
    
    if not gitignore_path.exists():
        print("⚠️ .gitignore does not exist")
        return False
    
    try:
        with open(gitignore_path, 'r') as f:
            gitignore_content = f.read()
        
        required_patterns = [
            "build-output/",
            "build-results/",
            "release-firmware/",
            "release-artifacts/",
            "*.bin",
            "*.elf"
        ]
        
        missing_patterns = []
        for pattern in required_patterns:
            if pattern not in gitignore_content:
                missing_patterns.append(pattern)
        
        if missing_patterns:
            print(f"⚠️ .gitignore missing patterns: {', '.join(missing_patterns)}")
            return False
        else:
            print("✅ .gitignore includes build artifact patterns")
            return True
            
    except Exception as e:
        print(f"❌ Error checking .gitignore: {e}")
        return False

def check_secret_template():
    """Check if .secret.template exists"""
    secret_template = Path(".secret.template")
    if secret_template.exists():
        print("✅ .secret.template exists")
        return True
    else:
        print("⚠️ .secret.template does not exist")
        return False

def check_environment_matrix():
    """Validate that all environments are covered in CI matrix"""
    # This would require parsing the workflow files to extract the matrix
    # For now, just check that we have a reasonable number of environments
    _, environments = check_platformio_config()
    
    if len(environments) >= 15:  # We expect around 19 environments
        print(f"✅ Good number of environments found: {len(environments)}")
        return True
    else:
        print(f"⚠️ Fewer environments than expected: {len(environments)}")
        return False

def check_network_connectivity():
    """Check network connectivity to PlatformIO servers"""
    test_urls = [
        "https://packages.platformio.org/",
        "https://registry.platformio.org/",
        "https://github.com/",
    ]
    
    connectivity_results = []
    for url in test_urls:
        try:
            response = urllib.request.urlopen(url, timeout=10)
            if response.status == 200:
                connectivity_results.append(True)
            else:
                connectivity_results.append(False)
        except (urllib.error.URLError, urllib.error.HTTPError, Exception):
            connectivity_results.append(False)
    
    successful_connections = sum(connectivity_results)
    total_tests = len(test_urls)
    
    if successful_connections == total_tests:
        print("✅ Full network connectivity to PlatformIO servers")
        return True
    elif successful_connections > 0:
        print(f"⚠️ Partial network connectivity ({successful_connections}/{total_tests} servers reachable)")
        return False
    else:
        print("❌ No network connectivity to PlatformIO servers")
        print("   This will cause platform installation failures")
        return False

def validate_setup():
    """Run all validation checks"""
    print("🔍 Validating Alteriom CI/CD Setup\n")
    
    checks = [
        ("PlatformIO Configuration", check_platformio_config),
        ("Network Connectivity", check_network_connectivity),
        ("GitHub Workflows", check_workflow_files),
        ("Build Script", check_build_script),
        ("GitIgnore Configuration", check_gitignore),
        ("Secret Template", check_secret_template),
        ("Environment Coverage", check_environment_matrix),
    ]
    
    results = []
    for check_name, check_func in checks:
        print(f"\n📋 Checking: {check_name}")
        if check_name == "PlatformIO Configuration":
            result, environments = check_func()
            results.append(result)
            if result:
                print(f"   Environments: {', '.join(environments[:5])}{'...' if len(environments) > 5 else ''}")
        else:
            result = check_func()
            results.append(result)
    
    # Summary
    passed = sum(results)
    total = len(results)
    
    print(f"\n📊 Validation Summary:")
    print(f"   Passed: {passed}/{total} checks")
    print(f"   Success Rate: {(passed/total*100):.1f}%")
    
    if passed == total:
        print("\n🎉 All validation checks passed!")
        print("   CI/CD setup appears to be correctly configured.")
        print("   You can now test the workflows in GitHub Actions.")
        return True
    else:
        print(f"\n⚠️ {total - passed} validation checks failed or showed warnings.")
        print("   Please address the issues above before using the CI/CD system.")
        return False

if __name__ == "__main__":
    # Ensure we're in the project directory
    if not Path("platformio.ini").exists():
        print("❌ Not in project root directory (platformio.ini not found)")
        sys.exit(1)
    
    success = validate_setup()
    sys.exit(0 if success else 1)