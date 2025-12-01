# Задача 3. Спинлоки

Реализовать TAS, TTAS & ticket lock с оптимизациями (pause, yield, backoff), построить графики -- среднее и максимальное время вхождения в крит. секцию от числа потоков. Обязательно корректная работа с барьерами памяти при обращении к атомарным переменным.

## Вопросы:
- барьеры памяти
- отложенный эффект операций
- синхронизация кэшей
- store buffer
- invalidate queue
- модели памяти C / C++
- свойства операции compare_exchange
- техники оптимизации
- честность
- чем ttas лучше tas
- в чём разница между спинлоком и мьютексом
- как можно сделать мьютекс

> Желающие получить балл побольше реализуют MCS lock / CLH lock.

## Графики
![all_implementations_comparison](resources/all_implementations_comparison.png "all_implementations_comparison")

![avg_time_all_implementations](resources/avg_time_all_implementations.png "avg_time_all_implementations")

![best_two_avg_time](resources/best_two_avg_time.png "best_two_avg_time")

![best_two_max_time](resources/best_two_max_time.png "best_two_max_time")

![best_two_side_by_side](resources/best_two_side_by_side.png "best_two_side_by_side")

![max_time_all_implementations](resources/max_time_all_implementations.png "max_time_all_implementations")
