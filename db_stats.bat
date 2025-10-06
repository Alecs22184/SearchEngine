@echo off
echo ===== Search Engine Database Statistics =====
echo.

psql -h localhost -p 5432 -U postgres -d search_engine -c "
SELECT '=== Documents ===' as info;
SELECT COUNT(*) as total_documents FROM documents;
SELECT '=== Words ===' as info;
SELECT COUNT(*) as total_words FROM words;
SELECT '=== Document-Word Relationships ===' as info;
SELECT COUNT(*) as total_relationships FROM document_words;

SELECT '' as info;
SELECT '=== Top 10 Most Frequent Words ===' as info;
SELECT w.word, SUM(dw.frequency) as frequency
FROM words w
JOIN document_words dw ON w.id = dw.word_id
GROUP BY w.word
ORDER BY frequency DESC
LIMIT 10;

SELECT '' as info;
SELECT '=== Recent Documents ===' as info;
SELECT id, LEFT(url, 50) as url, LENGTH(content) as content_length 
FROM documents 
ORDER BY id DESC 
LIMIT 5;
"

pause