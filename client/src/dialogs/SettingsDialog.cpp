#include "SettingsDialog.h"
#include "core/AppSettings.h"

#include <QTabWidget>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("系统设置");
    resize(540, 340);

    auto *mainLayout = new QVBoxLayout(this);

    // ---- Tab 部件：目前仅「前台业务设置」 ----
    m_tabs = new QTabWidget(this);
    {
        QWidget *pageFront = new QWidget;
        QVBoxLayout *fl = new QVBoxLayout(pageFront);
        fl->setContentsMargins(20, 20, 20, 20);
        fl->setSpacing(12);

        m_chkSimpleMode = new QCheckBox("简洁模式", pageFront);
        m_chkSimpleMode->setChecked(AppSettings::simpleMode());
        m_chkSimpleMode->setStyleSheet("font-size:15px;font-weight:bold;");
        fl->addWidget(m_chkSimpleMode);

        auto *tip = new QLabel(
            "勾选后工单状态流转为 派工中→已提单→已结算（绕过待提单）：\n"
            "  · 库房工作台-工单结算/提单 可对「派工中」工单直接确认提单；\n"
            "  · 前台业务-工单查询 不再显示「通知提单」按钮。\n"
            "提示：本设置保存在本客户端（client_settings.ini），多台电脑需分别设置。",
            pageFront);
        tip->setWordWrap(true);
        tip->setStyleSheet("color:#7f8c8d;font-size:12px;");
        fl->addWidget(tip);
        fl->addStretch();

        m_tabs->addTab(pageFront, "前台业务设置");
    }
    mainLayout->addWidget(m_tabs);

    // ---- 底部按钮 ----
    QHBoxLayout *btns = new QHBoxLayout;
    btns->addStretch();
    QPushButton *btnOk = new QPushButton("保存");
    QPushButton *btnCancel = new QPushButton("取消");
    btnOk->setStyleSheet("QPushButton{padding:6px 24px;background:#3498db;color:#fff;border:none;border-radius:3px;}"
                         "QPushButton:hover{background:#2980b9;}");
    btnCancel->setStyleSheet("QPushButton{padding:6px 24px;border:1px solid #bdc3c7;border-radius:3px;background:#ecf0f1;}"
                             "QPushButton:hover{background:#d5dbdb;}");
    btns->addWidget(btnOk);
    btns->addWidget(btnCancel);
    mainLayout->addLayout(btns);

    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(btnOk, &QPushButton::clicked, this, [this]() {
        AppSettings::setSimpleMode(m_chkSimpleMode->isChecked());
        accept();
    });
}
