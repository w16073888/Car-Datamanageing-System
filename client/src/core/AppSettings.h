#ifndef APPSETTINGS_H
#define APPSETTINGS_H

// ============================================================
// 客户端本地设置（QSettings INI，保存在客户端 exe 同目录 client_settings.ini）
//   与服务器无关的纯界面/流程偏好在此集中管理。
// ============================================================
class AppSettings
{
public:
    // 简洁模式：工单状态流转 派工中→已提单→已结算（绕过待提单）
    //   勾选后：库房可对「已派工」工单直接确认提单；前台工单查询不提供「通知提单」按钮。
    //   提示：本设置为单客户端本地保存，多台电脑需分别勾选。
    static bool simpleMode();
    static void setSimpleMode(bool on);
};

#endif // APPSETTINGS_H
