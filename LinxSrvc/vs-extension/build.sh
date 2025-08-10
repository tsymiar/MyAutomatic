#!/bin/bash
if [ "${1}" == "clean" ]; then
    rm -rvf out node_modules *.vsix
    exit 0
fi
if command -v vsce >/dev/null 2>&1; then
    echo "vsce is available to build"
else
    curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.39.7/install.sh | bash
    if [ "${PIPESTATUS[0]}" -ne 0 ]; then bash install.sh; fi;
    source ~/.bashrc
    nvm install --lts
    nvm use --lts
    npm cache clean --force
    npm uninstall -g yo generator-code
    npm install -g yo generator-code
    ls $(node -e "console.log(require.resolve('stream'))")
    npm install -g vsce
fi
npm install --save vscode typescript @types/node
npm install --save-dev @types/vscode vsce
npm install @types/vscode@1.75.0
npm audit fix --force
vsce package
