cmake_minimum_required(VERSION 3.20)
project(ProcSight)

# Стандарт C++17 необходим для работы с <filesystem>
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Поиск ncurses.
# На Arch Linux библиотека разработки поставляется вместе с основным пакетом ncurses.
find_package(Curses REQUIRED)

# 2. Определяем исполняемый файл и все связанные ресурсы
add_executable(procsight
        main.cpp
        ProcessList.cpp
        SystemStats.cpp
)

# 3. Подключаем заголовочные файлы
# Использование PRIVATE ограничивает область видимости только этим таргетом
target_include_directories(procsight PRIVATE ${CURSES_INCLUDE_DIRS})

# 4. Линковка библиотек
# В Arch Linux крайне рекомендуется явно линковать ncursesw для поддержки UTF-8
target_link_libraries(procsight PRIVATE ${CURSES_LIBRARIES} ncursesw)

# Если ты планируешь использовать расширенные функции форм (меню, поля ввода)
# можно добавить библиотеку form:
# target_link_libraries(procsight PRIVATE ${CURSES_LIBRARIES} ncursesw form)