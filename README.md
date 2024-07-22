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
 char_driver</br>
 |</br>
 |-- Makefile</br>
 |</br>
 |-- chr_driver.c</br>
 |</br>
 |-- README.md</br>
 |</br>
 |-- samples</br>
     |</br>
     |-- sample_changing_io_mode</br>
         |</br>
         |-- test_read.c</br>
         |</br>
         |-- test_write.c</br>
     |</br>
     |-- sample_last_op_info</br>
         |</br>
         |-- test_read.c</br>
         |</br>
         |-- test_write.c</br>

 - `chr_driver.c` - файл с исходным кодом драйвера.
 - `Makefile` - файл, содержащий инструкции по сборке драйвера.
 - `README.md` - файл с документацией проекта.
 - `samples` - папка с примерами использования символьного устройства.
 - `sample_changing_io_mode` - папка с примером, демонстрирующим смену блокировки чтения и записи.
 - `sample_last_op_info` - папка с примером, демонстрирующим получение информации о последних операция чтения и записи.
 - `test_read.c` - файл с кодом процесса-читателя.
 - `test_write.c` - файл с кодом процесса-писателя.

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
dmesg | tail 
sudo mknod /dev/my_char_device c <major number> <minor number>
sudo chmod 666 /dev/my_char_device
```
Команда dmesg | tail необходима для просмотра логов с номерами устройства.

## Пример использования
Для демонстрации возможностей разработанного символьного устройства в проекте приведены 2 примера в папке `samples`.
Для запуска желаемого примера необходимо:
 1. Перейти в папку с примером, например с примером на получение информации о последних операциях чтения и записи:
```bush
cd sample_last_op_info
```
 2. Скомпилировать файлы читателя и писателя:
```bush
gcc -o reader test_read.c
gcc -o writer test_write.c
```
 3. Запустить в разных терминалах запустите получившиеся исполняемые файлы `reader` и `writer`
```bush
./writer
```
```bush
./reader
```

В результате в терминалах должен быть похожий вывод:
```bush
Read 18 bytes: Hello from writer!
Last read time: 4305450340
Last write time: 4305450340
Last read op process id: 33066
Last write op process id: 33067
Last read op user id: 1001
Last write op user id: 1001
```
```bush
Written 18 bytes to /dev/my_char_device
Last read time: 4305450340
Last write time: 4305450340
Last read op process id: 33066
Last write op process id: 33067
Last read op user id: 1001
Last write op user id: 1001
```

## Отчистка (удаление модуля и устройства)
```bash
sudo rmmod chr_driver
sudo rm /dev/my_char_device
```