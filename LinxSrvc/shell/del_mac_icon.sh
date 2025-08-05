#!/bin/bash
# Deletes .app files/folders that do not contain the suffix ' — Windows 10+ x64'
python -c "
import os
import shutil
import argparse
import re
import logging

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [%(levelname)s] %(message)s',
    handlers=[logging.StreamHandler()]
)
logger = logging.getLogger(__name__)

TARGET_SUFFIX = ' — Windows 10+ x64'
APP_EXT = '.app'

class WinAppCleaner:
    def __init__(self, confirm: bool = True, dry_run: bool = False):
        self.confirm = confirm
        self.dry_run = dry_run
        self.total_deleted = 0

    def find_and_delete(self, directory: str):
        logger.info(f'Scanning directory: {directory}')
        try:
            app_entries = [e for e in os.scandir(directory) if e.name.endswith(APP_EXT)]
        except FileNotFoundError:
            logger.error(f'Directory not found: {directory}')
            return

        app_names = [e.name for e in app_entries]
        # Filter out entries that do not contain the target suffix
        base_pattern = re.compile(
            rf'^(.*?)(?: \d+)?{re.escape(TARGET_SUFFIX)}{re.escape(APP_EXT)}$'
        )
        for name in app_names:
            if TARGET_SUFFIX in name:
                base_match = base_pattern.match(name)
                if not base_match:
                    continue
                base_name = base_match.group(1)
                # Check for other .app files that match the base name
                patterns = [
                    f'{base_name}{APP_EXT}',  # base_name.app
                    re.compile(rf'^{re.escape(base_name)} \d+{re.escape(APP_EXT)}$'),  # base_name 1.app
                    re.compile(rf'^{re.escape(base_name)} \d+{re.escape(TARGET_SUFFIX)}{re.escape(APP_EXT)}$')  # base_name 1 — Windows 10+ x64.app
                ]
                for other in app_names:
                    if other == name:
                        continue
                    if other == patterns[0] or patterns[1].match(other) or patterns[2].match(other):
                        self._delete(directory, other)

    def _delete(self, directory, filename):
        full_path = os.path.join(directory, filename)
        logger.info(f'Found to delete: {full_path}')
        if self.confirm:
            ans = input(f'Delete \'{full_path}\'? (y/N): ').strip().lower()
            if ans != 'y':
                logger.info('Skipped by user.')
                return
        if not self.dry_run:
            try:
                if os.path.isdir(full_path):
                    shutil.rmtree(full_path)
                else:
                    os.remove(full_path)
                logger.info(f'Deleted: {full_path}')
                self.total_deleted += 1
            except Exception as e:
                logger.error(f'Failed to delete {full_path}: {e}')
        else:
            logger.info(f'[Dry-run] Would delete: {full_path}')

    def run(self, directory: str):
        self.find_and_delete(directory)
        logger.info(f'Total deleted: {self.total_deleted}')

def main():
    parser = argparse.ArgumentParser(
        description='Delete .app files/folders not containing \' — Windows 10+ x64\' if a corresponding one exists'
    )
    parser.add_argument(
        'directory',
        nargs='?',
        default='/Applications',
        help='Target directory to scan'
    )
    parser.add_argument(
        '--auto',
        action='store_false',
        dest='confirm',
        help='Disable confirmation prompts'
    )
    parser.add_argument(
        '--dry-run',
        action='store_true',
        help='Simulate operations without actual deletion'
    )
    args = parser.parse_args()

    cleaner = WinAppCleaner(
        confirm=args.confirm,
        dry_run=args.dry_run
    )
    try:
        cleaner.run(os.path.expanduser(args.directory))
    except KeyboardInterrupt:
        logger.info('\nOperation cancelled by user')

if __name__ == '__main__':
    main()
" <<< "$0"
