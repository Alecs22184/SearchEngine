@echo off
echo ===== FINAL SYSTEM STATISTICS =====
echo.

echo Checking database connection and statistics...
echo.

psql -h localhost -p 5432 -U postgres -d search_engine -c "SELECT '=== SYSTEM OVERVIEW ===' as info; SELECT 'Documents: ' || COUNT(*) FROM documents; SELECT 'Unique words: ' || COUNT(*) FROM words; SELECT 'Word-document relationships: ' || COUNT(*) FROM document_words;"

psql -h localhost -p 5432 -U postgres -d search_engine -c "SELECT '=== DOCUMENT SIZES ===' as info; SELECT 'Small (<10K): ' || COUNT(*) FROM documents WHERE LENGTH(content) < 10000 UNION ALL SELECT 'Medium (10K-50K): ' || COUNT(*) FROM documents WHERE LENGTH(content) BETWEEN 10000 AND 50000 UNION ALL SELECT 'Large (>50K): ' || COUNT(*) FROM documents WHERE LENGTH(content) > 50000;"

psql -h localhost -p 5432 -U postgres -d search_engine -c "SELECT '=== TOP 15 MOST FREQUENT WORDS ===' as info; SELECT w.word, SUM(dw.frequency) as total_occurrences, COUNT(DISTINCT dw.document_id) as documents_count FROM words w JOIN document_words dw ON w.id = dw.word_id GROUP BY w.word ORDER BY total_occurrences DESC LIMIT 15;"

psql -h localhost -p 5432 -U postgres -d search_engine -c "SELECT '=== SEARCH PERFORMANCE ===' as info; SELECT 'Total words in database: ' || SUM(frequency) FROM document_words;"

echo.
echo ===== SEARCH ENGINE STATUS: OPERATIONAL =====
pause