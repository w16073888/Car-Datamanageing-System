$ErrorActionPreference = 'Continue'
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
$root = [System.Windows.Automation.AutomationElement]::RootElement

Write-Output '=== top windows ==='
$cond = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::Window)
$wins = $root.FindAll([System.Windows.Automation.TreeScope]::Children, $cond)
for ($i = 0; $i -lt $wins.Count; $i++) {
    $e = $wins.Item($i)
    try {
        $r = $e.Current.BoundingRectangle
        Write-Output ("win[{0}] name='{1}' class='{2}' pid={3} rect={4},{5} {6}x{7}" -f $i, $e.Current.Name, $e.Current.ClassName, $e.Current.ProcessId, $r.X, $r.Y, $r.Width, $r.Height)
    } catch { Write-Output ("win[{0}] name='{1}' (no rect)" -f $i, $e.Current.Name) }
}

Write-Output '=== all MenuItems under root ==='
$mcond = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::MenuItem)
$mis = $root.FindAll([System.Windows.Automation.TreeScope]::Descendants, $mcond)
Write-Output ("count = " + $mis.Count)
for ($i = 0; $i -lt $mis.Count; $i++) {
    $e = $mis.Item($i)
    try {
        $r = $e.Current.BoundingRectangle
        Write-Output ("  mi[{0}] name='{1}' rect={2},{3} {4}x{5}" -f $i, $e.Current.Name, $r.X, $r.Y, $r.Width, $r.Height)
    } catch { Write-Output ("  mi[{0}] name='{1}'" -f $i, $e.Current.Name) }
}

Write-Output '=== 4s-client process ==='
Get-Process -Name '4s-client' -ErrorAction SilentlyContinue | Select-Object Id, MainWindowTitle, MainWindowHandle | Format-List

Write-Output '=== native window titles for pid ==='
$p = Get-Process -Name '4s-client' -ErrorAction SilentlyContinue
if ($p) {
    Add-Type @'
using System;
using System.Text;
using System.Runtime.InteropServices;
public class NT {
    [DllImport("user32.dll")] public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc cb, IntPtr lParam);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint pid);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowText(IntPtr hWnd, StringBuilder sb, int max);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr hWnd, StringBuilder sb, int max);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hWnd);
}
'@
    $targetPid = $p.Id
    $cb = [NT+EnumWindowsProc]{ param($hWnd, $l)
        $wpid = 0; [NT]::GetWindowThreadProcessId($hWnd, [ref]$wpid) | Out-Null
        if ($wpid -eq $targetPid) {
            $sb = New-Object System.Text.StringBuilder 256
            [NT]::GetWindowText($hWnd, $sb, 256) | Out-Null
            $sb2 = New-Object System.Text.StringBuilder 256
            [NT]::GetClassName($hWnd, $sb2, 256) | Out-Null
            $vis = [NT]::IsWindowVisible($hWnd)
            Write-Output ("  hwnd=0x{0:X} title='{1}' class='{2}' visible={3}" -f $hWnd.ToInt64(), $sb.ToString(), $sb2.ToString(), $vis)
        }
        return $true
    }
    [NT]::EnumWindows($cb, [IntPtr]::Zero) | Out-Null
}
