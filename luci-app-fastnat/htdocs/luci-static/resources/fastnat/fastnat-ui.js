/* Modal helpers extracted verbatim (behaviour-wise) from
 * luci-app-fwx-resources/common.js, trimmed to the two functions the
 * fastnat page uses, so jQuery is no longer required. */

function showCommonModal(message, duration, type) {
    duration = duration == null ? 2000 : duration;
    var existing = document.querySelector('.common-modal-mask');
    if (existing) existing.remove();

    var mask = document.createElement('div');
    mask.className = 'common-modal-mask';

    var content = document.createElement('div');
    content.className = 'common-modal-content';
    content.style.backgroundColor = 'rgba(0, 0, 0, 0.5)';
    content.style.color = 'white';
    content.textContent = message;

    mask.appendChild(content);
    document.body.appendChild(mask);

    setTimeout(function() { mask.classList.add('show'); }, 10);
    setTimeout(function() {
        mask.classList.remove('show');
        setTimeout(function() {
            if (mask.parentNode) mask.remove();
        }, 300);
    }, duration);
}

function showConfirmModal(message, onConfirm, onCancel, confirmText, cancelText) {
    confirmText = confirmText || '确定';
    cancelText = cancelText || '取消';

    var existing = document.querySelector('.common-modal-mask');
    if (existing) existing.remove();

    var mask = document.createElement('div');
    mask.className = 'common-modal-mask';

    var content = document.createElement('div');
    content.className = 'common-modal-content';
    content.style.width = '300px';
    content.style.height = 'auto';
    content.style.minHeight = '120px';
    content.style.padding = '20px';
    content.style.flexDirection = 'column';
    content.style.justifyContent = 'space-between';
    content.style.backgroundColor = 'rgba(0, 0, 0, 0.5)';
    content.style.color = 'white';

    var messageDiv = document.createElement('div');
    messageDiv.textContent = message;
    messageDiv.style.marginBottom = '20px';
    messageDiv.style.textAlign = 'center';

    var buttonContainer = document.createElement('div');
    buttonContainer.style.display = 'flex';
    buttonContainer.style.justifyContent = 'center';
    buttonContainer.style.gap = '10px';

    var confirmBtn = document.createElement('button');
    confirmBtn.textContent = confirmText;
    confirmBtn.style.padding = '8px 16px';
    confirmBtn.style.backgroundColor = '#2885e8';
    confirmBtn.style.color = 'white';
    confirmBtn.style.border = 'none';
    confirmBtn.style.borderRadius = '4px';
    confirmBtn.style.cursor = 'pointer';
    confirmBtn.onclick = function() {
        mask.remove();
        if (onConfirm) onConfirm();
    };

    var cancelBtn = document.createElement('button');
    cancelBtn.textContent = cancelText;
    cancelBtn.style.padding = '8px 16px';
    cancelBtn.style.backgroundColor = '#666';
    cancelBtn.style.color = 'white';
    cancelBtn.style.border = 'none';
    cancelBtn.style.borderRadius = '4px';
    cancelBtn.style.cursor = 'pointer';
    cancelBtn.onclick = function() {
        mask.remove();
        if (onCancel) onCancel();
    };

    buttonContainer.appendChild(confirmBtn);
    buttonContainer.appendChild(cancelBtn);
    content.appendChild(messageDiv);
    content.appendChild(buttonContainer);
    mask.appendChild(content);
    document.body.appendChild(mask);

    setTimeout(function() { mask.classList.add('show'); }, 10);
}

window.showCommonModal = showCommonModal;
window.showConfirmModal = showConfirmModal;