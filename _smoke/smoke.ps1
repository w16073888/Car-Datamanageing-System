# 冒烟测试：业务保修「锁定车辆时清空预计备件列表」
# 流程：重启新client → 登录 → 前台业务/业务报修 → 锁定川B14250 → 添加备件"机"
#      → 验证备件表1行 → 取消派工 → 锁定111 → 验证备件表0行（本次修复点）
$ErrorActionPreference = 'Stop'
$log = 'D:\temmpcode\4s\_smoke\smoke.log'
$shot = 'D:\temmpcode\4s\_smoke'
if (Test-Path $log) { Remove-Item $log -Force }
function Log($m) {
    $line = "[{0}] {1}" -f (Get-Date -Format 'HH:mm:ss.fff'), $m
    Add-Content -Path $log -Value $line -Encoding UTF8
    Write-Host $line
}

Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms

Add-Type @'
using System;
using System.Runtime.InteropServices;
public class Win32Mouse {
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int X, int Y);
    [DllImport("user32.dll")] public static extern void mouse_event(uint dwFlags, uint dx, uint dy, uint dwData, UIntPtr dwExtraInfo);
}
public class Win32Kbd {
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
}
'@

function SendKeysTo($hwnd, [int[]]$vkCodes) {
    # 直接向指定窗口投递键盘消息（不依赖焦点），菜单导航用
    foreach ($vk in $vkCodes) {
        [Win32Kbd]::PostMessage($hwnd, 0x0100, [IntPtr]$vk, [IntPtr]::Zero)  # WM_KEYDOWN
        Start-Sleep -Milliseconds 100
        [Win32Kbd]::PostMessage($hwnd, 0x0101, [IntPtr]$vk, [IntPtr]::Zero)  # WM_KEYUP
        Start-Sleep -Milliseconds 200
    }
}

# ---------- helpers ----------
$root = [System.Windows.Automation.AutomationElement]::RootElement
function Wait-Window($title, $timeoutSec = 25) {
    $deadline = (Get-Date).AddSeconds($timeoutSec)
    while ((Get-Date) -lt $deadline) {
        $cond = New-Object System.Windows.Automation.PropertyCondition(
            [System.Windows.Automation.AutomationElement]::NameProperty, $title)
        $el = $root.FindFirst([System.Windows.Automation.TreeScope]::Children, $cond)
        if ($el) { return $el }
        Start-Sleep -Milliseconds 300
    }
    return $null
}

function Find-Ctl($wnd, $controlType, $name) {
    $cond = New-Object System.Windows.Automation.AndCondition(
        (New-Object System.Windows.Automation.PropertyCondition(
            [System.Windows.Automation.AutomationElement]::ControlTypeProperty,
            [System.Windows.Automation.ControlType]::$controlType)),
        (New-Object System.Windows.Automation.PropertyCondition(
            [System.Windows.Automation.AutomationElement]::NameProperty, $name)))
    return $wnd.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $cond)
}

function Wait-Ctl($wnd, $controlType, $name, $timeoutSec = 15) {
    $deadline = (Get-Date).AddSeconds($timeoutSec)
    while ((Get-Date) -lt $deadline) {
        $el = Find-Ctl $wnd $controlType $name
        if ($el) { return $el }
        Start-Sleep -Milliseconds 300
    }
    return $null
}

function Set-EditText($edit, $text) {
    $edit.SetFocus()
    Start-Sleep -Milliseconds 120
    $p = $edit.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern)
    $p.SetValue($text)
    Start-Sleep -Milliseconds 250
}

function Click-Button($btn) {
    try {
        $p = $btn.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern)
        $p.Invoke()
    } catch {
        Mouse-Click $btn
    }
    Start-Sleep -Milliseconds 400
}

function Invoke-OrClick($el) {
    try {
        $p = $el.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern)
        $p.Invoke()
        return
    } catch {}
    Mouse-Click $el
}

function Activate-Window($wnd) {
    try {
        $p = $wnd.GetCurrentPattern([System.Windows.Automation.WindowPattern]::Pattern)
        $p.SetWindowVisualState([System.Windows.Automation.WindowVisualState]::Normal)
    } catch {}
    try { $wnd.SetFocus() } catch {}
    Start-Sleep -Milliseconds 300
}

