-- ============================================================
-- 密码明文化迁移脚本
-- 说明：系统已改为明文存储密码，此脚本将数据库中
--       已有的SHA256哈希密码转换为对应的明文。
--       已知密码直接还原，未知哈希重置为默认密码"123456"
-- 用法: USE garagedb; SOURCE migration_plaintext_password.sql;
-- ============================================================

-- 1. 更新默认管理员（ADMIN001）密码: admin123 → admin123（哈希转明文）
--    SHA256("admin123") = 240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9
UPDATE t_employee
SET password = 'admin123'
WHERE password = '240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9';

-- 2. 将所有其他SHA256格式（64位十六进制）的密码重置为"123456"
--    这样在界面上就能正常显示明文密码，而不是乱码哈希
UPDATE t_employee
SET password = '123456'
WHERE LENGTH(password) = 64
  AND password REGEXP '^[0-9a-fA-F]{64}$'
  AND password != 'admin123';  -- 避免误伤上面已更新的记录

-- 3. 输出更新结果
SELECT CONCAT('已转换 ', ROW_COUNT(), ' 条密码记录为明文') AS 迁移结果;

-- 4. 显示更新后的员工表密码列（确认密码已是明文）
SELECT id, employee_id, name, password, position, phone
FROM t_employee
ORDER BY id;
