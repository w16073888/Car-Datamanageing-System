$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
$root = [System.Windows.Automation.AutomationElement]::RootElement

function Find-Ctl($wnd, $controlType, $name) {
    $cond = New-Object System.Windows.Automation.AndCondition(
        (New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::$controlType)),
        (New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::NameProperty, $name)))
    return $wnd.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $cond)
}

$main = $root.FindFirst([System.Windows.Automation.TreeScope]::Children, (New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::NameProperty, '科盟汽修综合数据管理')))
if (-not $main) { Write-Output 'main NOT FOUND'; exit 1 }
# activate
try { $main.GetCurrentPattern([System.Windows.Automation.WindowPattern]::Pattern).SetWindowVisualState([System.Windows.Automation.WindowVisualState]::Normal) } catch {}
try { $main.SetFocus() } catch {}
Start-Sleep -Milliseconds 300

$menuRepair = Find-Ctl $main 'MenuItem' '前台业务'
if (-not $menuRepair) { Write-Output 'menu 前台业务 NOT FOUND'; exit 1 }
Write-Output ("menu found, rect=" + $menuRepair.Current.BoundingRectangle)

# try expandcollaps
$opened = $false
try {
    $ec = $menuRepair.GetCurrentPattern([System.Windows.Automation.ExpandCollapsePattern]::Pattern)
    $ec.Expand()
    $opened = $true
    Write-Output 'expand via ExpandCollapsePattern'
} catch { Write-Output ('ExpandCollapse err: ' + $_.Exception.Message) }

if (-not $opened) {
    # real mouse click
    Add-Type @'
using System;
using System.Runtime.InteropServices;
public class M2 {
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int X, int Y);
    [DllImport("user32.dll")] public static extern void mouse_event(uint f, uint dx, uint dy, uint d, UIntPtr e);
}
'@
    $r = $menuRepair.Current.BoundingRectangle
    $cx = [int]($r.X + $r.Width/2); $cy = [int]($r.Y + $r.Height/2)
    [M2]::SetCursorPos($cx, $cy)
    Start-Sleep -Milliseconds 100
    [M2]::mouse_event(0x0002,0,0,0,[UIntPtr]::Zero)
    [M2]::mouse_event(0x0004,0,0,0,[UIntPtr]::Zero)
    Write-Output ("mouse clicked at $cx,$cy")
}
Start-Sleep -Milliseconds 800

# screenshot full screen
$b = [System.Windows.Forms.SystemInformation]::VirtualScreen
$bmp = New-Object System.Drawing.Bitmap($b.Width, $b.Height)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($b.Left, $b.Top, 0, 0, $bmp.Size)
$bmp.Save('D:\temmpcode\4s\_smoke\menu_state.png', [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()
Write-Output 'saved menu_state.png'

# dump top-level windows
$wcond = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::Window)
$wins = $root.FindAll([System.Windows.Automation.TreeScope]::Children, $wcond)
Write-Output ("top windows: " + $wins.Count)
for ($i = 0; $i -lt $wins.Count; $i++) {
    $e = $wins.Item($i)
    Write-Output ("  win[{0}] name='{1}' class='{2}'" -f $i, $e.Current.Name, $e.Current.ClassName)
}

# dump menu items anywhere under root
$micond = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::MenuItem)
$mis = $root.FindAll([System.Windows.Automation.TreeScope]::Descendants, $micond)
Write-Output ("all MenuItems: " + $mis.Count)
for ($i = 0; $i -lt $mis.Count; $i++) {
    Write-Output ("  mi[{0}] name='{1}'" -f $i, $mis.Item($i).Current.Name)
}
