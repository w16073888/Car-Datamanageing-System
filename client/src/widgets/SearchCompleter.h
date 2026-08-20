#ifndef SEARCHCOMPLETER_H
#define SEARCHCOMPLETER_H

#include <QObject>
#include <QStringList>
#include <QVariant>
#include <QList>

class QLineEdit;
class QStandardItemModel;
class QTreeView;

// ============================================================
// 输入框下方自动展开的多列结果下拉（替换"模糊搜索多结果"的确认弹窗）。
//
//   行为（非模态）：
//   · 无表头、多列展示（列内容由 setResults 传入，首列为主列）；
//   · 只负责"多结果"的展示与确认 —— 唯一/零匹配由调用方处理，
//     调用方保证只有 rows>1 时才调 showDropdown()，因此不会误弹；
//   · 非模态：不抢占输入焦点、不拦截鼠标。下拉打开时——
//       - 在搜索框继续输入：触发调用方重搜，下拉实时刷新并保持打开；
//       - 点击/操作菜单以外的区域：该操作正常生效，同时下拉自动收起；
//   · 鼠标单击行 / 键盘：↑↓ 移动、回车确认、Esc 关闭（焦点停留在
//     输入框上时同样生效），Tab 收起并让焦点正常移走；
//   · 选中行的 id 通过 selected(int) 返回，行文本不参与后续逻辑。
//
//   注：本类未用 QCompleter —— 其 UnfilteredPopupCompletion 会在
//   输入框文字变化时自动弹窗，无法满足"唯一结果不弹下拉"的要求，
//   故改为手控 QTreeView 弹层。
// ============================================================
class SearchCompleter : public QObject
{
    Q_OBJECT
public:
    explicit SearchCompleter(QObject *parent = nullptr);

    // 挂靠的输入框（车辆 6 字段搜索可反复指定，锚定当前输入框）
    void setEdit(QLineEdit *edit);

    // 填充结果：rows 每行=一列显示数据，ids 与行并行、不显示
    void setResults(const QList<QStringList> &rows, const QList<QVariant> &ids);

    // 弹出/收起下拉（调用方保证多结果时再调 showDropdown）
    void showDropdown();
    void hideDropdown();

    // 下拉宽度：0=按内容自适应（至少等于输入框宽）；>0=固定像素。
    // 各处宽度由页面按显示内容逐一指定。
    void setPopupWidth(int width);

    bool isVisible() const;

signals:
    void selected(int id);

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    void movePopup();
    void moveCurrent(int delta);
    void confirmCurrent();
    /// 事件接收者 obj 是否落在搜索框或下拉自身（含子控件）内
    bool isOnEditOrPopup(QObject *obj) const;

    QLineEdit *m_edit = nullptr;
    QStandardItemModel *m_model = nullptr;
    QTreeView *m_popup = nullptr;
    QList<QVariant> m_ids;
    int m_popupWidth = 0;   // 0=按内容自适应
};

#endif // SEARCHCOMPLETER_H
