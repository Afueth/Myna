// Runs inside the Myna view. Sends clicks on results and buttons to the extension.
(function () {
    const vscode = acquireVsCodeApi();

    document.addEventListener('click', (event) => {
        const target = event.target.closest('[data-action]');
        if (!target) {
            return;
        }
        const action = target.getAttribute('data-action');
        if (action === 'open') {
            vscode.postMessage({ type: 'open', index: Number(target.getAttribute('data-index')) });
        } else if (action === 'copy') {
            vscode.postMessage({ type: 'copy' });
        }
    });
})();
