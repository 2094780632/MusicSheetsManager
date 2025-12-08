-- 外键约束
PRAGMA foreign_keys = ON;

-- 表的创建
CREATE TABLE IF NOT EXISTS Category(
    c_id INTEGER PRIMARY KEY AUTOINCREMENT,
    c_name TEXT,
    c_remark TEXT
);

CREATE TABLE IF NOT EXISTS Song(
    s_id INTEGER PRIMARY KEY AUTOINCREMENT, --id
    s_name TEXT, --歌名
    s_composer TEXT, --作者
    s_key TEXT, --调性
    s_addDate TEXT, --yyyy-MM--dd
    s_isFav BOOLEAN,
    s_remark TEXT, --备注
    c_id INTEGER, --外键 分类
    FOREIGN KEY (c_id) REFERENCES Category(c_id)
);
