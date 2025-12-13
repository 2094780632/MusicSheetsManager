-- 外键约束
PRAGMA foreign_keys = ON;

-- 表的创建
CREATE TABLE IF NOT EXISTS Category(
    c_id INTEGER PRIMARY KEY AUTOINCREMENT,
    c_name TEXT NOT NULL UNIQUE,
    c_remark TEXT
);

CREATE TABLE IF NOT EXISTS Song(
    s_id INTEGER PRIMARY KEY AUTOINCREMENT, --id
    s_name TEXT NOT NULL, --歌名
    s_composer TEXT, --作者
    s_key TEXT, --调性
    s_addDate TEXT NOT NULL DEFAULT (DATE('now')), --yyyy-MM--dd
    s_isFav BOOLEAN NOT NULL DEFAULT 0,
    s_remark TEXT, --备注
    s_version TEXT, --版本
    s_type TEXT, --导入类型
    c_id INTEGER, --外键 分类
    FOREIGN KEY (c_id) REFERENCES Category(c_id)
    ON UPDATE CASCADE
    ON DELETE SET NULL -- 分类被删时歌曲保留，但c_id置空
);

CREATE TABLE IF NOT EXISTS File(
    f_id INTEGER PRIMARY KEY AUTOINCREMENT, --id
    f_index INTEGER, --索引
    f_path TEXT NOT NULL, --目录
    f_type TEXT, --类型
    s_id INTEGER NOT NULL, --外键，所属歌曲
    FOREIGN KEY (s_id) REFERENCES Song(s_id)
    ON UPDATE CASCADE
    ON DELETE CASCADE
);
