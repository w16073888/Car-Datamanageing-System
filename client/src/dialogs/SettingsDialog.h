#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

class QTabWidget;
class QCheckBox;
class QLineEdit;

// ============================================================
// 系统设置（系统维护 → 系统设置）
//   当前仅一个 tab「前台业务设置」：简洁模式开关、办公地点
// ============================================================
class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);

private:
    QTabWidget *m_tabs;
    QCheckBox *m_chkSimpleMode;   // 简洁模式：绕过待提单
    QLineEdit *m_editOfficeAddr;  // 办公地点：结算单底部地址
    QLineEdit *m_editServicePhone;// 服务电话：结算单底部与地址同行
};

#endif // SETTINGSDIALOG_H
