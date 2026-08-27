$ErrorActionPreference = 'Continue'
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName System.Windows.Forms
$root = [System.Windows.Automation.AutomationElement]::RootElement

Add-Type @'
using System;
using System.Text;
using System.Runtime.InteropServices;
public class NT {
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc cb, IntPtr lParam);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint pid);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowText(IntPtr hWnd, StringBuilder sb, int max);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr hWnd, StringBuilder sb, int max);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);
    public struct RECT { public int Left, Top, Right, Bottom; }
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
}
'@

$p = Get-Process -Name '4s-client' -ErrorAction SilentlyContinue
if (-not $p) { Write-Output 'client NOT running'; exit 1 }
$targetPid = $p.Id
Write-Output ("=== native windows of pid " + $targetPid + " ===")
$cb = [NT+EnumWindowsProc]{ param($hWnd, $l)
    $wpid = 0; [NT]::GetWindowThreadProcessId($hWnd, [ref]$wpid) | Out-Null
    if ($wpid -eq $targetPid) {
        $sb = New-Object System.Text.StringBuilder 256
        [NT]::GetWindowText($hWnd, $sb, 256) | Out-Null
        $sb2 = New-Object System.Text.StringBuilder 256
        [NT]::GetClassName($hWnd, $sb2, 256) | Out-Null
        $vis = [NT]::IsWindowVisible($hWnd)
        $r = New-Object NT+RECT
        [NT]::GetWindowRect($hWnd, [ref]$r) | Out-Null
        Write-Output ("  hwnd=0x{0:X} title='{1}' class='{2}' visible={3} rect={4},{5} {6}x{7}" -f $hWnd.ToInt64(), $sb.ToString(), $sb2.ToString(), $vis, $r.Left, $r.Top, ($r.Right-$r.Left), ($r.Bottom-$r.Top))
    }
    return $true
}
[NT]::EnumWindows($cb, [IntPtr]::Zero) | Out-Null

Write-Output '=== DPI awareness of process ==='
Add-Type @'
using System;
using System.Runtime.InteropServices;
public class DP {
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    [DllImport("shcore.dll")] public static extern int GetProcessDpiAwareness(IntPtr hprocess, out int value);
}
'@
try {
    $aware = 0
    [DP]::GetProcessDpiAwareness($p.Handle, [ref]$aware) | Out-Null
    Write-Output ("process DPI awareness = " + $aware + " (0=unaware,1=system,2=permonitor)")
} catch { Write-Output ("dpi check err: " + $_.Exception.Message) }
Write-Output ("main hwnd = 0x{0:X}" -f $p.MainWindowHandle.ToInt64())

# 尝试 SendKeys 键盘导航打开 业务报修
Write-Output '=== SendKeys keyboard nav attempt ==='
$main = $root.FindFirst([System.Windows.Automation.TreeScope]::Children, (New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::NameProperty, '科盟汽修综合数据管理')))
if ($main) {
    [NT]::SetForegroundWindow([IntPtr]$main.Current.NativeWindowHandle) | Out-Null
    Start-Sleep -Milliseconds 400
    # F10 聚焦菜单栏 -> Right 到前台业务 -> Enter 展开 -> Enter 触发首项 业务报修
    [System.Windows.Forms.SendKeys]::SendWait('{F10}')
    Start-Sleep -Milliseconds 300
    [System.Windows.Forms.SendKeys]::SendWait('{RIGHT}')
    Start-Sleep -Milliseconds 300
    [System.Windows.Forms.SendKeys]::SendWait('{ENTER}')
    Start-Sleep -Milliseconds 300
    [System.Windows.Forms.SendKeys]::SendWait('{ENTER}')
    Start-Sleep -Milliseconds 800
    $deadline = (Get-Date).AddSeconds(5)
    $fd = $null
    while ((Get-Date) -lt $deadline) {
        $fd = $root.FindFirst([System.Windows.Automation.TreeScope]::Children, (New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::NameProperty, '业务报修')))
        if ($fd) { break }
        Start-Sleep -Milliseconds 300
    }
    Write-Output ("业务报修 dialog via SendKeys: " + [bool]$fd)
} else { Write-Output 'main window NOT FOUND via UIA' }
