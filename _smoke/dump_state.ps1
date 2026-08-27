$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
$root = [System.Windows.Automation.AutomationElement]::RootElement
$cond = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::Window)
$wins = $root.FindAll([System.Windows.Automation.TreeScope]::Children, $cond)
Write-Output ("top-level windows: " + $wins.Count)
for ($i = 0; $i -lt $wins.Count; $i++) {
    $e = $wins.Item($i)
    try {
        $r = $e.Current.BoundingRectangle
        Write-Output ("[{0}] name='{1}' rect={2},{3} {4}x{5}" -f $i, $e.Current.Name, $r.X, $r.Y, $r.Width, $r.Height)
    } catch {
        Write-Output ("[{0}] name='{1}' (no rect)" -f $i, $e.Current.Name)
    }
}
# screenshot of virtual screen
$b = [System.Windows.Forms.SystemInformation]::VirtualScreen
$bmp = New-Object System.Drawing.Bitmap($b.Width, $b.Height)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($b.Left, $b.Top, 0, 0, $bmp.Size)
$bmp.Save('D:\temmpcode\4s\_smoke\state.png', [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()
Write-Output 'saved state.png'
