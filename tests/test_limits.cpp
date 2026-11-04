// SPDX-FileCopyrightText: 2026 Total Markdown contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "core/DocumentLimits.h"

class LimitsTest : public QObject
{
    Q_OBJECT

private slots:
    void ordinaryDocumentKeepsEverythingOn();
    void megabyteDocumentDefersThePreview();
    void hugeDocumentPreviewsOnRequestOnly();
    void thresholdsAreExclusive();
    void codeCapsNeverGrowWithSize();
};

void LimitsTest::ordinaryDocumentKeepsEverythingOn()
{
    const DocumentLimits limits = DocumentLimits::forSize(64 * 1024);

    QVERIFY(limits.tier() == DocumentLimits::Tier::Interactive);
    QVERIFY(!limits.degraded());
    QVERIFY(limits.previewEnabled());
    QVERIFY(limits.editorHighlightEnabled());
    QCOMPARE(limits.renderIntervalMs(), DocumentLimits::defaultRenderIntervalMs);
    QVERIFY(limits.codeHighlightMaxBlockChars() > 0);
}

void LimitsTest::megabyteDocumentDefersThePreview()
{
    const DocumentLimits limits = DocumentLimits::forSize(2 * 1024 * 1024);

    QVERIFY(limits.tier() == DocumentLimits::Tier::Deferred);
    QVERIFY(limits.degraded());
    // The preview still arrives, but only once typing pauses for a while.
    QVERIFY(limits.previewEnabled());
    QVERIFY(limits.renderIntervalMs() > DocumentLimits::defaultRenderIntervalMs);
    QVERIFY(limits.editorHighlightEnabled());
}

void LimitsTest::hugeDocumentPreviewsOnRequestOnly()
{
    const DocumentLimits limits = DocumentLimits::forSize(8 * 1024 * 1024);

    QVERIFY(limits.tier() == DocumentLimits::Tier::Manual);
    QVERIFY(!limits.previewEnabled());
    QVERIFY(!limits.editorHighlightEnabled());
}

void LimitsTest::thresholdsAreExclusive()
{
    const qint64 live = DocumentLimits::livePreviewLimitBytes;
    const qint64 full = DocumentLimits::previewLimitBytes;

    QVERIFY(DocumentLimits::forSize(0).tier() == DocumentLimits::Tier::Interactive);
    QVERIFY(DocumentLimits::forSize(live).tier() == DocumentLimits::Tier::Interactive);
    QVERIFY(DocumentLimits::forSize(live + 1).tier() == DocumentLimits::Tier::Deferred);
    QVERIFY(DocumentLimits::forSize(full).tier() == DocumentLimits::Tier::Deferred);
    QVERIFY(DocumentLimits::forSize(full + 1).tier() == DocumentLimits::Tier::Manual);
}

void LimitsTest::codeCapsNeverGrowWithSize()
{
    // A bigger document never gets a bigger budget: the policy is monotonic.
    const DocumentLimits small = DocumentLimits::forSize(1024);
    const DocumentLimits medium = DocumentLimits::forSize(2 * 1024 * 1024);
    const DocumentLimits large = DocumentLimits::forSize(8 * 1024 * 1024);

    QVERIFY(small.codeHighlightMaxBlockChars() >= medium.codeHighlightMaxBlockChars());
    QVERIFY(medium.codeHighlightMaxBlockChars() >= large.codeHighlightMaxBlockChars());
    QVERIFY(small.codeHighlightMaxTotalChars() >= medium.codeHighlightMaxTotalChars());
    QVERIFY(medium.codeHighlightMaxTotalChars() >= large.codeHighlightMaxTotalChars());
}

QTEST_MAIN(LimitsTest)
#include "test_limits.moc"
