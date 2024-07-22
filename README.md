# Символьный драйвер для обмена данными между процессами на Linux

## Описание проекта
Этот проект представляет собой реализацию драйвера символьного устройства, 
который предлагает возможность обмениваться информацией процессам через файл устрайства.
Реализован обмен с помощью кольцевого буфера.
Пользователю предоставлены следующие операции работы с устройством:
 - `open`
 - `read`
 - `write`
 - `ioctl`

## Основные особености
 1. Размер кольцевого буфера может быть задан через параметры модуля.
 2. Операции чтения и запичи могут быть блокирующими и неблокирующими.
 3. Изначально операции чтения и записи блокирующие.
 4. Драйвер предоставляет пользователю интерфес изменения типа операций чтения и записи с неблокирующего на блокирующий и наоборот через вызов ioctl.
 5. Драйвер предоставляет возможность получить информацию о последней операции записи и чтения через вызов ioctl.

## Структура проекта
 char_driver
 |
 |-- Makefile
 |
 |-- chr_driver.c
 |
 |-- README.md

 - `chr_driver.c` - файл с исходным кодом драйвера.
 - `Makefile` - файл, содержащий инструкции по сборке драйвера.
 - `README.md` - файл с документацией проекта.

## Сборка проекта
 1. Склонируйте репозиторий https://github.com/suvorovDm-1/char_driver:
```bash
git clone https://github.com/suvorovDm-1/char_driver.git
```
 2. Компиляция
```bash
cd char_driver
make
sudo insmod chr_driver.ko
dmesg | tail (чтобы увидеть логи с номерами устройства)
sudo mknod /dev/my_char_driver c <major number> <minor number>
```

## Отчистка
```bash
sudo rmmod chr_driver
sudo rm /dev/my_char_driver
```