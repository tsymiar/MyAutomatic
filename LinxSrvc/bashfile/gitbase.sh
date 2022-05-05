#!/bin/bash
git remote add project-history https://github.com/tsymiar/MyAutomatic.git
git log --oneline --decorate
INSTRUCT=$(echo "get history by echo 'get history from blah blah blah' | git commit-tree ${RETAIN_COMMIT}^{tree}" | git commit-tree ${RETAIN_COMMIT}^{tree})
git rebase -f --onto ${INSTRUCT} ${RETAIN_COMMIT}
git replace ${ABOVE_COMMIT} ${BELOW_COMMIT}
git remote set-url origin  https://[token]@github.com/tsymiar/MyAutomatic.git/
