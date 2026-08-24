#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

class QTabWidget;
class QCheckBox;

// ============================================================
// 系统设置（系统维护 → 系统设置）
//   当前仅一个 tab「前台业务设置」：简洁模式开关
// ============================================================
class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);

private:
    QTabWidget *m_tabs;
    QCheckBox *m_chkSimpleMode;   // 简洁模式：绕过待提单
};

#endif // SETTINGSDIALOG_H
