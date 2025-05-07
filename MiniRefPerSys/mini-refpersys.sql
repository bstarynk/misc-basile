PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE IF NOT EXISTS "generated_c_code_tb" (
	"c_basename_fl"	TEXT NOT NULL,
	"c_rank_fl"	INTEGER NOT NULL,
	PRIMARY KEY("c_rank_fl" AUTOINCREMENT)
);
DELETE FROM sqlite_sequence;
COMMIT;
