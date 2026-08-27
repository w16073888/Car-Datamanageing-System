# Win7 部署说明（Qt 5.15.2 重建版）

> 本文档针对已完成的 **Qt 5.15.2 重建**产物，说明在两台 Win7 老电脑上的部署步骤。
> 与《拆分与部署说明.md》的不同点：本版本已用 Qt 5.15.2（MinGW 64 位，Win7 兼容）重新编译，
> 并且把 MySQL 驱动所需的 VC/UCRT 运行库**应用本地打包**进了服务端部署目录，Win7 开箱即用。

---

## 1. 本次重建做了什么

| 项目 | 说明 |
|---|---|
| Qt 5.15.2 | 从 `D:\QT5.15.2\qt-everywhere-src-5.15.2` 源码编译（qtbase + qtcharts），MinGW 64 位 |
| 编译器 | `D:\Qt\Tools\mingw1310_64`（GCC 13.1，msvcrt 运行库，Win7 兼容） |
| MySQL 驱动 | `qsqlmysql.dll`，链接 MySQL 5.7 的 `libmysql.dll` |
| 服务端 | `server\release\4s-server.exe` |
| 客户端 | `client\release\4s-client.exe` |
| 初始化工具 | `server\tools\DbSetup\release\DbSetup.exe` |
| 代码兼容性修复 | 客户端修了几处 Qt6 专属 API（见第 6 节） |

## 2. 部署目录内容

### 服务端 `server\deploy\`（拷到 Win7 服务器电脑）
```
4s-server.exe         数据服务进程
DbSetup.exe           一次性建库/迁移工具
config.ini            填写 MySQL 连接信息（user/password 需改）
sql\                  init.sql + migrations\（DbSetup 使用）
Qt5Core.dll / Qt5Network.dll / Qt5Sql.dll
plugins\sqldrivers\qsqlmysql.dll
libmysql.dll          MySQL 5.7 客户端库
VCRUNTIME140.dll / VCRUNTIME140_1.dll / MSVCP140.dll   ← VC 运行库（应用本地）
ucrtbase.dll + api-ms-win-crt-*.dll                     ← UCRT（应用本地，Win7 无需装更新）
libgcc_s_seh-1.dll / libstdc++-6.dll / libwinpthread-1.dll  ← MinGW 运行库
```

### 客户端 `client\deploy\`（拷到各员工电脑，含 Win7）
```
4s-client.exe         客户端程序
config.ini            填写 server/host、server/port
logo.ico / logo.jpg
Qt5Core/Gui/Widgets/Network/Sql/PrintSupport/Charts.dll
plugins\platforms\qwindows.dll 等（不含 sqldrivers，客户端不直连数据库）
libgcc_s_seh-1.dll / libstdc++-6.dll / libwinpthread-1.dll
```

## 3. Win7 部署步骤

### 服务器电脑（Win7）
1. 安装 MySQL 5.7（zip 方式即可，与本机相同）。初始化数据目录后启动。
2. 创建应用账号并授权（用 `mysql_native_password`，绕开 Qt QMYSQL 兼容问题）：
   ```sql
   CREATE USER 'garage'@'%' IDENTIFIED WITH mysql_native_password BY '你的密码';
   GRANT ALL PRIVILEGES ON garagedb.* TO 'garage'@'%';
   ```
3. 把 `server\deploy\` 整个拷到 Win7 服务器电脑。
4. 编辑 `config.ini`：`[mysql]` 下 user/password 改成上面创建的账号。
5. 运行 `DbSetup.exe --init`（全新安装）——**只运行一次**。
6. 双击 `4s-server.exe`，验证日志输出"MySQL 连接成功"。
7. 可选：建开机自启快捷方式。

### 客户端电脑（Win7 或多台）
1. 把 `client\deploy\` 拷到各电脑。
2. 编辑 `config.ini`：`[server] host=服务器IP port=9456`。
3. 双击 `4s-client.exe`，用初始账号登录（见 `sql\init.sql` 中的管理员账号）。

## 4. Win7 注意事项

- **已内置运行库**：服务端部署目录已带 VC 运行库 + UCRT（应用本地部署），Win7 无需额外安装 KB2999226 或 VC 运行库，`libmysql.dll` 可直接加载。
- 若杀毒软件误报客户端 exe，将本项目目录加入信任/排除项（未签名 GUI 程序常见）。
- 服务端自启：用启动文件夹快捷方式（免管理员）。

## 5. 本机验证结果

- 从 `server\deploy\` 目录实测：`QSqlDatabase::isDriverAvailable("QMYSQL") = 1`（驱动+libmysql+运行库全部加载成功）。
- 连接 127.0.0.1:3306 返回"Can't connect"（仅因本机 MySQL 未启动），证明加载链路完整。

## 6. 源码兼容性改动记录（相对原 Qt6 版）

| 文件 | 改动 |
|---|---|
| `client/mainwindow.cpp` | 删除遗留的 `#include "database/DbManager.h"`（客户端不直连库） |
| `client/src/pages/FrontDeskPage.cpp` / `WarehousePage.cpp` | `QVariant(QMetaType::fromType<QString>())` → `QString()`（Qt5 无此构造） |
| `client/src/pages/QuotePage.cpp` | `&QDoubleSpinBox::valueChanged` 补 `QOverload<double>::of(...)`；加 `#include <QDebug>` |
| `client/src/pages/EmployeePage.cpp` | 加 `#include <QDialogButtonBox>` |
| `client/src/dialogs/WorkOrderDetailDialog.cpp` | 加 `#include <QDateTime>` |

Qt 5.15.2 源码补丁（`D:\QT5.15.2\qt-everywhere-src-5.15.2\qtbase\`，GCC 13 兼容）：
- `src/corelib/global/qglobal.h`、`qfloat16.h`：补 `#include <limits>`
- `src/corelib/io/qfilesystemengine_win.cpp`：FILE_ID_INFO 与新版 w32api 冲突，改为本地唯一命名结构体

## 7. 产物位置速查

```
Qt 安装目录：D:\QT5.15.2\qt515
服务端 exe ：D:\temmpcode\4s\server\release\4s-server.exe
客户端 exe ：D:\temmpcode\4s\client\release\4s-client.exe
服务端部署：D:\temmpcode\4s\server\deploy\
客户端部署：D:\temmpcode\4s\client\deploy\
```
