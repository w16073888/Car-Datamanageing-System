-- 进店服务信息表
-- 运行此文件前需先创建/打开 ser.db 数据库连接
CREATE TABLE IF NOT EXISTS services (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    repair_person TEXT NOT NULL,       -- 维修责任人
    repair_content TEXT,               -- 报修内容
    mileage INTEGER,                   -- 记录车辆行驶公里数
    labor_cost REAL                    -- 维修工时费
);
