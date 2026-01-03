PRAGMA foreign_keys = OFF;      --关闭外键约束
DELETE FROM File;               --删除 文件
DELETE FROM Song;               --删除歌曲
DELETE FROM Category;           --删除 分类

DELETE FROM sqlite_sequence;    --删除 SQLite记录（下一次添加时AUTOINCREMENT的字段才会从0开始）
PRAGMA foreign_keys = ON;       --重新打开外键约束
