// SPDX-FileCopyrightText: 2026 Total Markdown contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/FontsDialog.h"

#include "core/FontManager.h"

#include <QDialogButtonBox>
#include <QFontComboBox>
#include <QFormLayout>
#include <QPushButton>
#include <QVBoxLayout>

namespace
{

constexpr auto kBundledBody = "Inter";
constexpr auto kBundledCode = "JetBrains Mono";

} // namespace

FontsDialog::FontsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Fonts"));

    // The bundled faces are registered application fonts, so they show up in
    // the lists like any other installed family -- and the reader can pick any
    // of those instead.
    FontManager::ensureBundledFontsLoaded();

    m_body = new QFontComboBox(this);
    m_code = new QFontComboBox(this);
    m_code->setFontFilters(QFontComboBox::MonospacedFonts);

    auto *reset = new QPushButton(tr("Use &bundled fonts"), this);
    reset->setObjectName(QStringLiteral("fontsResetButton"));
    connect(reset, &QPushButton::clicked, this, [this] {
        m_body->setCurrentText(QLatin1String(kBundledBody));
        m_code->setCurrentText(QLatin1String(kBundledCode));
    });

    auto *form = new QFormLayout;
    form->addRow(tr("&Text font"), m_body);
    form->addRow(tr("&Monospace font"), m_code);
    form->addRow(reset);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

void FontsDialog::setBodyFamily(const QString &family)
{
    m_body->setCurrentText(family);
}

void FontsDialog::setCodeFamily(const QString &family)
{
    m_code->setCurrentText(family);
}

QString FontsDialog::bodyFamily() const
{
    return m_body->currentText();
}

QString FontsDialog::codeFamily() const
{
    return m_code->currentText();
}
