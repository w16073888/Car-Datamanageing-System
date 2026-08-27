$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
$root = [System.Windows.Automation.AutomationElement]::RootElement
$cond = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::NameProperty, '科盟汽车管理系统 - 登录')
$w = $root.FindFirst([System.Windows.Automation.TreeScope]::Children, $cond)
if (-not $w) { Write-Output 'login window NOT FOUND'; exit 1 }
$editCond = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::Edit)
$edits = $w.FindAll([System.Windows.Automation.TreeScope]::Descendants, $editCond)
Write-Output ("edit count: " + $edits.Count)
for ($i = 0; $i -lt $edits.Count; $i++) {
    $e = $edits.Item($i)
    Write-Output ("--- Edit index $i ---")
    try { Write-Output ("  Name = '" + $e.Current.Name + "'") } catch { Write-Output "  Name = (err)" }
    try { Write-Output ("  ClassName = '" + $e.Current.ClassName + "'") } catch {}
    try { Write-Output ("  HelpText = '" + $e.Current.HelpText + "'") } catch {}
    try { Write-Output ("  IsEnabled = " + $e.Current.IsEnabled) } catch {}
    try { Write-Output ("  IsOffscreen = " + $e.Current.IsOffscreen) } catch {}
    try {
        $r = $e.Current.BoundingRectangle
        Write-Output ("  Rect = {0},{1} {2}x{3}" -f $r.X, $r.Y, $r.Width, $r.Height)
    } catch {}
}
