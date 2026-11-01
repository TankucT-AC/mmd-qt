// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/MarkdownBuilder.h"

#include "core/HtmlFragment.h"
#include "core/ImageResolver.h"

#include <QDir>
#include <QFileInfo>
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QImage>
#include <QImageReader>
#include <QPainter>
#include <QRegularExpression>
#include <QScreen>
#include <QStack>
#include <QSvgRenderer>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QTextFrame>
#include <QTextList>
#include <QTextTable>
#include <QTextTableCell>
#include <QUrl>

extern "C" {
#include "entity.h"
#include "md4c.h"
}

#include <memory>
#include <optional>
#include <vector>

#include <algorithm>

namespace
{

constexpr unsigned kParserFlags = MD_FLAG_TABLES | MD_FLAG_STRIKETHROUGH | MD_FLAG_TASKLISTS | MD_FLAG_PERMISSIVEAUTOLINKS | MD_FLAG_COLLAPSEWHITESPACE;

constexpr int kRuleLength = 40;

QString decodeEntityText(const char *data, unsigned size)
{
    // The entity table keys include the '&' and ';' (e.g. "&amp;").
    const ENTITY *entity = entity_lookup(data, size_t(size));
    if (!entity)
        return QString::fromUtf8(data, int(size));

    const char32_t codepoints[2] = {entity->codepoints[0], entity->codepoints[1]};
    return QString::fromUcs4(codepoints, entity->codepoints[1] != 0 ? 2 : 1);
}

// MD_ATTRIBUTE strings arrive as a chain of typed substrings; entities inside
// them are not decoded by md4c itself.
QString decodeAttribute(const MD_ATTRIBUTE &attribute)
{
    QString result;
    if (attribute.size == 0)
        return result;

    for (unsigned i = 0;; ++i) {
        const unsigned begin = attribute.substr_offsets[i];
        const unsigned end = attribute.substr_offsets[i + 1];
        switch (attribute.substr_types[i]) {
        case MD_TEXT_ENTITY:
            result += decodeEntityText(attribute.text + begin, end - begin);
            break;
        case MD_TEXT_NULLCHAR:
            result += QChar::ReplacementCharacter;
            break;
        default:
            result += QString::fromUtf8(attribute.text + begin, int(end - begin));
            break;
        }
        if (end == attribute.size)
            break;
    }
    return result;
}

qreal devicePixelRatio()
{
    if (const QScreen *screen = QGuiApplication::primaryScreen())
        return screen->devicePixelRatio();
    return 1.0;
}

// Rasterises an SVG at the requested pixel size with antialiasing, so scaled
// vector art stays smooth instead of showing stair-stepped edges.
QImage rasteriseSvg(const QString &path, const QSize &pixelSize)
{
    QSvgRenderer renderer(path);
    if (!renderer.isValid() || !pixelSize.isValid())
        return QImage();

    QImage image(pixelSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    renderer.render(&painter, QRectF(QPointF(0.0, 0.0), QSizeF(pixelSize)));
    painter.end();
    return image;
}

class Builder
{
public:
    Builder(QTextDocument *document, const RenderStyle &style)
        : m_document(document)
        , m_style(style)
        , m_cursor(document)
        , m_bodyFont(style.fonts.bodyFont())
        , m_codeFont(style.fonts.codeFont())
    {
        for (int level = 1; level <= 6; ++level)
            m_headingFonts[level] = style.fonts.headingFont(level);
    }

    bool run(const QString &markdown);

    int enterBlock(MD_BLOCKTYPE type, void *detail);
    int leaveBlock(MD_BLOCKTYPE type, void *detail);
    int enterSpan(MD_SPANTYPE type, void *detail);
    int leaveSpan(MD_SPANTYPE type, void *detail);
    int text(MD_TEXTTYPE type, const char *data, unsigned size);

private:
    struct SpanFormatState {
        bool em = false;
        bool strong = false;
        bool del = false;
        bool code = false;
        int sub = 0;
        int sup = 0;
        bool kbd = false;
    };

    struct PendingBlock {
        bool valid = false;
        QTextBlockFormat format;
        QTextCharFormat baseCharFormat;
        bool attachToList = false;
    };

    struct Cell {
        std::unique_ptr<QTextDocument> document;
        bool header = false;
        Qt::Alignment alignment = Qt::AlignLeft;
    };

    struct TableState {
        unsigned columns = 0;
        unsigned headerRows = 0;
        bool inHead = false;
        std::vector<Cell> currentRow;
        std::vector<std::vector<Cell>> rows;
    };

    struct SavedState {
        QTextCursor cursor;
        PendingBlock pending;
        QTextCharFormat blockCharFormat;
        SpanFormatState spans;
        QString linkHref;
        bool firstBlock = true;
    };

    struct FrameContext {
        QTextFrame *frame = nullptr;
        QTextCursor outerCursor;
        QTextTableCellFormat quoteCellFormat; // re-applied on close (see popFrame)
        bool isQuote = false;
    };

    void setPendingBlock(const QTextBlockFormat &format, const QTextCharFormat &base, bool attachToList);
    void startBlock();
    void insertImageElement(const QString &source, const QString &alt);
    void insertChip(const QString &text, MarkdownBuilder::ChipKind kind);
    void insertVerbatim(const QString &content);
    void startVerbatimFrame(const QColor &background, const QString &language);
    void endVerbatimFrame();
    void removeFrameSpacers();
    void emitTable();

    // HTML support: a tolerant subset that makes GitHub READMEs readable.
    void renderHtmlBlock(const QString &html);
    void applyInlineHtml(const QString &html);
    void insertSummary(const QString &html);
    void renderCallout(QTextFrame *quote);

    QString linkFor(const QString &href) const;

    QTextFrame *pushFrame(const QTextFrameFormat &format);
    QTextFrame *pushQuote();
    void popFrame();

    QTextCharFormat composeCharFormat() const;
    QTextCharFormat bodyCharFormat() const;
    QTextCharFormat codeCharFormat() const;
    QTextCharFormat headingCharFormat(int level) const;
    QTextCharFormat ruleCharFormat() const;

    QTextBlockFormat paragraphFormat() const;
    QTextBlockFormat headingFormat(int level) const;
    QTextBlockFormat codeFormat(const QString &language) const;
    QTextFrameFormat codeFrameFormat(const QColor &background) const;
    QTextBlockFormat ruleFormat() const;

    QTextDocument *m_document = nullptr;
    RenderStyle m_style;
    QTextCursor m_cursor;

    // Resolved once: building a QFont per block/span would repeat
    // QFontDatabase lookups for the system font set.
    QFont m_bodyFont;
    QFont m_codeFont;
    QFont m_headingFonts[7];

    bool m_firstBlock = true;
    PendingBlock m_pending;
    QTextCharFormat m_blockCharFormat;

    SpanFormatState m_span;
    QString m_linkHref;

    bool m_taskDone = false;
    bool m_taskPending = false;
    int m_imageDepth = 0;

    bool m_inListItem = false;
    bool m_listItemFirstBlockDone = false;
    QVector<QTextListFormat> m_pendingListFormats;
    QVector<QTextList *> m_lists;
    QVector<FrameContext> m_frameStack;

    bool m_inVerbatimBlock = false;
    bool m_verbatimPendingNewline = false;
    QTextBlockFormat m_verbatimBlockFormat;
    QTextFrame *m_verbatimFrame = nullptr;
    QTextCursor m_verbatimCursor;

    std::optional<TableState> m_table = std::nullopt;
    QStack<SavedState> m_cellStack;

    // HTML subset state.
    bool m_inHtmlBlock = false;
    QString m_htmlBuffer;
    QString m_pendingImageSrc;
    QString m_imageAltBuffer;
    int m_htmlCenterDepth = 0; // inside <div align="center">
    int m_htmlDetailsDepth = 0; // inside <details>
    QVector<QString> m_htmlTagStack; // open inline tags, for matching </...>
    QString m_kbdBuffer; // text collected between <kbd> and </kbd>
    bool m_inKbd = false;
};

int onEnterBlock(MD_BLOCKTYPE type, void *detail, void *userdata)
{
    return static_cast<Builder *>(userdata)->enterBlock(type, detail);
}

int onLeaveBlock(MD_BLOCKTYPE type, void *detail, void *userdata)
{
    return static_cast<Builder *>(userdata)->leaveBlock(type, detail);
}

int onEnterSpan(MD_SPANTYPE type, void *detail, void *userdata)
{
    return static_cast<Builder *>(userdata)->enterSpan(type, detail);
}

int onLeaveSpan(MD_SPANTYPE type, void *detail, void *userdata)
{
    return static_cast<Builder *>(userdata)->leaveSpan(type, detail);
}

int onText(MD_TEXTTYPE type, const char *data, MD_SIZE size, void *userdata)
{
    return static_cast<Builder *>(userdata)->text(type, data, size);
}

bool Builder::run(const QString &markdown)
{
    const QByteArray utf8 = markdown.toUtf8();

    MD_PARSER parser = {};
    parser.abi_version = 0;
    parser.flags = kParserFlags;
    parser.enter_block = &onEnterBlock;
    parser.leave_block = &onLeaveBlock;
    parser.enter_span = &onEnterSpan;
    parser.leave_span = &onLeaveSpan;
    parser.text = &onText;

    m_document->clear();
    m_document->setIndentWidth(24); // ~2 spaces per list level
    m_cursor = QTextCursor(m_document);
    m_firstBlock = true;
    m_pending = PendingBlock{};

    const bool ok = md_parse(utf8.constData(), MD_SIZE(utf8.size()), &parser, this) == 0;
    removeFrameSpacers();
    return ok;
}

void Builder::setPendingBlock(const QTextBlockFormat &format, const QTextCharFormat &base, bool attachToList)
{
    m_pending = PendingBlock{true, format, base, attachToList};
}

void Builder::startBlock()
{
    if (!m_pending.valid)
        return;
    const PendingBlock pending = m_pending;
    m_pending = PendingBlock{};
    m_blockCharFormat = pending.baseCharFormat;

    // Inside <div align="center"> every block is centred.
    QTextBlockFormat blockFormat = pending.format;
    if (m_htmlCenterDepth > 0)
        blockFormat.setAlignment(Qt::AlignHCenter);

    if (m_firstBlock) {
        m_cursor.setBlockFormat(blockFormat);
        m_cursor.setBlockCharFormat(pending.baseCharFormat);
        m_firstBlock = false;
    } else {
        m_cursor.insertBlock(blockFormat, pending.baseCharFormat);
    }

    if (pending.attachToList) {
        if (!m_pendingListFormats.isEmpty()) {
            QTextListFormat listFormat = m_pendingListFormats.takeLast();
            // Task lists show the checkbox instead of a bullet (Zed/GitHub style).
            if (m_taskPending)
                listFormat.setStyle(QTextListFormat::ListStyleUndefined);
            listFormat.setIndent(m_lists.size() + 1);
            m_lists.append(m_cursor.createList(listFormat));
        } else if (!m_lists.isEmpty() && m_lists.last()->itemNumber(m_cursor.block()) < 0) {
            m_lists.last()->add(m_cursor.block());
        }
        if (m_inListItem)
            m_listItemFirstBlockDone = true;
    } else if (!m_lists.isEmpty() && m_inListItem && m_listItemFirstBlockDone) {
        // Continuation paragraph inside a list item: align with the markers.
        QTextBlockFormat adjusted = m_cursor.block().blockFormat();
        adjusted.setIndent(m_lists.size());
        m_cursor.setBlockFormat(adjusted);
    }

    if (m_taskPending) {
        QTextCharFormat markerFormat = pending.baseCharFormat;
        const QFontMetricsF metrics(markerFormat.font());
        const qreal side = qRound(metrics.height() * 0.7);
        markerFormat.setObjectType(MarkdownBuilder::checkboxObjectType);
        markerFormat.setProperty(MarkdownBuilder::checkboxCheckedKey, m_taskDone);
        markerFormat.setProperty(MarkdownBuilder::checkboxSizeKey, side);
        m_cursor.insertText(QString(QChar::ObjectReplacementCharacter), markerFormat);
        m_cursor.insertText(QStringLiteral("  "), pending.baseCharFormat);
        m_taskPending = false;
    }
}

QTextCharFormat Builder::composeCharFormat() const
{
    QTextCharFormat format = m_blockCharFormat;
    if (m_span.strong)
        format.setFontWeight(QFont::Bold);
    if (m_span.em)
        format.setFontItalic(true);
    if (m_span.del)
        format.setFontStrikeOut(true);
    if (m_span.code) {
        format.setFont(m_style.fonts.codeFont());
        format.setBackground(m_style.theme.markdown.codeBackground);
        format.setForeground(m_style.theme.markdown.codeText);
    }
    if (m_span.kbd) {
        QFont key = m_style.fonts.codeFont();
        key.setPointSizeF(key.pointSizeF() * 0.92);
        format.setFont(key);
        format.setBackground(m_style.theme.markdown.codeBackground);
        format.setForeground(m_style.theme.markdown.codeText);
    }
    if (m_span.sup > 0)
        format.setVerticalAlignment(QTextCharFormat::AlignSuperScript);
    else if (m_span.sub > 0)
        format.setVerticalAlignment(QTextCharFormat::AlignSubScript);
    if (!m_linkHref.isEmpty()) {
        format.setAnchor(true);
        format.setAnchorHref(m_linkHref);
        format.setForeground(m_style.theme.markdown.link);
        format.setFontUnderline(true);
    }
    return format;
}

QTextCharFormat Builder::bodyCharFormat() const
{
    QTextCharFormat format;
    format.setFont(m_bodyFont);
    format.setForeground(m_style.theme.palette.color(QPalette::Text));
    return format;
}

QTextCharFormat Builder::codeCharFormat() const
{
    QTextCharFormat format;
    format.setFont(m_codeFont);
    format.setForeground(m_style.theme.markdown.codeText);
    return format;
}

QTextCharFormat Builder::headingCharFormat(int level) const
{
    QTextCharFormat format;
    format.setFont(m_headingFonts[std::clamp(level, 1, 6)]);
    format.setForeground(level >= 6 ? m_style.theme.markdown.weakText : m_style.theme.markdown.heading);
    return format;
}

QTextCharFormat Builder::ruleCharFormat() const
{
    QTextCharFormat format;
    format.setFont(m_bodyFont);
    format.setForeground(m_style.theme.markdown.rule);
    return format;
}

QTextBlockFormat Builder::paragraphFormat() const
{
    QTextBlockFormat format;
    format.setLineHeight(122, QTextBlockFormat::ProportionalHeight);
    format.setBottomMargin(3);
    return format;
}

QTextBlockFormat Builder::headingFormat(int level) const
{
    QTextBlockFormat format;
    switch (level) {
    case 1:
        format.setTopMargin(14);
        format.setBottomMargin(5);
        break;
    case 2:
        format.setTopMargin(12);
        format.setBottomMargin(4);
        break;
    case 3:
        format.setTopMargin(10);
        format.setBottomMargin(3);
        break;
    case 6:
        format.setTopMargin(8);
        format.setBottomMargin(2);
        break;
    default:
        format.setTopMargin(8);
        format.setBottomMargin(2);
        break;
    }
    format.setProperty(MarkdownBuilder::kindKey, int(MarkdownBuilder::BlockKind::Heading));
    format.setProperty(MarkdownBuilder::headingLevelKey, level);
    return format;
}

QTextBlockFormat Builder::codeFormat(const QString &language) const
{
    // The shaded box is a QTextFrame wrapping the whole fenced region, so the
    // lines themselves carry no background, padding or outer spacing.
    QTextBlockFormat format;
    format.setTopMargin(0);
    format.setBottomMargin(0);
    format.setLeftMargin(0);
    format.setRightMargin(0);
    format.setProperty(MarkdownBuilder::kindKey, int(MarkdownBuilder::BlockKind::Code));
    format.setProperty(MarkdownBuilder::languageKey, language);
    return format;
}

QTextFrameFormat Builder::codeFrameFormat(const QColor &background) const
{
    QTextFrameFormat format;
    format.setBackground(background);
    format.setBorder(0);
    format.setPadding(8);
    format.setTopMargin(6);
    format.setBottomMargin(6);
    // Tag the frame so the view can find code boxes (e.g. for the copy button).
    format.setProperty(MarkdownBuilder::kindKey, int(MarkdownBuilder::BlockKind::Code));
    return format;
}

QTextBlockFormat Builder::ruleFormat() const
{
    QTextBlockFormat format;
    format.setAlignment(Qt::AlignHCenter);
    format.setTopMargin(6);
    format.setBottomMargin(6);
    format.setProperty(MarkdownBuilder::kindKey, int(MarkdownBuilder::BlockKind::Rule));
    return format;
}

QTextFrame *Builder::pushFrame(const QTextFrameFormat &format)
{
    FrameContext context;
    context.outerCursor = m_cursor;
    context.frame = m_cursor.insertFrame(format);
    m_firstBlock = false;
    m_frameStack.append(context);
    m_cursor = context.frame->firstCursorPosition();
    return context.frame;
}

QTextFrame *Builder::pushQuote()
{
    // GitHub-style quote: an indent plus a 4px bar on the left, no fill. Only a
    // table cell supports a single-edge border, so the quote is a 1x1 table.
    QTextTableFormat tableFormat;
    tableFormat.setBorder(0);
    tableFormat.setCellSpacing(0);
    tableFormat.setCellPadding(0);
    tableFormat.setMargin(0);
    QVector<QTextLength> constraints;
    constraints.append(QTextLength(QTextLength::PercentageLength, 100));
    tableFormat.setColumnWidthConstraints(constraints);

    FrameContext context;
    context.outerCursor = m_cursor;
    QTextTable *table = m_cursor.insertTable(1, 1, tableFormat);

    QTextTableCellFormat cellFormat;
    cellFormat.setLeftBorder(4);
    cellFormat.setLeftBorderStyle(QTextFrameFormat::BorderStyle_Solid);
    cellFormat.setLeftBorderBrush(m_style.theme.markdown.quoteBar);
    cellFormat.setLeftPadding(14);
    cellFormat.setTopPadding(2);
    cellFormat.setBottomPadding(2);
    table->cellAt(0, 0).setFormat(cellFormat);

    context.frame = table;
    context.quoteCellFormat = cellFormat;
    context.isQuote = true;
    m_frameStack.append(context);
    m_cursor = table->cellAt(0, 0).firstCursorPosition();
    // Reuse the cell's existing empty block as the first quote line.
    m_firstBlock = true;
    return table;
}

void Builder::popFrame()
{
    if (m_frameStack.isEmpty())
        return;
    const FrameContext context = m_frameStack.takeLast();
    // Cell formats are wiped when the cell's blocks are formatted, so the
    // quote bar is applied last, once the content is in place.
    if (context.isQuote) {
        if (auto *table = qobject_cast<QTextTable *>(context.frame))
            table->cellAt(0, 0).setFormat(context.quoteCellFormat);
        renderCallout(context.frame);
    }
    m_cursor = QTextCursor(m_document);
    m_cursor.setPosition(context.frame->lastPosition() + 1);
    m_firstBlock = false;
}

QString Builder::linkFor(const QString &href) const
{
    if (href.isEmpty())
        return QString();

    const QUrl url(href);
    if (!url.scheme().isEmpty())
        return href;
    if (m_style.baseDir.isEmpty())
        return href;
    return QUrl::fromLocalFile(QDir(m_style.baseDir).filePath(href)).toString();
}

int Builder::enterBlock(MD_BLOCKTYPE type, void *detail)
{
    switch (type) {
    case MD_BLOCK_DOC:
        break;
    case MD_BLOCK_QUOTE:
        pushQuote();
        break;
    case MD_BLOCK_UL:
    case MD_BLOCK_OL: {
        QTextListFormat format;
        format.setStyle(type == MD_BLOCK_OL ? QTextListFormat::ListDecimal : QTextListFormat::ListDisc);
        m_pendingListFormats.append(format);
        break;
    }
    case MD_BLOCK_LI: {
        const auto *item = static_cast<const MD_BLOCK_LI_DETAIL *>(detail);
        m_inListItem = true;
        m_listItemFirstBlockDone = false;
        m_taskDone = false;
        m_taskPending = false;
        if (item && item->is_task) {
            m_taskPending = true;
            m_taskDone = item->task_mark != ' ';
        }
        setPendingBlock(paragraphFormat(), bodyCharFormat(), true);
        break;
    }
    case MD_BLOCK_H: {
        const auto *heading = static_cast<const MD_BLOCK_H_DETAIL *>(detail);
        const int level = int(heading->level);
        setPendingBlock(headingFormat(level), headingCharFormat(level), false);
        break;
    }
    case MD_BLOCK_CODE: {
        const auto *code = static_cast<const MD_BLOCK_CODE_DETAIL *>(detail);
        startVerbatimFrame(m_style.theme.markdown.codeBackground, decodeAttribute(code->lang));
        break;
    }
    case MD_BLOCK_HTML:
        m_inHtmlBlock = true;
        m_htmlBuffer.clear();
        break;
    case MD_BLOCK_P:
        setPendingBlock(paragraphFormat(), bodyCharFormat(), m_inListItem && !m_listItemFirstBlockDone);
        break;
    case MD_BLOCK_HR:
        setPendingBlock(ruleFormat(), ruleCharFormat(), false);
        break;
    case MD_BLOCK_TABLE: {
        const auto *table = static_cast<const MD_BLOCK_TABLE_DETAIL *>(detail);
        m_table = TableState();
        m_table->columns = table->col_count;
        break;
    }
    case MD_BLOCK_THEAD:
        if (m_table)
            m_table->inHead = true;
        break;
    case MD_BLOCK_TBODY:
        if (m_table)
            m_table->inHead = false;
        break;
    case MD_BLOCK_TR:
        if (m_table)
            m_table->currentRow.clear();
        break;
    case MD_BLOCK_TH:
    case MD_BLOCK_TD: {
        if (!m_table)
            break;
        const auto *cellDetail = static_cast<const MD_BLOCK_TD_DETAIL *>(detail);

        SavedState saved;
        saved.cursor = m_cursor;
        saved.pending = m_pending;
        saved.blockCharFormat = m_blockCharFormat;
        saved.spans = m_span;
        saved.linkHref = m_linkHref;
        saved.firstBlock = m_firstBlock;
        m_cellStack.push(saved);

        Cell cell;
        cell.document = std::make_unique<QTextDocument>();
        cell.header = m_table->inHead;
        switch (cellDetail->align) {
        case MD_ALIGN_CENTER:
            cell.alignment = Qt::AlignHCenter;
            break;
        case MD_ALIGN_RIGHT:
            cell.alignment = Qt::AlignRight;
            break;
        default:
            cell.alignment = Qt::AlignLeft;
            break;
        }
        m_table->currentRow.push_back(std::move(cell));

        m_cursor = QTextCursor(m_table->currentRow.back().document.get());
        m_firstBlock = true;
        m_span = SpanFormatState{};
        m_linkHref.clear();
        setPendingBlock(paragraphFormat(), bodyCharFormat(), false);
        break;
    }
    default:
        break;
    }
    return 0;
}

int Builder::leaveBlock(MD_BLOCKTYPE type, void *detail)
{
    Q_UNUSED(detail);
    switch (type) {
    case MD_BLOCK_DOC:
        break;
    case MD_BLOCK_QUOTE:
        popFrame();
        break;
    case MD_BLOCK_UL:
    case MD_BLOCK_OL:
        // A list that never got a materialized item stays pending.
        if (m_pendingListFormats.size() > m_lists.size())
            m_pendingListFormats.removeLast();
        else if (!m_lists.isEmpty())
            m_lists.removeLast();
        break;
    case MD_BLOCK_LI:
        startBlock(); // materialize empty items, e.g. "- " with no text
        m_inListItem = false;
        m_taskDone = false;
        m_taskPending = false;
        break;
    case MD_BLOCK_H:
        startBlock(); // materialize even when there was no text callback
        break;
    case MD_BLOCK_CODE:
        endVerbatimFrame();
        break;
    case MD_BLOCK_HTML:
        m_inHtmlBlock = false;
        renderHtmlBlock(m_htmlBuffer);
        m_htmlBuffer.clear();
        break;
    case MD_BLOCK_HR:
        startBlock();
        m_cursor.insertText(QString(QChar(0x2500)).repeated(kRuleLength), ruleCharFormat());
        break;
    case MD_BLOCK_TABLE:
        emitTable();
        m_table.reset();
        break;
    case MD_BLOCK_TR:
        if (m_table && !m_table->currentRow.empty()) {
            if (m_table->currentRow.front().header)
                ++m_table->headerRows;
            m_table->rows.push_back(std::move(m_table->currentRow));
            m_table->currentRow.clear();
        }
        break;
    case MD_BLOCK_TH:
    case MD_BLOCK_TD: {
        if (m_cellStack.isEmpty())
            break;
        const SavedState saved = m_cellStack.pop();
        m_cursor = saved.cursor;
        m_pending = saved.pending;
        m_blockCharFormat = saved.blockCharFormat;
        m_span = saved.spans;
        m_linkHref = saved.linkHref;
        m_firstBlock = saved.firstBlock;
        break;
    }
    default:
        break;
    }
    return 0;
}

int Builder::enterSpan(MD_SPANTYPE type, void *detail)
{
    switch (type) {
    case MD_SPAN_EM:
        m_span.em = true;
        break;
    case MD_SPAN_STRONG:
        m_span.strong = true;
        break;
    case MD_SPAN_DEL:
        m_span.del = true;
        break;
    case MD_SPAN_CODE:
        startBlock(); // inline code may open a tight list item
        m_span.code = true;
        break;
    case MD_SPAN_A: {
        const auto *link = static_cast<const MD_SPAN_A_DETAIL *>(detail);
        startBlock();
        m_linkHref = linkFor(decodeAttribute(link->href));
        break;
    }
    case MD_SPAN_IMG: {
        const auto *image = static_cast<const MD_SPAN_IMG_DETAIL *>(detail);
        startBlock();
        ++m_imageDepth;
        // The alt text arrives as text callbacks after this point; collect it
        // and insert the image (or a chip) on leave.
        m_pendingImageSrc = decodeAttribute(image->src);
        m_imageAltBuffer.clear();
        break;
    }
    default:
        break;
    }
    return 0;
}

int Builder::leaveSpan(MD_SPANTYPE type, void *detail)
{
    Q_UNUSED(detail);
    switch (type) {
    case MD_SPAN_EM:
        m_span.em = false;
        break;
    case MD_SPAN_STRONG:
        m_span.strong = false;
        break;
    case MD_SPAN_DEL:
        m_span.del = false;
        break;
    case MD_SPAN_CODE:
        m_span.code = false;
        break;
    case MD_SPAN_A:
        m_linkHref.clear();
        break;
    case MD_SPAN_IMG:
        --m_imageDepth;
        insertImageElement(m_pendingImageSrc, m_imageAltBuffer.trimmed());
        m_pendingImageSrc.clear();
        m_imageAltBuffer.clear();
        break;
    default:
        break;
    }
    return 0;
}

void Builder::startVerbatimFrame(const QColor &background, const QString &language)
{
    // Flush any pending block so the frame starts on a line of its own.
    startBlock();

    m_verbatimBlockFormat = codeFormat(language);
    m_verbatimPendingNewline = false;
    m_inVerbatimBlock = true;

    // The frame box is the shaded region: it keeps long lines that wrap and
    // blank lines inside one clean rectangle, with padding all around.
    m_verbatimFrame = pushFrame(codeFrameFormat(background));
    m_verbatimCursor = m_cursor;
    m_verbatimCursor.setBlockFormat(m_verbatimBlockFormat);
    m_verbatimCursor.setBlockCharFormat(codeCharFormat());
    m_blockCharFormat = codeCharFormat();
}

void Builder::endVerbatimFrame()
{
    m_inVerbatimBlock = false;
    m_verbatimFrame = nullptr;
    popFrame();
}

void Builder::removeFrameSpacers()
{
    // Inserting frames leaves empty, unformatted blocks immediately before or
    // after them (and between two adjacent frames). They would render as extra
    // blank lines, so drop them.
    auto insideChildFrame = [this](const QTextBlock &block) {
        return block.isValid() && m_document->frameAt(block.position()) != m_document->rootFrame();
    };

    QVector<int> positions;
    for (QTextBlock block = m_document->firstBlock(); block.isValid(); block = block.next()) {
        if (!block.text().isEmpty())
            continue;
        if (block.blockFormat().intProperty(MarkdownBuilder::kindKey) != 0)
            continue;
        if (block.blockFormat().background().style() != Qt::NoBrush)
            continue;
        if (insideChildFrame(block))
            continue;
        if (insideChildFrame(block.previous()) || insideChildFrame(block.next()))
            positions.append(block.position());
    }

    for (int position : positions) {
        QTextCursor cursor(m_document);
        cursor.setPosition(position);
        // Deleting cannot cross a frame boundary, so collapse the spacer to
        // (almost) zero height instead of removing it.
        QTextBlockFormat format = cursor.blockFormat();
        format.setTopMargin(0);
        format.setBottomMargin(0);
        format.setLineHeight(0, QTextBlockFormat::FixedHeight);
        cursor.setBlockFormat(format);
    }
}

void Builder::insertVerbatim(const QString &content)
{
    int from = 0;
    while (from < content.size()) {
        const int newline = content.indexOf(QLatin1Char('\n'), from);
        const QString line = newline < 0 ? content.mid(from) : content.mid(from, newline - from);

        if (m_verbatimPendingNewline) {
            m_verbatimCursor.insertBlock(m_verbatimBlockFormat, m_blockCharFormat);
            m_verbatimPendingNewline = false;
        }
        if (!line.isEmpty())
            m_verbatimCursor.insertText(line, composeCharFormat());
        if (newline < 0)
            break;
        m_verbatimPendingNewline = true;
        from = newline + 1;
    }
}

int Builder::text(MD_TEXTTYPE type, const char *data, unsigned size)
{
    if (m_imageDepth > 0) {
        // Alt text of an image: kept to label a chip when the source is remote.
        switch (type) {
        case MD_TEXT_ENTITY:
            m_imageAltBuffer += decodeEntityText(data, size);
            break;
        case MD_TEXT_NULLCHAR:
            m_imageAltBuffer += QChar::ReplacementCharacter;
            break;
        case MD_TEXT_BR:
        case MD_TEXT_SOFTBR:
            m_imageAltBuffer += QLatin1Char(' ');
            break;
        default:
            m_imageAltBuffer += QString::fromUtf8(data, int(size));
            break;
        }
        return 0;
    }

    QString decoded;
    switch (type) {
    case MD_TEXT_BR:
        startBlock();
        m_cursor.insertText(QString(QChar(0x2028)), composeCharFormat());
        return 0;
    case MD_TEXT_SOFTBR:
        decoded = QStringLiteral(" ");
        break;
    case MD_TEXT_ENTITY:
        decoded = decodeEntityText(data, size);
        break;
    case MD_TEXT_NULLCHAR:
        decoded = QChar::ReplacementCharacter;
        break;
    default:
        decoded = QString::fromUtf8(data, int(size));
        break;
    }

    if (m_inHtmlBlock) {
        m_htmlBuffer += QString::fromUtf8(data, int(size));
        return 0;
    }

    if (type == MD_TEXT_HTML) {
        applyInlineHtml(QString::fromUtf8(data, int(size)));
        return 0;
    }

    if (m_inKbd && type != MD_TEXT_BR) {
        m_kbdBuffer += type == MD_TEXT_ENTITY ? decodeEntityText(data, size) : QString::fromUtf8(data, int(size));
        return 0;
    }

    if (m_inVerbatimBlock) {
        insertVerbatim(decoded);
        return 0;
    }

    startBlock();

    if (!decoded.isEmpty())
        m_cursor.insertText(decoded, composeCharFormat());
    return 0;
}

void Builder::insertImageElement(const QString &source, const QString &alt)
{
    const QString path = ImageResolver::resolve(source, m_style.baseDir);

    // A remote image (e.g. a shields.io badge) is not fetched: show its alt
    // text as a chip so the document stays readable offline.
    if (path.isEmpty() && !alt.isEmpty()) {
        insertChip(alt, MarkdownBuilder::ChipKind::Badge);
        return;
    }

    if (path.isEmpty() || !QFileInfo::exists(path)) {
        QTextCharFormat missing = m_blockCharFormat;
        missing.setForeground(m_style.theme.markdown.weakText);
        missing.setFontItalic(true);
        m_cursor.insertText(QStringLiteral("[image: %1]").arg(source), missing);
        return;
    }

    if (ImageResolver::isAnimated(path)) {
        QTextCharFormat format;
        format.setObjectType(MarkdownBuilder::gifObjectType);
        format.setProperty(MarkdownBuilder::gifPathKey, path);
        m_cursor.insertText(QString(QChar::ObjectReplacementCharacter), format);
        return;
    }

    const bool isSvg = path.endsWith(QStringLiteral(".svg"), Qt::CaseInsensitive);

    QSize natural;
    if (isSvg) {
        QSvgRenderer renderer(path);
        natural = renderer.defaultSize();
    }
    if (!natural.isValid())
        natural = QImageReader(path).size();

    QSize display = natural;
    if (natural.isValid() && natural.width() > m_style.maxImageWidth) {
        display.setWidth(m_style.maxImageWidth);
        display.setHeight(qRound(qreal(natural.height()) * m_style.maxImageWidth / natural.width()));
    }

    // SVG is resolution-independent: rasterise it at the device pixel size with
    // antialiasing and hand it over as a resource, so it stays crisp.
    if (isSvg && display.isValid()) {
        const QSize pixelSize = (QSizeF(display) * devicePixelRatio()).toSize();
        const QImage rasterised = rasteriseSvg(path, pixelSize);
        if (!rasterised.isNull()) {
            const QString name = QStringLiteral("mmd-res-") + QString::number(qHash(path), 16);
            m_document->addResource(QTextDocument::ImageResource, QUrl(name), rasterised);

            QTextImageFormat format;
            format.setName(name);
            format.setWidth(display.width());
            format.setHeight(display.height());
            m_cursor.insertText(QString(QChar::ObjectReplacementCharacter), format);
            return;
        }
    }

    QTextImageFormat format;
    format.setName(path);
    if (natural.isValid() && natural.width() > m_style.maxImageWidth) {
        format.setWidth(display.width());
        format.setHeight(display.height());
    }
    m_cursor.insertText(QString(QChar::ObjectReplacementCharacter), format);
}

void Builder::emitTable()
{
    if (!m_table || m_table->rows.empty() || m_table->columns == 0)
        return;
    TableState table = std::move(*m_table);

    if (!m_firstBlock)
        m_cursor.insertBlock(QTextBlockFormat(), bodyCharFormat());
    m_firstBlock = false;

    QTextTableFormat format;
    format.setBorder(1);
    format.setBorderBrush(QBrush(m_style.theme.markdown.tableBorder));
    format.setCellSpacing(0);
    format.setCellPadding(8);
    format.setHeaderRowCount(int(table.headerRows));
    QVector<QTextLength> constraints;
    for (unsigned column = 0; column < table.columns; ++column)
        constraints.append(QTextLength(QTextLength::PercentageLength, 100.0 / table.columns));
    format.setColumnWidthConstraints(constraints);

    QTextTable *target = m_cursor.insertTable(int(table.rows.size()), int(table.columns), format);
    for (int row = 0; row < int(table.rows.size()); ++row) {
        const int columns = (std::min)(int(table.columns), int(table.rows[row].size()));
        for (int column = 0; column < columns; ++column) {
            const Cell &cell = table.rows[row][column];
            QTextCursor cellCursor = target->cellAt(row, column).firstCursorPosition();
            if (cell.document)
                cellCursor.insertFragment(QTextDocumentFragment(cell.document.get()));
            if (cell.alignment != Qt::AlignLeft) {
                QTextBlockFormat blockFormat = cellCursor.blockFormat();
                blockFormat.setAlignment(cell.alignment);
                cellCursor.setBlockFormat(blockFormat);
            }
            if (cell.header) {
                QTextTableCellFormat cellFormat;
                cellFormat.setBackground(m_style.theme.markdown.tableHeaderBackground);
                cellFormat.setFontWeight(QFont::Bold);
                target->cellAt(row, column).setFormat(cellFormat);
            }
        }
    }

    m_cursor = QTextCursor(m_document);
    m_cursor.movePosition(QTextCursor::End);
}

void Builder::insertChip(const QString &text, MarkdownBuilder::ChipKind kind)
{
    QTextCharFormat format = m_blockCharFormat;
    format.setObjectType(MarkdownBuilder::chipObjectType);
    format.setProperty(MarkdownBuilder::chipTextKey, text);
    format.setProperty(MarkdownBuilder::chipKindKey, int(kind));
    QFont font = m_style.fonts.bodyFont();
    font.setPointSizeF(font.pointSizeF() * 0.92);
    format.setFont(font);
    m_cursor.insertText(QString(QChar::ObjectReplacementCharacter), format);
}

void Builder::applyInlineHtml(const QString &html)
{
    const QVector<HtmlToken> tokens = parseHtmlFragment(html);
    for (const HtmlToken &token : tokens) {
        switch (token.kind) {
        case HtmlToken::Text:
            if (!token.text.isEmpty()) {
                startBlock();
                m_cursor.insertText(token.text, composeCharFormat());
            }
            break;
        case HtmlToken::Comment:
        case HtmlToken::Declaration:
            break;
        case HtmlToken::StartTag:
        case HtmlToken::SelfClosingTag: {
            const QString &name = token.name;
            if (name == QLatin1String("b") || name == QLatin1String("strong")) {
                m_span.strong = true;
                m_htmlTagStack.append(name);
            } else if (name == QLatin1String("i") || name == QLatin1String("em")) {
                m_span.em = true;
                m_htmlTagStack.append(name);
            } else if (name == QLatin1String("code") || name == QLatin1String("tt")) {
                m_span.code = true;
                m_htmlTagStack.append(name);
            } else if (name == QLatin1String("sub")) {
                ++m_span.sub;
                m_htmlTagStack.append(name);
            } else if (name == QLatin1String("sup")) {
                ++m_span.sup;
                m_htmlTagStack.append(name);
            } else if (name == QLatin1String("kbd")) {
                m_inKbd = true;
                m_kbdBuffer.clear();
                m_htmlTagStack.append(name);
            } else if (name == QLatin1String("a")) {
                startBlock();
                m_linkHref = linkFor(htmlAttribute(token, QStringLiteral("href")));
                m_htmlTagStack.append(name);
            } else if (name == QLatin1String("br")) {
                startBlock();
                m_cursor.insertText(QString(QChar(0x2028)), composeCharFormat());
            } else if (name == QLatin1String("img")) {
                startBlock();
                insertImageElement(htmlAttribute(token, QStringLiteral("src")), htmlAttribute(token, QStringLiteral("alt")));
            } else if (!token.selfClosing && !isHtmlVoidElement(name)) {
                m_htmlTagStack.append(QString()); // unknown container: balanced no-op
            }
            break;
        }
        case HtmlToken::EndTag: {
            if (m_htmlTagStack.isEmpty())
                break;
            const QString open = m_htmlTagStack.takeLast();
            if (open == QLatin1String("b") || open == QLatin1String("strong"))
                m_span.strong = false;
            else if (open == QLatin1String("i") || open == QLatin1String("em"))
                m_span.em = false;
            else if (open == QLatin1String("code") || open == QLatin1String("tt"))
                m_span.code = false;
            else if (open == QLatin1String("sub"))
                m_span.sub = (std::max)(0, m_span.sub - 1);
            else if (open == QLatin1String("sup"))
                m_span.sup = (std::max)(0, m_span.sup - 1);
            else if (open == QLatin1String("kbd")) {
                m_inKbd = false;
                startBlock();
                insertChip(m_kbdBuffer.trimmed(), MarkdownBuilder::ChipKind::Key);
                m_kbdBuffer.clear();
            } else if (open == QLatin1String("a"))
                m_linkHref.clear();
            break;
        }
        }
    }
}

void Builder::insertSummary(const QString &html)
{
    // QTextDocument cannot collapse, so <summary> becomes a marked bold line.
    setPendingBlock(paragraphFormat(), bodyCharFormat(), false);
    startBlock();

    QTextCharFormat marker = bodyCharFormat();
    marker.setForeground(m_style.theme.markdown.weakText);
    m_cursor.insertText(QStringLiteral("\u25B8 "), marker);

    QTextCharFormat title = bodyCharFormat();
    title.setFontWeight(QFont::Bold);
    // The inner content is usually inline HTML; keep its text.
    QString inner;
    for (const HtmlToken &token : parseHtmlFragment(html)) {
        if (token.kind == HtmlToken::Text)
            inner += token.text;
    }
    m_cursor.insertText(inner.trimmed(), title);
}

void Builder::renderHtmlBlock(const QString &html)
{
    const QVector<HtmlToken> tokens = parseHtmlFragment(html);
    bool handled = false;

    for (int i = 0; i < tokens.size(); ++i) {
        const HtmlToken &token = tokens[i];

        if (token.kind == HtmlToken::StartTag && token.name == QLatin1String("summary")) {
            handled = true;
            QString inner;
            int j = i + 1;
            for (; j < tokens.size(); ++j) {
                if (tokens[j].kind == HtmlToken::EndTag && tokens[j].name == QLatin1String("summary"))
                    break;
                if (tokens[j].kind == HtmlToken::Text)
                    inner += tokens[j].text;
            }
            insertSummary(inner);
            i = j;
            continue;
        }

        if (token.kind == HtmlToken::StartTag && token.name == QLatin1String("table")) {
            handled = true;
            // Collect the whole table, then render it as a QTextTable.
            QVector<QVector<QString>> rows;
            QVector<bool> headerRow;
            QVector<QString> currentRow;
            bool currentHeader = false;
            bool inCell = false;
            QString cellText;
            for (int j = i + 1; j < tokens.size(); ++j) {
                const HtmlToken &t = tokens[j];
                if (t.kind == HtmlToken::EndTag && t.name == QLatin1String("table"))
                    break;
                if (t.kind == HtmlToken::StartTag && t.name == QLatin1String("tr")) {
                    currentRow.clear();
                    currentHeader = false;
                    continue;
                }
                if (t.kind == HtmlToken::EndTag && t.name == QLatin1String("tr") && !currentRow.isEmpty()) {
                    rows.append(currentRow);
                    headerRow.append(currentHeader);
                    continue;
                }
                if (t.kind == HtmlToken::StartTag && (t.name == QLatin1String("td") || t.name == QLatin1String("th"))) {
                    inCell = true;
                    cellText.clear();
                    if (t.name == QLatin1String("th"))
                        currentHeader = true;
                    continue;
                }
                if (t.kind == HtmlToken::EndTag && (t.name == QLatin1String("td") || t.name == QLatin1String("th"))) {
                    if (inCell)
                        currentRow.append(cellText.trimmed());
                    inCell = false;
                    continue;
                }
                if (inCell && t.kind == HtmlToken::Text)
                    cellText += t.text;
            }
            if (!rows.isEmpty()) {
                if (!m_firstBlock)
                    m_cursor.insertBlock(QTextBlockFormat(), bodyCharFormat());
                m_firstBlock = false;
                unsigned columns = 0;
                for (const QVector<QString> &row : rows)
                    columns = (std::max)(columns, unsigned(row.size()));
                QTextTableFormat format;
                format.setBorder(1);
                format.setBorderBrush(QBrush(m_style.theme.markdown.tableBorder));
                format.setCellSpacing(0);
                format.setCellPadding(8);
                QVector<QTextLength> constraints;
                for (unsigned column = 0; column < columns; ++column)
                    constraints.append(QTextLength(QTextLength::PercentageLength, 100.0 / columns));
                format.setColumnWidthConstraints(constraints);
                QTextTable *table = m_cursor.insertTable(int(rows.size()), int(columns), format);
                for (int row = 0; row < rows.size(); ++row) {
                    for (int column = 0; column < rows[row].size(); ++column) {
                        QTextCursor cellCursor = table->cellAt(row, column).firstCursorPosition();
                        cellCursor.insertText(rows[row][column], bodyCharFormat());
                        if (headerRow[row]) {
                            QTextTableCellFormat cellFormat;
                            cellFormat.setBackground(m_style.theme.markdown.tableHeaderBackground);
                            cellFormat.setFontWeight(QFont::Bold);
                            table->cellAt(row, column).setFormat(cellFormat);
                        }
                    }
                }
                m_cursor = QTextCursor(m_document);
                m_cursor.movePosition(QTextCursor::End);
            }
            // Skip past the table.
            for (int j = i + 1; j < tokens.size(); ++j) {
                if (tokens[j].kind == HtmlToken::EndTag && tokens[j].name == QLatin1String("table")) {
                    i = j;
                    break;
                }
            }
            continue;
        }

        if ((token.kind == HtmlToken::StartTag || token.kind == HtmlToken::SelfClosingTag) && token.name == QLatin1String("img")) {
            handled = true;
            startBlock();
            insertImageElement(htmlAttribute(token, QStringLiteral("src")), htmlAttribute(token, QStringLiteral("alt")));
            continue;
        }

        if (token.kind == HtmlToken::StartTag && token.name == QLatin1String("div")) {
            handled = true;
            if (htmlAttribute(token, QStringLiteral("align")).compare(QLatin1String("center"), Qt::CaseInsensitive) == 0)
                ++m_htmlCenterDepth;
            continue;
        }
        if (token.kind == HtmlToken::EndTag && token.name == QLatin1String("div")) {
            handled = true;
            if (m_htmlCenterDepth > 0)
                --m_htmlCenterDepth;
            continue;
        }

        if (token.kind == HtmlToken::StartTag && token.name == QLatin1String("details")) {
            handled = true;
            ++m_htmlDetailsDepth;
            continue;
        }
        if (token.kind == HtmlToken::EndTag && token.name == QLatin1String("details")) {
            handled = true;
            if (m_htmlDetailsDepth > 0)
                --m_htmlDetailsDepth;
            continue;
        }
    }

    // Nothing we understand: keep the raw HTML visible rather than dropping it.
    if (!handled) {
        const QString raw = html.trimmed();
        if (!raw.isEmpty()) {
            if (!m_firstBlock)
                m_cursor.insertBlock(paragraphFormat(), bodyCharFormat());
            m_firstBlock = false;
            QTextCharFormat format = codeCharFormat();
            format.setForeground(m_style.theme.markdown.weakText);
            m_cursor.insertText(raw, format);
        }
    }
}

void Builder::renderCallout(QTextFrame *quote)
{
    auto *table = qobject_cast<QTextTable *>(quote);
    if (!table)
        return;

    QTextTableCell cell = table->cellAt(0, 0);
    const QTextBlock first = cell.firstCursorPosition().block();
    static const QRegularExpression pattern(QStringLiteral("^\\[!(NOTE|TIP|IMPORTANT|WARNING|CAUTION)\\]\\s*"), QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = pattern.match(first.text());
    if (!match.hasMatch())
        return;

    const QString type = match.captured(1).toUpper();
    QColor color;
    QString label;
    if (type == QLatin1String("TIP")) {
        color = m_style.theme.markdown.taskDone;
        label = QStringLiteral("Tip");
    } else if (type == QLatin1String("IMPORTANT")) {
        color = m_style.theme.syntax.annotation;
        label = QStringLiteral("Important");
    } else if (type == QLatin1String("WARNING")) {
        color = m_style.theme.syntax.warning;
        label = QStringLiteral("Warning");
    } else if (type == QLatin1String("CAUTION")) {
        color = m_style.theme.syntax.error;
        label = QStringLiteral("Caution");
    } else {
        color = m_style.theme.markdown.link;
        label = QStringLiteral("Note");
    }

    // Replace the [!TYPE] marker with a coloured label; keep any text that
    // followed it on the same paragraph (a soft break) as a new block.
    const QString rest = first.text().mid(match.capturedLength(0)).trimmed();
    QTextCursor cursor(m_document);
    cursor.setPosition(first.position());
    cursor.setPosition(first.position() + first.length() - 1, QTextCursor::KeepAnchor);
    QTextCharFormat labelFormat;
    labelFormat.setFontWeight(QFont::Bold);
    labelFormat.setForeground(color);
    cursor.insertText(label, labelFormat);
    if (!rest.isEmpty()) {
        cursor.insertBlock(paragraphFormat(), bodyCharFormat());
        cursor.insertText(rest, bodyCharFormat());
    }

    // Tint the bar and give the box a faint wash of the type colour.
    QTextTableCellFormat cellFormat = cell.format().toTableCellFormat();
    cellFormat.setLeftBorderBrush(color);
    QColor wash = color;
    wash.setAlpha(20);
    cellFormat.setBackground(wash);
    cell.setFormat(cellFormat);
}

} // namespace

bool MarkdownBuilder::build(QTextDocument *target, const QString &markdown, const RenderStyle &style)
{
    Builder builder(target, style);
    return builder.run(markdown);
}
