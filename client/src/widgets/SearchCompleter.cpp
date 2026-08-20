#include "widgets/SearchCompleter.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLineEdit>
#include <QModelIndex>
#include <QMouseEvent>
#include <QScreen>
#include <QStandardItemModel>
#include <QTreeView>

SearchCompleter::SearchCompleter(QObject *parent)
    : QObject(parent)
{
    m_model = new QStandardItemModel(this);

    // 用 Qt::Tool 而非 Qt::Popup：Qt::Popup 内部会跑嵌套事件循环并抓取鼠标，
    // 下拉弹出期间像模态框一样挡住其它操作。改为无边框工具窗后：
    //   - 显示时不抢占输入焦点（可边输入边看下拉刷新）
    //   - 不抓取鼠标、不跑嵌套循环 → 非模态，点击外部立即生效（外部点击手动收起）
    m_popup = new QTreeView(qobject_cast<QWidget *>(parent));   // 挂到页面下，随页面置顶/隐藏
    m_popup->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    m_popup->setAttribute(Qt::WA_ShowWithoutActivating);
    m_popup->setFocusPolicy(Qt::NoFocus);
    m_popup->setRootIsDecorated(false);
    m_popup->setHeaderHidden(true);                 // 不带表头
    m_popup->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_popup->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_popup->setSelectionMode(QAbstractItemView::SingleSelection);
    m_popup->setAllColumnsShowFocus(true);
    m_popup->setUniformRowHeights(true);
    m_popup->setModel(m_model);
    m_popup->installEventFilter(this);
    m_popup->viewport()->installEventFilter(this);

    // 非模态 + 菜单外自动收起：
    //  - 不抢焦点、不抓鼠标 → 搜索框可继续输入（触发调用方重搜并保持打开），外部操作直接生效；
    //  - 安装应用级事件过滤，点击/输入发生在菜单（含搜索框）以外时立即收起；
    //  - 切换到其他程序时同样收起，避免残留悬浮在其他窗口上方。
    qApp->installEventFilter(this);
    connect(qApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState st) {
        if (st != Qt::ApplicationActive)
            hideDropdown();
    });

    // 鼠标单击行 → 确认选择
    connect(m_popup, &QTreeView::clicked, this, &SearchCompleter::confirmCurrent);
}

void SearchCompleter::setEdit(QLineEdit *edit)
{
    if (m_edit == edit)
        return;
    if (m_edit)
        m_edit->removeEventFilter(this);
    m_edit = edit;
    if (m_edit)
        m_edit->installEventFilter(this);
    // 收起/刷新下拉由各调用方在 textChanged 搜索槽中显式处理：
    // 无匹配 → hideDropdown()；有匹配 → setResults()+showDropdown() 覆盖刷新。
    // 不能在 setEdit 内连 textChanged→hideDropdown：那会排在页面搜索槽之后触发，
    // 导致刚弹出的下拉又被立即收起。
}

