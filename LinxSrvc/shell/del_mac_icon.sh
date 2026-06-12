#!/bin/bash
# Delete .app files that have a corresponding "— Windows 10+ x64.app" sibling
python3 -c "
import os, shutil, argparse, re, logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s [%(levelname)s] %(message)s')
log = logging.getLogger(__name__)

SUFFIX = ' — Windows 10+ x64'
EXT = '.app'

class WinAppCleaner:
    def __init__(self, confirm=True, dry_run=False):
        self.confirm = confirm
        self.dry_run = dry_run
        self.deleted = 0

    def run(self, directory):
        log.info(f'Scanning: {directory}')
        try:
            entries = [e.name for e in os.scandir(directory) if e.name.endswith(EXT)]
        except FileNotFoundError:
            log.error(f'Not found: {directory}')
            return

        for name in entries:
            if SUFFIX not in name:
                continue
            base = name[:name.index(SUFFIX)]
            # Candidates matching base name: base.app, base N.app, base N SUFFIX.app
            for other in entries:
                if other == name:
                    continue
                if other == f'{base}{EXT}' or re.match(rf'^{re.escape(base)} \d+{re.escape(EXT)}$', other) or \
                   re.match(rf'^{re.escape(base)} \d+{re.escape(SUFFIX)}{re.escape(EXT)}$', other):
                    self._delete(directory, other)

        log.info(f'Deleted: {self.deleted}')

    def _delete(self, d, fname):
        path = os.path.join(d, fname)
        log.info(f'Found: {path}')
        if self.confirm and input(f'Delete \"{path}\"? (y/N): ').strip().lower() != 'y':
            log.info('Skipped')
            return
        if not self.dry_run:
            try:
                (shutil.rmtree if os.path.isdir(path) else os.remove)(path)
                log.info(f'Deleted: {path}')
                self.deleted += 1
            except Exception as e:
                log.error(f'Failed: {path}: {e}')

if __name__ == '__main__':
    p = argparse.ArgumentParser(description='Clean non-Win x64 .app duplicates')
    p.add_argument('directory', nargs='?', default='/Applications', help='Target directory')
    p.add_argument('--yes', '-y', action='store_false', dest='confirm', help='Skip confirmation')
    p.add_argument('--dry-run', '-n', action='store_true', help='Simulate only')
    args = p.parse_args()
    try:
        WinAppCleaner(confirm=args.confirm, dry_run=args.dry_run).run(os.path.expanduser(args.directory))
    except KeyboardInterrupt:
        log.info('Cancelled')
"
