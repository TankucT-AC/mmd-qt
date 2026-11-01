// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/DocumentLimits.h"

DocumentLimits DocumentLimits::forSize(qint64 sizeBytes)
{
    DocumentLimits limits;
    limits.m_sizeBytes = sizeBytes;

    if (sizeBytes > previewLimitBytes) {
        limits.m_tier = Tier::Manual;
        limits.m_previewEnabled = false;
        // Highlighting the source of a file this large costs more than it is
        // worth; the editor stays a plain text box.
        limits.m_editorHighlightEnabled = false;
        limits.m_renderIntervalMs = deferredRenderIntervalMs;
        limits.m_codeHighlightMaxBlockChars = 16 * 1024;
        limits.m_codeHighlightMaxTotalChars = 64 * 1024;
    } else if (sizeBytes > livePreviewLimitBytes) {
        limits.m_tier = Tier::Deferred;
        limits.m_renderIntervalMs = deferredRenderIntervalMs;
        limits.m_codeHighlightMaxBlockChars = 32 * 1024;
        limits.m_codeHighlightMaxTotalChars = 256 * 1024;
    }

    return limits;
}
