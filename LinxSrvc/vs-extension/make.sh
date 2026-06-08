#!/bin/bash
if [ "${1}" == "clean" ]; then
    rm -rvf out ./*.vsix node_modules scripts/__pycache__
    exit 0
fi
if command -v npm >/dev/null 2>&1; then
    echo "npm is available to build"
else
    echo "installing Node.js and npm."
    sudo apt-get update
    sudo apt-get install -y nodejs npm
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
npm install --save vscode typescript
npm install --save-dev @types/node @types/vscode @vscode/vsce
npm install @types/vscode@1.75.0
npm audit fix --force
vsce package
