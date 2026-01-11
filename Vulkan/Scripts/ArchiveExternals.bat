if not exist "7zr.exe" (
    curl -L "https://www.dropbox.com/scl/fi/kk1rgbe1uk0ypup11kei3/7zr.exe?rlkey=z3mnmn0ybyfepl300oebuknsb&st=m9esl85c&dl=1" -O "7zr.exe" >nul
)

7zr.exe a -mx=9 -r "Externals.zip" "../Externals"
7zr.exe a -mx=9 -r "Dragon.zip" "../Resources/Meshes/Dragon.obj"

del "7zr.exe"
