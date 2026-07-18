-- ============================================================
-- cus.sql —— 车主信息表（customers）
--
-- 功能：创建车主信息数据库表，用于存储车主的基本联系信息。
-- 运行此文件前需先创建/打开 cus.db 数据库连接。
--
-- 字段说明：
--   id          : 自增主键（自动编号）
--   owner_name  : 车主姓名（选填）
--   owner_phone : 车主电话（选填）
-- ============================================================

CREATE TABLE IF NOT EXISTS customers (
    id INTEGER PRIMARY KEY AUTOINCREMENT,  -- 自增主键（系统自动生成）
    owner_name TEXT,                       -- 车主姓名（选填）
    owner_phone TEXT                       -- 车主电话（选填）
);
