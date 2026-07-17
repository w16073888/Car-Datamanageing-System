-- 备件信息表
-- 运行此文件前需先创建/打开 ware.db 数据库连接
CREATE TABLE IF NOT EXISTS parts (
    part_id TEXT PRIMARY KEY,          -- 备件编号
    name TEXT NOT NULL,                -- 名称
    quantity INTEGER DEFAULT 0,        -- 数量
    price REAL,                        -- 金额
    supplier TEXT,                     -- 供货商
    warranty_period TEXT               -- 质保期
);
