-- ============================================================
-- ware.sql —— 备件信息表（parts）
--
-- 功能：创建备件（配件/零件）库存数据库表，用于存储汽修
-- 备件的编号、名称、库存数量、进货与销售价格、供货商、
-- 出库日期、质保期等信息。
-- 运行此文件前需先创建/打开 ware.db 数据库连接。
--
-- 字段说明：
--   part_id       : 备件编号（主键）
--   name          : 名称（必填）
--   quantity      : 数量（必填，默认 0）
--   purchase_price: 进货价（必填，单位：元）
--   sale_price    : 销售价（默认 0，程序计算建议 = 进货价 * 1.4）
--   supplier      : 供货商（必填）
--   out_date      : 出库日期（必填，格式建议 YYYY-MM-DD）
--   warranty_period: 质保期（选填，如"12个月""2年"）
--
-- 关于 sale_price（销售价）：
--   如不填写，C++ 代码层面默认按"进货价 × 1.4"计算
--   （即 0.4 利润），SQL 层不做自动计算。
-- ============================================================

CREATE TABLE IF NOT EXISTS parts (
    part_id TEXT PRIMARY KEY,                -- 备件编号
    name TEXT NOT NULL,                      -- 名称（必填）
    quantity INTEGER NOT NULL DEFAULT 0,     -- 数量（必填，默认为 0）
    purchase_price REAL NOT NULL,            -- 进货价（必填）
    sale_price REAL DEFAULT 0,               -- 销售价（默认 0，C++ 层计算 0.4 利润）
    supplier TEXT NOT NULL,                  -- 供货商（必填）
    out_date TEXT NOT NULL,                  -- 出库日期（必填）
    warranty_period TEXT                     -- 质保期（选填）
);
