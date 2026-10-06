-- 015: 车主电话加长（支持一次录入 1~3 个号码）
-- t_vehicle.owner_phone 原为 VARCHAR(20)，录入两个手机号
-- （如 13800138000/13900139000 共 23 字符）即因超长导致保存失败；
-- 现支持一次录入 1~3 个号码（含分隔符约 40 字符），加宽到 VARCHAR(100)。
ALTER TABLE t_vehicle MODIFY COLUMN owner_phone VARCHAR(100) COMMENT '车主电话（可含1~3个号码）';
