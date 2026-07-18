-- ============================================================
-- ser.sql —— 进店服务信息表（services）
--
-- 功能：创建进店服务记录数据库表，用于存储车辆到店维修的
-- 服务信息，包括维修责任人、报修内容、行驶公里数、工时费、
-- 驾驶员信息以及结算状态。
-- 运行此文件可以生成数据库 ser.db，每行储存进店服务信息。
--
-- 字段说明：
--   id             : 工号（必填，自增主键/ID，系统自动生成）
--   repair_person  : 维修责任人（选填）
--   repair_content : 报修内容（选填）
--   mileage        : 记录车辆行驶公里数（选填）
--   labor_cost     : 维修工时费（选填，单位：元）
--   driver_name    : 驾驶员姓名（选填）
--   driver_phone   : 驾驶员电话（选填）
--   is_settled     : 是否结算（必填，0=已结算，1=未结算，默认0）
--   report_time    : 保修时间（必填，程序自动生成，格式 YYYY-MM-DD）
-- ============================================================

CREATE TABLE IF NOT EXISTS services (
    id INTEGER PRIMARY KEY AUTOINCREMENT,   -- 自增主键（系统自动生成）
    repair_person TEXT,                     -- 维修责任人（选填）
    repair_content TEXT,                    -- 报修内容（选填）
    mileage INTEGER,                       -- 记录车辆行驶公里数（选填）
    labor_cost REAL,                        -- 维修工时费（选填）
    driver_name TEXT,                       -- 驾驶员姓名（选填）
    driver_phone TEXT,                      -- 驾驶员电话（选填）
    is_settled INTEGER NOT NULL DEFAULT 0,  -- 是否结算（必填，0=已结算，1=未结算）
    report_time TEXT NOT NULL               -- 报修时间（必填）
);