void SearchCompleter::setResults(const QList<QStringList> &rows, const QList<QVariant> &ids)
{
    m_ids = ids;
    m_model->removeRows(0, m_model->rowCount());
    m_model->setColumnCount(0);
    if (rows.isEmpty())
        return;
    const int cols = rows.first().size();
    m_model->setColumnCount(cols);
    for (int r = 0; r < rows.size(); ++r) {
        const QStringList &row = rows.at(r);
        for (int c = 0; c < cols; ++c)
            m_model->setItem(r, c, new QStandardItem(row.value(c)));
    }
    m_popup->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

void SearchCompleter::showDropdown()
{
    if (!m_edit || m_model->rowCount() == 0)
        return;
    movePopup();
    m_popup->setCurrentIndex(m_model->index(0, 0));
    m_popup->show();
    m_popup->raise();
}

void SearchCompleter::hideDropdown()
{
    m_popup->hide();
}

bool SearchCompleter::isVisible() const
{
    return m_popup->isVisible();
}

void SearchCompleter::setPopupWidth(int width)
{
    m_popupWidth = width;
}

void SearchCompleter::movePopup()
{
    const int rowH = m_popup->sizeHintForRow(0);
    const int rows = qMin(m_model->rowCount(), 8);  // 最多显示 8 行
    const int h = rowH * rows + m_popup->frameWidth() * 2 + 4;

    int w = m_edit->width();
    if (m_popupWidth > 0) {
        w = m_popupWidth;
    } else {
        // 按各列最大内容宽度估算（QTreeView::sizeHintForColumn 是 protected，改用字体度量）
        const QFontMetrics fm(m_popup->font());
        int content = 0;
        for (int c = 0; c < m_model->columnCount(); ++c) {
            int maxW = 0;
            for (int r = 0; r < m_model->rowCount(); ++r) {
                const QString txt = m_model->index(r, c).data().toString();
                maxW = qMax(maxW, fm.horizontalAdvance(txt));
            }
            content += maxW + 20;                   // 单元格内边距
        }
        content += 24;                              // 滚动条/边框预留
        w = qMax(w, content);
    }

    QPoint p = m_edit->mapToGlobal(QPoint(0, m_edit->height()));
    // 屏幕下方/右侧放不下时向上、向左收
    if (QScreen *screen = QGuiApplication::screenAt(p)) {
        const QRect sr = screen->availableGeometry();
        if (p.y() + h > sr.bottom())
            p.setY(m_edit->mapToGlobal(QPoint(0, 0)).y() - h);
        if (p.x() + w > sr.right())
            p.setX(sr.right() - w);
        if (p.x() < sr.left())
            p.setX(sr.left());
    }
    m_popup->setGeometry(QRect(p, QSize(w, h)));
}

void SearchCompleter::moveCurrent(int delta)
{
    const int count = m_model->rowCount();
    if (count == 0)
        return;
    QModelIndex idx = m_popup->currentIndex();
    int row = idx.isValid() ? idx.row() + delta : 0;
    if (row < 0)
        row = 0;
    if (row >= count)
        row = count - 1;
    const QModelIndex ni = m_model->index(row, 0);
    m_popup->setCurrentIndex(ni);
    m_popup->scrollTo(ni);
}

void SearchCompleter::confirmCurrent()
{
    if (!m_popup->isVisible())
        return;
    const QModelIndex idx = m_popup->currentIndex();
    if (!idx.isValid())
        return;
    const int row = idx.row();
    if (row >= 0 && row < m_ids.size()) {
        m_popup->hide();
        emit selected(m_ids.at(row).toInt());
    }
}

bool SearchCompleter::eventFilter(QObject *obj, QEvent *ev)
{
    // ---- 应用级：菜单打开时，发生在菜单（含搜索框）以外的用户操作都自动收起 ----
    // 说明：非模态下拉不抢焦点不抓鼠标，因此外部点击/输入会正常到达目标控件；
    // 这里只负责"一旦菜单外发生操作，就收起菜单"，不拦截该操作本身。
    // 只响应真实控件上的操作（鼠标按下/按键/滚轮）：排除 FocusOut —— 内部焦点事件
    // 会被派发给非控件对象（如应用级 QStyle），弹层激活/焦点变化时会误判为"菜单外操作"
    // 而把刚打开的弹层关掉，导致无法点击/回车选中；真正的外部点击一定先触发
    // MouseButtonPress，Tab 则由下方对象级分支处理。
    if (m_popup->isVisible() && qobject_cast<QWidget *>(obj) && !isOnEditOrPopup(obj)) {
        if (ev->type() == QEvent::MouseButtonPress
            || ev->type() == QEvent::KeyPress
            || ev->type() == QEvent::Wheel) {
            hideDropdown();
        }
    }

    if (ev->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent *>(ev);
        // 焦点停留在输入框上（下拉已展开）：接管方向键/回车/Esc/Tab
        if (obj == m_edit && m_popup->isVisible()) {
            switch (ke->key()) {
            case Qt::Key_Up:
                moveCurrent(-1);
                return true;
            case Qt::Key_Down:
                moveCurrent(+1);
                return true;
            case Qt::Key_Return:
            case Qt::Key_Enter:
                confirmCurrent();
                return true;
            case Qt::Key_Tab:
            case Qt::Key_Backtab:
                m_popup->hide();        // Tab 收起下拉，但不吞掉事件，让焦点正常移走
                return false;
            case Qt::Key_Escape:
                m_popup->hide();
                return true;
            default:
                break;                  // 其它键（含普通字符）不下拉拦截 → 正常输入并触发重搜
            }
        }
        // 焦点落在弹层自身上：回车/Esc
        if ((obj == m_popup || obj == m_popup->viewport()) && m_popup->isVisible()) {
            if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
                confirmCurrent();
                return true;
            }
            if (ke->key() == Qt::Key_Escape) {
                m_popup->hide();
                return true;
            }
        }
    }
    return QObject::eventFilter(obj, ev);
}

bool SearchCompleter::isOnEditOrPopup(QObject *obj) const
{
    QWidget *w = qobject_cast<QWidget *>(obj);
    if (!w)
        return false;
    if (w == m_edit || w == m_popup)
        return true;
    if (m_edit && m_edit->isAncestorOf(w))
        return true;
    if (m_popup && m_popup->isAncestorOf(w))
        return true;
    return false;
}
