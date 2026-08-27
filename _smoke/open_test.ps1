$ErrorActionPreference = 'Continue'
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName System.Windows.Forms
$root = [System.Windows.Automation.AutomationElement]::RootElement

function Log($m) { Write-Host ("[{0}] {1}" -f (Get-Date -Format 'HH:mm:ss'), $m) }
function Wait-Win($title, $timeoutSec = 15) {
    $deadline = (Get-Date).AddSeconds($timeoutSec)
    while ((Get-Date) -lt $deadline) {
        $cond = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::NameProperty, $title)
        $el = $root.FindFirst([System.Windows.Automation.TreeScope]::Children, $cond)
        if ($el) { return $el }
        Start-Sleep -Milliseconds 300
    }
    return $null
}
function Find-Ctl($wnd, $controlType, $name) {
    $cond = New-Object System.Windows.Automation.AndCondition(
        (New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::$controlType)),
        (New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::NameProperty, $name)))
    return $wnd.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $cond)
}
function Set-Edit($edit, $text) {
    $edit.SetFocus(); Start-Sleep -Milliseconds 100
    $p = $edit.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern)
    $p.SetValue($text); Start-Sleep -Milliseconds 200
}
Add-Type @'
using System;
using System.Runtime.InteropServices;
public class MC { [DllImport("user32.dll")] public static extern bool SetCursorPos(int X, int Y); [DllImport("user32.dll")] public static extern void mouse_event(uint f, uint dx, uint dy, uint d, UIntPtr e); }
'@
function Mouse-ClickEl($el) {
    $r = $el.Current.BoundingRectangle
    $cx = [int]($r.X + $r.Width/2); $cy = [int]($r.Y + $r.Height/2)
    [MC]::SetCursorPos($cx, $cy); Start-Sleep -Milliseconds 80
    [MC]::mouse_event(0x0002,0,0,0,[UIntPtr]::Zero)
    [MC]::mouse_event(0x0004,0,0,0,[UIntPtr]::Zero)
    Start-Sleep -Milliseconds 300
}

Log 'start client'
Start-Process -FilePath 'D:\temmpcode\4s\client\client_deploy\4s-client.exe'
$login = Wait-Win '科盟汽车管理系统 - 登录' 15
if (-not $login) { Log 'FAIL login window'; exit 1 }
Log 'login window OK'
$edits = $login.FindAll([System.Windows.Automation.TreeScope]::Descendants, (New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::Edit)))
Set-Edit $edits.Item(0) '1'; Set-Edit $edits.Item(1) '123456'
$btn = Find-Ctl $login 'Button' '登 录'
Mouse-ClickEl $btn
$main = Wait-Win '科盟汽修综合数据管理' 15
if (-not $main) { Log 'FAIL main window'; exit 1 }
Log 'main window OK'

# 打开前台业务菜单（真实鼠标点击菜单栏项）
$menu = Find-Ctl $main 'MenuItem' '前台业务'
if (-not $menu) { Log 'FAIL menu 前台业务'; exit 1 }
Mouse-ClickEl $menu
Start-Sleep -Milliseconds 600
# 再真实点击 业务报修 子项
$mi = $root.FindFirst([System.Windows.Automation.TreeScope]::Descendants, (New-Object System.Windows.Automation.AndCondition(
    (New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::MenuItem)),
    (New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::NameProperty, '业务报修')))))
if (-not $mi) { Log 'FAIL 业务报修 item NOT FOUND'; exit 1 }
Log ('业务报修 item rect=' + $mi.Current.BoundingRectangle)
Mouse-ClickEl $mi
$fd = Wait-Win '业务报修' 8
if ($fd) { Log ('OK: 业务报修 dialog opened! rect=' + $fd.Current.BoundingRectangle) }
else {
    Log 'FAIL: 业务报修 dialog still not found'
    $wcond = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::Window)
    $wins = $root.FindAll([System.Windows.Automation.TreeScope]::Children, $wcond)
    Log ("top windows now: " + $wins.Count)
    for ($i=0; $i -lt $wins.Count; $i++) { Log ("  win[$i] '" + $wins.Item($i).Current.Name + "'") }
}
