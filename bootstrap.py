#!/usr/bin/env python

import argparse

import sys
import os
import errno
import platform
from bootstraper import MaxMEBootstraper

def isWin():
  return 'Windows' in platform.system()


def isLinux():
  return 'Linux' in platform.system()


def isDarwin():
  return 'Darwin' in platform.system()


def main():
    parser = argparse.ArgumentParser(description='Genarate project files of Virual Device based platform etc.')

    if (isWin()):
        parser.add_argument('target', choices=['win',  'win64']
        , help='Special the target that build for.')

    parser.add_argument('--folder', default='build', help='The build folder, relative path of project directory.Default is"./build".')
    parser.add_argument('--build', action='store_true', help='Build after project files genarated.')
    parser.add_argument('--debug', action='store_true', help='Enable debug build.')

    args = parser.parse_args()

    if "--help" in sys.argv or "-h" in sys.argv:
        parser.print_help()
        sys.exit()
    try:
        bootstraper = MaxMEBootstraper(
        target= args.target,
        build_folder= args.folder,
        build= args.build,
        debug= args.debug
        )
        
        bootstraper.run()
    except (OSError) as e:
        print(e)
        sys.exit(e)

if __name__ == "__main__":
    sys.exit(main())

