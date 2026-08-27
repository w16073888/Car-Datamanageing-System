$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
$root = [System.Windows.Automation.AutomationElement]::RootElement
$cond = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::Window)
$wins = $root.FindAll([System.Windows.Automation.TreeScope]::Children, $cond)
for ($i = 0; $i -lt $wins.Count; $i++) {
    $e = $wins.Item($i)
    try {
        $pid = $e.Current.ProcessId
        $cls = $e.Current.ClassName
        $name = $e.Current.Name
        $r = $e.Current.BoundingRectangle
        Write-Output ("[{0}] pid={1} class='{2}' name='{3}' rect={4},{5} {6}x{7}" -f $i, $pid, $cls, $name, $r.X, $r.Y, $r.Width, $r.Height)
    } catch { Write-Output ("[{0}] err {1}" -f $i, $_.Exception.Message) }
}
Write-Output '--- client process ---'
Get-Process -Name '4s-client' -ErrorAction SilentlyContinue | Select-Object Id, ProcessName, MainWindowTitle | Format-List
