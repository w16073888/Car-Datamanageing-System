-- 车主信息表
-- 运行此文件前需先创建/打开 cus.db 数据库连接
CREATE TABLE IF NOT EXISTS customers (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    owner_name TEXT NOT NULL,          -- 车主姓名
    owner_phone TEXT,                  -- 车主电话
    driver_name TEXT,                  -- 驾驶员姓名
    driver_phone TEXT                  -- 驾驶员电话
);