function Mouse-Click($el) {
    $r = $el.Current.BoundingRectangle
    $cx = [int]($r.X + $r.Width / 2); $cy = [int]($r.Y + $r.Height / 2)
    [Win32Mouse]::SetCursorPos($cx, $cy)
    Start-Sleep -Milliseconds 60
    [Win32Mouse]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)
    [Win32Mouse]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 150
}

function Find-Table($wnd) {
    foreach ($ct in 'Table', 'DataGrid') {
        $cond = New-Object System.Windows.Automation.PropertyCondition(
            [System.Windows.Automation.AutomationElement]::ControlTypeProperty,
            [System.Windows.Automation.ControlType]::$ct)
        $t = $wnd.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $cond)
        if ($t) { return $t }
    }
    return $null
}

function Get-TableRows($table) {
    try {
        $p = $table.GetCurrentPattern([System.Windows.Automation.TablePattern]::Pattern)
        return $p.Current.RowCount
    } catch {
        $cond = New-Object System.Windows.Automation.PropertyCondition(
            [System.Windows.Automation.AutomationElement]::ControlTypeProperty,
            [System.Windows.Automation.ControlType]::Row)
        return $table.FindAll([System.Windows.Automation.TreeScope]::Children, $cond).Count
    }
}

function Snap-Window($wnd, $name) {
    try {
        $r = $wnd.Current.BoundingRectangle
        if ($r.Width -le 1 -or $r.Height -le 1) { Log "  (snap skip: $name not visible)"; return }
        $bmp = New-Object System.Drawing.Bitmap([int]$r.Width, [int]$r.Height)
        $g = [System.Drawing.Graphics]::FromImage($bmp)
        $g.CopyFromScreen([int]$r.X, [int]$r.Y, 0, 0, $bmp.Size)
        $bmp.Save("$shot\$name.png", [System.Drawing.Imaging.ImageFormat]::Png)
        $g.Dispose(); $bmp.Dispose()
        Log "  snap: $name.png ($([int]$r.Width)x$([int]$r.Height))"
    } catch { Log "  snap FAIL $name : $($_.Exception.Message)" }
}

# ---------- 1. kill old client & start new ----------
Log "=== 1. restart client ==="
Get-Process -Name '4s-client' -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 800
Start-Process -FilePath 'D:\temmpcode\4s\client\client_deploy\4s-client.exe'
Log "started new client, waiting for login window..."

$login = Wait-Window '科盟汽车管理系统 - 登录' 20
if (-not $login) { Log 'FAIL: login window not found'; exit 1 }
Log "login window appeared."
Snap-Window $login '01-login'

# ---------- 2. login ----------
Log "=== 2. login (工号=1, 密码=123456) ==="
$editCondL = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::Edit)
$loginEdits = $login.FindAll([System.Windows.Automation.TreeScope]::Descendants, $editCondL)
if ($loginEdits.Count -lt 2) { Log ('FAIL: login edits not found, count=' + $loginEdits.Count); exit 1 }
$editEmp = $loginEdits.Item(0)   # 工号
$editPwd = $loginEdits.Item(1)   # 密码
Set-EditText $editEmp '1'
Set-EditText $editPwd '123456'
$btnLogin = Wait-Ctl $login 'Button' '登 录' 5
if (-not $btnLogin) { Log 'FAIL: login button not found'; exit 1 }
Click-Button $btnLogin

$main = Wait-Window '科盟汽修综合数据管理' 15
if (-not $main) { Log 'FAIL: main window not found after login'; exit 1 }
Log "logged in, main window appeared."
Snap-Window $main '02-main'

# ---------- 3. open 业务报修 ----------
Log "=== 3. open 前台业务->业务报修 ==="
Activate-Window $main
# 键盘菜单导航（直接投递到主窗口，不依赖焦点）：F10=121 聚焦菜单栏 → Right=39 到"前台业务"
# → Enter=13 展开 → Enter=13 触发首项"业务报修"
$mainHwnd = [IntPtr]$main.Current.NativeWindowHandle
[Win32Kbd]::SetForegroundWindow($mainHwnd)
Start-Sleep -Milliseconds 200
SendKeysTo $mainHwnd @(121, 39, 13, 13)
# 若键盘导航未生效，回退到 ExpandCollapse 展开 + 真实点击
$fd = Wait-Window '业务报修' 6
if (-not $fd) {
    Log 'keyboard nav did not open dialog, trying ExpandCollapse+mouse fallback'
    $menuRepair = Wait-Ctl $main 'MenuItem' '前台业务' 10
    if ($menuRepair) {
        try { $menuRepair.GetCurrentPattern([System.Windows.Automation.ExpandCollapsePattern]::Pattern).Expand() } catch { Invoke-OrClick $menuRepair }
        Start-Sleep -Milliseconds 700
        $micond = New-Object System.Windows.Automation.AndCondition(
            (New-Object System.Windows.Automation.PropertyCondition(
                [System.Windows.Automation.AutomationElement]::ControlTypeProperty,
                [System.Windows.Automation.ControlType]::MenuItem)),
            (New-Object System.Windows.Automation.PropertyCondition(
                [System.Windows.Automation.AutomationElement]::NameProperty, '业务报修')))
        $actFront = $root.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $micond)
        if ($actFront) { Invoke-OrClick $actFront }
    }
    $fd = Wait-Window '业务报修' 10
}
if (-not $fd) { Log 'FAIL: 业务报修 dialog not found'; exit 1 }
Log "业务报修 dialog opened."
Activate-Window $fd
Snap-Window $fd '03-frontdesk'

