$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
$root = [System.Windows.Automation.AutomationElement]::RootElement
$cond = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::Window)
$wins = $root.FindAll([System.Windows.Automation.TreeScope]::Children, $cond)
for ($i = 0; $i -lt $wins.Count; $i++) {
    $e = $wins.Item($i)
    try {
        $pid = $e.GetCurrentPropertyValue([System.Windows.Automation.AutomationElement]::ProcessIdProperty)
        $name = $e.Current.Name
        $r = $e.Current.BoundingRectangle
        Write-Output ("[{0}] pid={1} name='{2}' rect={3},{4} {5}x{6}" -f $i, $pid, $name, $r.X, $r.Y, $r.Width, $r.Height)
    } catch { Write-Output ("[{0}] err {1}" -f $i, $_.Exception.Message) }
}
