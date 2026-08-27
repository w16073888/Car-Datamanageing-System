$ErrorActionPreference = 'Continue'
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
$root = [System.Windows.Automation.AutomationElement]::RootElement

function Log($m) { Write-Host ("[{0}] {1}" -f (Get-Date -Format 'HH:mm:ss'), $m) }
function Is-Alive { return [bool](Get-Process -Name '4s-client' -ErrorAction SilentlyContinue) }
function Find-Ctl($wnd, $controlType, $name) {
    $cond = New-Object System.Windows.Automation.AndCondition(
        (New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::$controlType)),
        (New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::NameProperty, $name)))
    return $wnd.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $cond)
}

# main window title 科盟汽修综合数据管理
$mainTitle = ([char]0x79D1 + [char]0x76DF + [char]0x6C7D + [char]0x4FEE + [char]0x7EFC + [char]0x5408 + [char]0x6570 + [char]0x636E + [char]0x7BA1 + [char]0x7406)
$main = $root.FindFirst([System.Windows.Automation.TreeScope]::Children, (New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::NameProperty, $mainTitle)))
if (-not $main) { Log 'main window NOT FOUND'; exit 1 }
Log ('main found, alive=' + (Is-Alive))

# find menu 前台业务
$menu = Find-Ctl $main 'MenuItem' ([char]0x524D + [char]0x53F0 + [char]0x4E1A + [char]0x52A1)
if (-not $menu) { Log 'menu 前台业务 NOT FOUND'; exit 1 }
Log ('menu found, rect=' + $menu.Current.BoundingRectangle)
try {
    $menu.GetCurrentPattern([System.Windows.Automation.ExpandCollapsePattern]::Pattern).Expand()
    Log 'expanded OK'
} catch { Log ('expand err: ' + $_.Exception.Message) }
Start-Sleep -Seconds 2
Log ('alive after expand: ' + (Is-Alive))

# invoke 业务报修
$mi = $root.FindFirst([System.Windows.Automation.TreeScope]::Descendants, (New-Object System.Windows.Automation.AndCondition(
    (New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::MenuItem)),
    (New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::NameProperty, ([char]0x4E1A + [char]0x52A1 + [char]0x62A5 + [char]0x4FEE))))))
if ($mi) {
    Log '业务报修 found, invoking'
    try { $mi.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke(); Log 'invoked' }
    catch { Log ('invoke err: ' + $_.Exception.Message) }
    Start-Sleep -Seconds 2
    Log ('alive after invoke: ' + (Is-Alive))
    # check for 业务报修 window
    $fdTitle = ([char]0x4E1A + [char]0x52A1 + [char]0x62A5 + [char]0x4FEE)
    $fd = $root.FindFirst([System.Windows.Automation.TreeScope]::Children, (New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::NameProperty, $fdTitle)))
    Log ('业务报修 window present: ' + [bool]$fd)
} else {
    Log '业务报修 item NOT FOUND after expand'
    $mis = $root.FindAll([System.Windows.Automation.TreeScope]::Descendants, (New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::MenuItem)))
    Log ('total MenuItems: ' + $mis.Count)
    for ($i=0; $i -lt $mis.Count; $i++) { Log ("  mi[$i] name='" + $mis.Item($i).Current.Name + "'") }
}
Log ('final alive: ' + (Is-Alive))
