# Запуск KLEE на TestComp тестах
## 0. Все необходимое.
* [benchexec](https://github.com/sosy-lab/benchexec)
* [sv-benchmarks](https://github.com/sosy-lab/sv-benchmarks)
* [testcov](https://gitlab.com/sosy-lab/software/test-suite-validator)

`benchexec` имеет репозиторий для Ubuntu с упрощенной настройкой, на остальных
дистрибутивах скорее всего придется пересобрать ядро как минимум.

## 1. Запуск `benchexec` на `KLEE`.
В репозитории имеется для этого два файла:
  * `/klee` - оболочка над `KLEE` для `benchexec`.
  * `/benchmark.xml` - спецификация необходимых тестов из `sv-benchmarks`.
`benchmark.xml`, в текущей конфигурации, должна лежать в `sv_benchmarks/c/`.
Запуск `benchexec` производится следующей командой в директории `sv_benchmarks/c/`: `PATH=../../states_and_lemmas:$PATH benchexec benchmark.xml`, где `../../states_and_lemmas` - путь до нашего репозитория (в `states_and_lemmas/build` подразумевается наличие собранного `KLEE`).
После этого в `sv_benchmarks/c/` появится директория `results`, где будут лежать результаты.

## 2. Запуск `testcov` на результатах `benchexec`.
В репозитории для этого лежит скрипт `testcov_run.py`. Нужно зайти в `results`, дойти до директорий с именами вида `*.yml`,
заархивировать их каждый в архивы с именами `.yml.zip`, после чего запустить `testcov_run.py` с параметрами, описание которых
можно получить с помощью `testcov_run.py -h`.

