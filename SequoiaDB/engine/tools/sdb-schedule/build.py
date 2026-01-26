from __future__ import print_function
import os
import sys
import subprocess

ROOT_DIR = os.path.abspath(os.path.dirname(__file__))
POM = os.path.join(ROOT_DIR, "src", "pom.xml")


def run_cmd(cmd, cwd=None):
    print("Running:", cmd)
    try:
        if sys.version_info[0] < 3:
            ret = subprocess.call(cmd, shell=True, cwd=cwd)
            if ret != 0:
                raise subprocess.CalledProcessError(ret, cmd)
        else:
            subprocess.run(cmd, shell=True, cwd=cwd, check=True)
    except subprocess.CalledProcessError as e:
        print("Command failed with exit code", e.returncode)
        sys.exit(e.returncode)


def build_project(version=None):
    print("Building the project...")

    cmd = "mvn clean install -f " + POM
    if version:
        cmd += " -Drevision=" + version

    run_cmd(cmd, ROOT_DIR)
    print("Build project successful!")


def main():
    version = None
    if len(sys.argv) > 1:
        version = sys.argv[1].strip()

    if version:
        print("Build with version:", version)
    else:
        print("Build without version parameter")

    build_project(version)
    print("\nBuild finished successfully!")


if __name__ == "__main__":
    main()
