PRAGMA foreign_keys = OFF;
DELETE FROM File;
DELETE FROM Song;
DELETE FROM Category;

DELETE FROM sqlite_sequence;
PRAGMA foreign_keys = ON;
