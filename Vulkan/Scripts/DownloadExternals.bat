if not exist "7zr.exe" (
    curl -L "https://www.dropbox.com/scl/fi/kk1rgbe1uk0ypup11kei3/7zr.exe?rlkey=z3mnmn0ybyfepl300oebuknsb&st=m9esl85c&dl=1" -O "7zr.exe" >nul
)
if not exist "7zr.exe" (
    exit
)

if not exist "Externals.zip" (
    curl -L "https://www.dropbox.com/scl/fi/fg8k72tmf8ix3ewjo2jvc/Externals.zip?rlkey=lasjpvi5x9e0phekbxalb664h&st=bizpbs9z&dl=1" -O "Externals.zip" >nul
)
if not exist "Externals.zip" (
    exit
)

if not exist "Dragon.zip" (
    curl -L "https://www.dropbox.com/scl/fi/1de0ooc5frruamcfrwwhe/Dragon.zip?rlkey=3nxwak9xidbs8iagefcp1awt5&st=tttjma1f&dl=1" -O "Dragon.zip" >nul
)
if not exist "Dragon.zip" (
    exit
)

7zr.exe x "Externals.zip" -o"../" -y
7zr.exe x "Dragon.zip" -o"../Resources/Meshes/" -y
del "Dragon.zip"
del "Externals.zip"
del "7zr.exe"
