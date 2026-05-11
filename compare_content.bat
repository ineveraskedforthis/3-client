cd flags
fc.exe latest_content_id local_content_id > NUL
if %errorlevel% EQU 0 (
                                echo "GAME CONTENT IS UP TO DATE"
) else (
                                (echo "REQUEST GAME CONTENT UPDATE") && (@copy latest_content_id local_content_id)
)
cd ..