# ---------- 4. lock vehicle 川B14250 ----------
Log "=== 4. lock 川B14250 ==="
$plate = Wait-Ctl $fd 'Edit' 'sPlate' 10
if (-not $plate) { Log 'FAIL: plate edit (sPlate) not found'; exit 1 }
Set-EditText $plate 'B14250'
[System.Windows.Forms.SendKeys]::SendWait('{DOWN}')
Start-Sleep -Milliseconds 150
[System.Windows.Forms.SendKeys]::SendWait('{ENTER}')
Start-Sleep -Milliseconds 600
Snap-Window $fd '04-locked-B14250'

$partSearch = Wait-Ctl $fd 'Edit' 'partSearch' 6
if (-not $partSearch) { Log 'FAIL: after lock, part search edit not visible (lock failed?)'; exit 1 }
Log "vehicle locked OK, part search visible."

# ---------- 5. add part 机 ----------
Log "=== 5. add part '机' ==="
Set-EditText $partSearch '机'
[System.Windows.Forms.SendKeys]::SendWait('{DOWN}')
Start-Sleep -Milliseconds 150
[System.Windows.Forms.SendKeys]::SendWait('{ENTER}')
Start-Sleep -Milliseconds 400
$btnAdd = Wait-Ctl $fd 'Button' '确定' 5
if (-not $btnAdd) { Log 'FAIL: 确定(添加备件) button not found'; exit 1 }
Click-Button $btnAdd
Start-Sleep -Milliseconds 500
Snap-Window $fd '05-part-added'

$table = Find-Table $fd
if ($table) {
    $rows1 = Get-TableRows $table
    Log "parts table rows after add = $rows1 (expect 1)"
    if ($rows1 -ne 1) { Log "WARN: expected 1 row after adding part" }
} else { Log 'FAIL: parts table control not found' }

# ---------- 6. cancel dispatch (回到查找) ----------
Log "=== 6. 取消派工 ==="
$btnCancel = Wait-Ctl $fd 'Button' '取消派工' 5
if (-not $btnCancel) { Log 'FAIL: 取消派工 button not found'; exit 1 }
Click-Button $btnCancel
Start-Sleep -Milliseconds 500
Snap-Window $fd '06-cancel-dispatch'

# ---------- 7. lock vehicle 111 ----------
Log "=== 7. lock 111 (修复验证点) ==="
$plate2 = Wait-Ctl $fd 'Edit' 'sPlate' 6
if (-not $plate2) { Log 'FAIL: plate edit not found after cancel'; exit 1 }
Set-EditText $plate2 '111'
[System.Windows.Forms.SendKeys]::SendWait('{DOWN}')
Start-Sleep -Milliseconds 150
[System.Windows.Forms.SendKeys]::SendWait('{ENTER}')
Start-Sleep -Milliseconds 600
Snap-Window $fd '07-locked-111'

$partSearch2 = Wait-Ctl $fd 'Edit' 'partSearch' 6
if (-not $partSearch2) { Log 'FAIL: after second lock, part search not visible'; exit 1 }
Log "second vehicle locked OK."

$table2 = Find-Table $fd
$rows2 = -1
if ($table2) { $rows2 = Get-TableRows $table2 }
Log "parts table rows after second lock = $rows2 (expect 0 => FIX OK)"

if ($rows2 -eq 0) { Log '=== SMOKE PASS: 锁定车辆后预计备件列表已清空 ===' }
else { Log '=== SMOKE FAIL: 预计备件列表未清空 ===' }
