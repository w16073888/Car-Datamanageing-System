-- ============================================================
-- car.sql —— 车辆信息表（vehicles）
--
-- 功能：创建车辆信息数据库表，用于存储车辆的基本信息。
-- 运行此文件可以生成数据库 car.db，每行储存车辆信息。
--
-- 字段说明：
--   customer_id    : 车主号（必填，关联 cus.db 中的车主 ID）
--   license_plate  : 车牌号（必填，主键/ID）
--   vin            : 车架号（选填）
--   engine_number  : 发动机号（选填）
--   purchase_date  : 购车日期（选填，格式建议 YYYY-MM-DD）
--   inspection_date: 年审日期（选填，格式建议 YYYY-MM-DD）
--   insurance_date : 保险日期（选填，格式建议 YYYY-MM-DD）
-- ============================================================

CREATE TABLE IF NOT EXISTS vehicles (
    customer_id INTEGER NOT NULL,            -- 车主号（必填，关联 cus.db 中的车主 ID）
    license_plate TEXT PRIMARY KEY,   -- 车牌号（必填，唯一标识一辆车）
    vin TEXT,                         -- 车架号（选填，车辆识别码 VIN）
    engine_number TEXT,               -- 发动机号（选填）
    purchase_date TEXT,               -- 购车日期（选填）
    inspection_date TEXT,             -- 年审日期（选填）
    insurance_date TEXT               -- 保险日期（选填）
);
