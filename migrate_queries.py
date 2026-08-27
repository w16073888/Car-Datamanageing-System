# -*- coding: utf-8 -*-
"""机械迁移脚本：把 QSqlQuery/DbManager 直连模式转为 RemoteQuery（4s-client 专用）。
用法: python migrate_queries.py <file1> [file2 ...]

转换规则：
  QSqlQuery xxx(DbManager::instance().database());   ->  RemoteQuery xxx;
  QSqlQuery xxx(db);                                  ->  RemoteQuery xxx;
  QSqlDatabase &db = DbManager::instance().database();->  (删除)
  DbManager::instance().executeQuery(xxx);            ->  xxx.exec();
  DbManager::instance().lastError()                   ->  (保留, 由调用方改)
"""
import re
import sys


def migrate(path):
    with open(path, 'r', encoding='utf-8') as f:
        src = f.read()
    orig = src

    # 1) 删除 QSqlDatabase &db = DbManager::instance().database();（含可能的缩进与换行）
    src = re.sub(r'\n[ \t]*QSqlDatabase\s*&\s*\w+\s*=\s*DbManager::instance\(\)\.database\(\);', '\n', src)

    # 2) QSqlQuery xxx(DbManager::instance().database());  /  QSqlQuery xxx(db);  /  QSqlQuery xxx;
    src = re.sub(r'QSqlQuery\s+(\w+)\s*\(\s*DbManager::instance\(\)\.database\(\)\s*\);', r'RemoteQuery \1;', src)
    src = re.sub(r'QSqlQuery\s+(\w+)\s*\(\s*db\s*\);', r'RemoteQuery \1;', src)
    src = re.sub(r'QSqlQuery\s+(\w+)\s*\(\s*\)\s*;', r'RemoteQuery \1;', src)

    # 3) DbManager::instance().executeQuery(xxx)  ->  xxx.exec()（含 if (executeQuery(x) && ...) 场景）
    src = re.sub(r'DbManager::instance\(\)\.executeQuery\((\w+)\)', r'\1.exec()', src)

    # 4) DbManager::instance().lastError() 在 executeQuery 场景已替换, 剩下的 lastError 引用保留原样(多数改为 query.lastError())
    # 统计
    changed = src.count('RemoteQuery ') - orig.count('RemoteQuery ')
    exec_chg = src.count('.exec();') - orig.count('.exec();')
    with open(path, 'w', encoding='utf-8') as f:
        f.write(src)
    return changed, exec_chg


if __name__ == '__main__':
    for p in sys.argv[1:]:
        c1, c2 = migrate(p)
        print(f"{p}: RemoteQuery 替换 {c1}, exec() 替换 {c2}")
