#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QString>

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

    // 办公地点：结算单底部显示的地址（系统维护-系统设置-前台业务 中配置）
    //   默认 成都市双流区航都大街二段370号；本地保存，多台电脑需分别设置。
    static QString officeAddress();
    static void setOfficeAddress(const QString &addr);

    // 服务电话：结算单底部与地址同行的联系电话（系统设置-前台业务 中配置）
    //   本地保存；为空时渲染为占位 028-________
    static QString servicePhone();
    static void setServicePhone(const QString &phone);
};

#endif // APPSETTINGS_H
