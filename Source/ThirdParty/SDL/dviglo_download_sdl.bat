:: Данный батник качает SDL нужной версии в папку repo

:: Меняем кодировку консоли на UTF-8
chcp 65001

:: Путь к git.exe
set "PATH=c:\program files\git\bin"

:: Качаем SDL 3 в папку repo
git clone https://github.com/libsdl-org/SDL repo

:: Возвращаем состояние репозитория к определённой версии
git -C repo reset --hard 050507c333b0dfecb3e08604ec7ff12d5c410935

:: Ждём нажатие Enter перед закрытием консоли
pause
