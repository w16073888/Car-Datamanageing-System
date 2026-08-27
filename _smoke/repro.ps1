$ErrorActionPreference = 'Continue'
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName System.Windows.Forms
$root = [System.Windows.Automation.AutomationElement]::RootElement

function Log($m) { Write-Host ("[{0}] {1}" -f (Get-Date -Format 'HH:mm:ss'), $m) }
function Wait-Win($title, $timeoutSec = 20) {
    $deadline = (Get-Date).AddSeconds($timeoutSec)
    while ((Get-Date) -lt $deadline) {
        $cond = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::NameProperty, $title)
        $el = $root.FindFirst([System.Windows.Automation.TreeScope]::Children, $cond)
        if ($el) { return $el }
        Start-Sleep -Milliseconds 300
    }
    return $null
}
function Find-Ctl($wnd, $controlType, $name) {
    $cond = New-Object System.Windows.Automation.AndCondition(
        (New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::$controlType)),
        (New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::NameProperty, $name)))
    return $wnd.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $cond)
}
function Set-Edit($edit, $text) {
    $edit.SetFocus(); Start-Sleep -Milliseconds 100
    $p = $edit.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern)
    $p.SetValue($text); Start-Sleep -Milliseconds 200
}
function Click-Btn($btn) {
    $p = $btn.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern); $p.Invoke()
    Start-Sleep -Milliseconds 400
}
function Is-Alive { return [bool](Get-Process -Name '4s-client' -ErrorAction SilentlyContinue) }

# 1. restart client
Log "restarting client"
Get-Process -Name '4s-client' -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 800
Start-Process -FilePath 'D:\temmpcode\4s\client\client_deploy\4s-client.exe'
Start-Sleep -Seconds 2
Log ("alive after start: " + (Is-Alive))

# 2. login
$login = Wait-Win ([char]0x79D1 + [char]0x76DF + [char]0x6C7D + [char]0x8F66 + [char]0x7BA1 + [char]0x7406 + [char]0x7CFB + [char]0x7EDF + [char]0x20 + [char]0x2D + [char]0x20 + [char]0x767B + [char]0x5F55) 10
if (-not $login) { Log 'login window not found'; exit 1 }
Log 'login window OK'
$edits = $login.FindAll([System.Windows.Automation.TreeScope]::Descendants, (New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::Edit)))
Set-Edit $edits.Item(0) '1'
Set-Edit $edits.Item(1) '123456'
$btn = Find-Ctl $login 'Button' ([char]0x767B + [char]0x20 + [char]0x5F55)  # 登 录
Click-Btn $btn

$main = Wait-Win ([char]0x79D1 + [char]0x76DF + [char]0x6C7D + [char]0x8F66 + [char]0x7BA1 + [char]0x7406 + [char]0x7EFC + [char]0x5408 + [char]0x6570 + [char]0x636E + [char]0x7BA1 + [char]0x7406) 15
if (-not $main) { Log 'main window not found'; exit 1 }
Log 'main window OK, logged in'

# 3. find + expand 前台业务 menu, then watch process
$menu = Find-Ctl $main 'MenuItem' ([char]0x524D + [char]0x53F0 + [char]0x4E1A + [char]0x52A1)  # 前台业务
if (-not $menu) { Log 'menu 前台业务 NOT FOUND'; exit 1 }
Log 'menu found, expanding...'
try {
    $menu.GetCurrentPattern([System.Windows.Automation.ExpandCollapsePattern]::Pattern).Expand()
    Log 'expanded OK'
} catch { Log ('expand err: ' + $_.Exception.Message) }
Start-Sleep -Seconds 1
Log ("alive after expand: " + (Is-Alive))
Start-Sleep -Seconds 3
Log ("alive after expand+3s: " + (Is-Alive))

# 4. find 业务报修 item and invoke
$mi = $root.FindFirst([System.Windows.Automation.TreeScope]::Descendants, (New-Object System.Windows.Automation.AndCondition(
    (New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::MenuItem)),
    (New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::NameProperty, ([char]0x4E1A + [char]0x52A1 + [char]0x62A5 + [char]0x4FEE))))))  # 业务报修
if ($mi) {
    Log '业务报修 item found, invoking...'
    try { $mi.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke(); Log 'invoked' }
    catch { Log ('invoke err: ' + $_.Exception.Message) }
    Start-Sleep -Seconds 2
    Log ("alive after invoke: " + (Is-Alive))
    $fd = Wait-Win ([char]0x4E1A + [char]0x52A1 + [char]0x62A5 + [char]0x4FEE) 6
    Log ("业务报修 dialog found: " + [bool]$fd)
} else {
    Log '业务报修 item NOT FOUND after expand'
    # dump all menu items to see state
    $mis = $root.FindAll([System.Windows.Automation.TreeScope]::Descendants, (New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::MenuItem)))
    Log ("total MenuItems now: " + $mis.Count)
    Log ("alive now: " + (Is-Alive))
}
