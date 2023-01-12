:: Данный батник качает SDL нужной версии в папку repo

:: Меняем кодировку консоли на UTF-8
chcp 65001

:: Путь к git.exe
set "PATH=c:\program files\git\bin"

:: Качаем SDL_gesture в папку gesture_repo
git clone https://github.com/libsdl-org/SDL_gesture gesture_repo

:: Возвращаем состояние репозитория к определённой версии
git -C repo reset --hard ce6e87b34f80340ec13f39bd4782e43e8fb50144

:: Ждём нажатие Enter перед закрытием консоли
pause
