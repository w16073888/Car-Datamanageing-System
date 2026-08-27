$ErrorActionPreference = 'Continue'
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName System.Windows.Forms
$root = [System.Windows.Automation.AutomationElement]::RootElement

function Find-Ctl($wnd, $controlType, $name) {
    $cond = New-Object System.Windows.Automation.AndCondition(
        (New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::$controlType)),
        (New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::NameProperty, $name)))
    return $wnd.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $cond)
}

# try to invoke 业务报修 menu item
$mi = Find-Ctl $root 'MenuItem' ([char]0x4E1A + [char]0x52A1 + [char]0x62A5 + [char]0x4FEE)  # 业务报修
if (-not $mi) { Write-Output 'menu 业务报修 NOT FOUND'; } else {
    Write-Output ("menu found, rect=" + $mi.Current.BoundingRectangle)
    try {
        $p = $mi.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern)
        $p.Invoke()
        Write-Output 'invoked via InvokePattern'
    } catch {
        Write-Output ('invoke failed: ' + $_.Exception.Message)
        # fallback to mouse click
        Add-Type @'
using System;
using System.Runtime.InteropServices;
public class M3 { [DllImport("user32.dll")] public static extern bool SetCursorPos(int X, int Y); [DllImport("user32.dll")] public static extern void mouse_event(uint f, uint dx, uint dy, uint d, UIntPtr e); }
'@
        $r = $mi.Current.BoundingRectangle
        $cx = [int]($r.X + $r.Width/2); $cy = [int]($r.Y + $r.Height/2)
        [M3]::SetCursorPos($cx, $cy)
        Start-Sleep -Milliseconds 100
        [M3]::mouse_event(0x0002,0,0,0,[UIntPtr]::Zero)
        [M3]::mouse_event(0x0004,0,0,0,[UIntPtr]::Zero)
        Write-Output ("mouse clicked at $cx,$cy")
    }
}

Start-Sleep -Seconds 3
# dump top windows
$wcond = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::Window)
$wins = $root.FindAll([System.Windows.Automation.TreeScope]::Children, $wcond)
Write-Output ("top windows: " + $wins.Count)
for ($i = 0; $i -lt $wins.Count; $i++) {
    $e = $wins.Item($i)
    Write-Output ("  win[{0}] name='{1}' class='{2}'" -f $i, $e.Current.Name, $e.Current.ClassName)
}
