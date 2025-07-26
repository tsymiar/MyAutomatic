curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.39.7/install.sh | bash
source ~/.bashrc
nvm install --lts
nvm use --lts
npm cache clean --force
npm uninstall -g yo generator-code
npm install -g yo generator-code
ls $(node -e "console.log(require.resolve('stream'))")
npm install --save vscode typescript @types/node
npm install --save-dev @types/vscode vsce
npm install -g vsce
vsce package
