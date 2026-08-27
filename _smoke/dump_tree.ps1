$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
$root = [System.Windows.Automation.AutomationElement]::RootElement
$cond = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::NameProperty, '科盟汽车管理系统 - 登录')
$w = $root.FindFirst([System.Windows.Automation.TreeScope]::Children, $cond)
if (-not $w) { Write-Output 'login window NOT FOUND'; exit 1 }
Write-Output ("window: " + $w.Current.Name)
$all = $w.FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition)
Write-Output ("descendant count: " + $all.Count)
for ($i = 0; $i -lt $all.Count; $i++) {
    $e = $all.Item($i)
    $ct = $e.Current.ControlType.ProgrammaticName
    $name = $e.Current.Name
    $aid = $e.Current.AutomationId
    Write-Output ("[{0}] ct={1} name='{2}' aid='{3}'" -f $i, $ct, $name, $aid)
}
