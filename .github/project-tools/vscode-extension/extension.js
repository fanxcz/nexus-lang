const vscode = require('vscode');
const { LanguageClient } = require('vscode-languageclient/node');

let client;
let outputChannel;

function activate(context) {
    const serverPath = vscode.workspace.getConfiguration('nexus').get('serverPath', 'nexus');
    outputChannel = vscode.window.createOutputChannel('NEXUS Language Server');

    const serverOptions = {
        command: serverPath,
        args: ['lsp'],
        options: {
            env: process.env,
            // The portable Windows entry point is a .cmd launcher.
            shell: process.platform === 'win32'
        }
    };

    const clientOptions = {
        documentSelector: [{ scheme: 'file', language: 'nexus' }],
        synchronize: {
            fileEvents: vscode.workspace.createFileSystemWatcher('**/*.nx')
        },
        outputChannel,
        traceOutputChannel: outputChannel
    };

    client = new LanguageClient(
        'nexusLanguageServer',
        'NEXUS Language Server',
        serverOptions,
        clientOptions
    );
    context.subscriptions.push(outputChannel, client);
    return client.start();
}

async function deactivate() {
    if (client) await client.stop();
}

module.exports = { activate, deactivate };
