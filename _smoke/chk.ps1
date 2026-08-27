$ErrorActionPreference = 'Continue'
# 1) char concat type check
$t = [char]0x79D1 + [char]0x76DF
Write-Output ("char+char => type=" + $t.GetType().Name + " value='" + $t + "'")

# 2) client process
$p = Get-Process -Name '4s-client' -ErrorAction SilentlyContinue
if ($p) { Write-Output ("client pid=" + $p.Id + " title='" + $p.MainWindowTitle + "' responding=" + $p.Responding) }
else { Write-Output "client NOT running" }

# 3) top-level windows via UIA
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
$root = [System.Windows.Automation.AutomationElement]::RootElement
$cond = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::Window)
$wins = $root.FindAll([System.Windows.Automation.TreeScope]::Children, $cond)
Write-Output ("top windows: " + $wins.Count)
for ($i = 0; $i -lt $wins.Count; $i++) {
    $e = $wins.Item($i)
    Write-Output ("  win[{0}] name='{1}' pid={2}" -f $i, $e.Current.Name, $e.Current.ProcessId)
}
