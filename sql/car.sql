-- 车辆信息表
-- 运行此文件前需先创建/打开 car.db 数据库连接
CREATE TABLE IF NOT EXISTS vehicles (
    license_plate TEXT PRIMARY KEY,   -- 车牌号
    vin TEXT NOT NULL,                -- 车架号
    engine_number TEXT NOT NULL,      -- 发动机号
    purchase_date TEXT,               -- 购车日期
    inspection_date TEXT,             -- 年审日期
    insurance_date TEXT               -- 保险日期
);
