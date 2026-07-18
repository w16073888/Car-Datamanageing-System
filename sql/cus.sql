-- ============================================================
-- cus.sql —— 车主信息表（customers）
--
-- 功能：创建车主信息数据库表，用于存储车主的基本联系信息。
-- 运行此文件可以生成数据库 cus.db，每行储存车主信息。
--
-- 字段说明：
--   id          : 车主号（必填，自增主键/ID，系统自动生成）
--   owner_name  : 车主姓名（选填）
--   owner_phone : 车主电话（选填）
-- ============================================================

CREATE TABLE IF NOT EXISTS customers (
    id INTEGER PRIMARY KEY AUTOINCREMENT,  -- 自增主键（系统自动生成）
    owner_name TEXT,                       -- 车主姓名（选填）
    owner_phone TEXT                       -- 车主电话（选填）
);
