# Туториал: свой прибор на QWidgets с нуля

Этот туториал повторяет путь, которым в проекте сделаны высотомер и спидометр. Мы соберём маленькое приложение из трёх файлов: виджет `SpeedGauge` (шкала 0–60, стрелка, цифровые метки, плавный ход) и `main.cpp`, который крутит стрелку синусоидой. Никаких .ui-файлов, никаких QML — только `QPainter` и тригонометрия.

Код самодостаточный: скопируй три файла + `CMakeLists.txt` в отдельную папку и собери.

---

## 0. Как вообще работает отрисовка в Qt Widgets

Три факта, из которых следует всё остальное:

1. **Виджет рисует себя сам в `paintEvent()`.** Система вызывает этот метод, когда виджету нужно перерисоваться. Тебе никогда не нужно вызывать его напрямую.
2. **Просить о перерисовке — `update()`.** Он ставит событие в очередь и схлопывает несколько запросов в один кадр. (`repaint()` — немедленная синхронная перерисовка, почти всегда не то, что нужно.)
3. **`QPainter` можно создавать только внутри `paintEvent()`** (на стеке, как в примерах ниже). Вне его QPainter на виджете — неопределённое поведение.

И одна главная ловушка: **ось Y направлена ВНИЗ**. Точка (0, −50) — выше центра. Отсюда все «странные» знаки в формулах ниже.

## 1. Трюк с виртуальными координатами

Прибор хочется рисовать в «приборных» единицах: центр (0,0), радиус 100, деление на радиусе 85. А виджет бывает 200×200, 340×280, любым. Вместо ручного пересчёта используем пару `setViewport` / `setWindow`:

```cpp
const int side = qMin(width(), height());          // прибор круглый — берём квадрат
p.setViewport((width() - side) / 2, (height() - side) / 2, side, side);
p.setWindow(-100, -100, 200, 200);
```

`setWindow` говорит: «логическая система координат — квадрат от (−100,−100) до (100,100)». Всё, что нарисовано дальше, автоматически масштабируется под физические пиксели. Хочешь радиус 85 — пиши 85. Хочешь текст на «высоте» −70 — пиши −70.

Это ровно то, что делает `BaseGaugeWidget::paintEvent` в `receiver/MainWindow.cpp` — все четыре прибора проекта наследуют этот механизм.

## 2. Каркас проекта

**CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.20)
project(GaugeTutorial LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)   # обязательно: SpeedGauge содержит Q_OBJECT

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(tutorial main.cpp SpeedGauge.h SpeedGauge.cpp)
target_link_libraries(tutorial PRIVATE Qt6::Widgets)
```

**SpeedGauge.h**

```cpp
#ifndef SPEEDGAUGE_H
#define SPEEDGAUGE_H

#include <QElapsedTimer>
#include <QWidget>

class SpeedGauge : public QWidget
{
    Q_OBJECT

public:
    explicit SpeedGauge(QWidget *parent = nullptr);

    // Публичный «интерфейс данных» прибора
    void setSpeed(float speed);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    float m_target = 0.0f;      // куда стрелка идёт
    float m_shown = 0.0f;       // что нарисовано прямо сейчас
    QElapsedTimer m_frameTimer; // время между кадрами (для плавного хода)
};

#endif
```

Обрати внимание на два поля вместо одного: `m_target` — цель, `m_shown` — текущее отображение. Разница между ними и даст плавный ход стрелки на шаге 6.

**SpeedGauge.cpp — каркас**

```cpp
#include "SpeedGauge.h"
#include <QPainter>
#include <QPen>
#include <cmath>

SpeedGauge::SpeedGauge(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(200, 200);              // без этого прибор можно ужать до нуля
    m_frameTimer.start();
}

void SpeedGauge::setSpeed(float speed)
{
    m_target = speed;
    update();   // НЕ repaint()! именно update()
}

void SpeedGauge::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);   // иначе круги и линии «лесенкой»
}
```

Собери уже сейчас — должно компилироваться и показывать пустое окно. (Да, `qMin`, `std::exp` и прочее подключим по ходу.)

## 3. Рисуем подложку и подпись

Всё рисование дальше — внутри `paintEvent`, после настройки координат из шага 1:

```cpp
// Тёмный круг-подложка
p.setPen(QPen(QColor(0x3A, 0x40, 0x4A), 2));   // цвет ободка + толщина 2
p.setBrush(QColor(0x0E, 0x0F, 0x12));          // заливка
p.drawEllipse(-100, -100, 200, 200);           // в виртуальных координатах!

// Подпись прибора
p.setPen(QColor(0xE8, 0xEA, 0xED));
p.setFont(QFont("monospace", 9));
p.drawText(-30, -70, "SPD");
```

`drawEllipse(x, y, w, h)` принимает прямоугольник, в который вписан эллипс. Наш «прямоугольник» — от (−100,−100) размером 200×200, то есть ровно весь циферблат.

В проекте эти цвета не хардкодятся, а берутся из семантической палитры `Theme.h` (`Theme::gaugeFace`, `Theme::outline`, `Theme::textPrimary`). В туториале упрощаем, но привыкай мыслить ролями: «цвет подложки», «цвет текста», «цвет тревоги». То же со шрифтом: здесь для простоты `QFont("monospace", 9)` (работает в Linux через fontconfig), а в проекте — переносимый системный моноширинный шрифт через `QFontDatabase::systemFont(QFontDatabase::FixedFont)`, см. `Theme::fixedFont`.

## 4. Деления шкалы: угол → точка

Шкала — 13 делений от 0 до 60 м/с (шаг 5). Значение переводится в угол так:

```
угол = старт + (значение / максимум) * размах
```

Для спидометра: старт −120°, размах 240°. Почему так: при оси Y, направленной вниз, угол 0° смотрит вправо, −90° — вверх, +90° — вниз. Диапазон −120°..+120° — это дуга, открытая слева (как у настоящего спидометра).

Точка на окружности радиуса r под углом a:

```
x = r * cos(a)      y = r * sin(a)      // a в радианах!
```

Код:

```cpp
constexpr float kMaxSpeed = 60.0f;
constexpr int kTickCount = 12;      // 12 делений = 13 штрихов, включая 0
constexpr float kStartDeg = -120.0f;
constexpr float kSweepDeg = 240.0f;

for (int i = 0; i <= kTickCount; ++i) {
    const float val = i * (kMaxSpeed / kTickCount);            // 0, 5, 10 ... 60
    const float deg = kStartDeg + (val / kMaxSpeed) * kSweepDeg;
    const float rad = deg * float(M_PI) / 180.0f;              // градусы -> радианы

    const bool major = (i % 3 == 0);                           // каждый третий — длинный
    const int rInner = major ? 75 : 80;

    p.setPen(QColor(0xE8, 0xEA, 0xED));
    p.drawLine(static_cast<int>(85 * std::cos(rad)),           // внешний конец штриха
               static_cast<int>(85 * std::sin(rad)),
               static_cast<int>(rInner * std::cos(rad)),       // внутренний конец
               static_cast<int>(rInner * std::sin(rad)));
}
```

Три вещи, которые здесь легко испортить:

- забыть перевести градусы в радианы (`std::cos` принимает радианы);
- написать `85 * cos(rad) / 100 * 100` с целыми — считай всё во `float`, каст в `int` только на границе;
- попытаться «помнить» прошлый угол вместо пересчёта — в `paintEvent` состояние не хранится между кадрами, всё пересчитывается с нуля. Это контракт: **отрисовка — чистая функция от состояния**.

## 5. Цифровые метки и стрелка

Метки — на мажорных делениях, радиусом меньше (63), по центру своего прямоугольника:

```cpp
if (major) {
    const int lx = static_cast<int>(63 * std::cos(rad));
    const int ly = static_cast<int>(63 * std::sin(rad));
    p.setPen(QColor(0x9A, 0xA0, 0xA8));                        // приглушённый цвет подписей
    p.setFont(QFont("monospace", 7));
    p.drawText(QRect(lx - 12, ly - 7, 24, 14), Qt::AlignCenter, QString::number(int(val)));
}
```

`drawText(QRect, AlignCenter, ...)` центрирует текст в точке — гораздо надёжнее, чем подгонять координаты вручную.

Стрелка — просто линия из центра (значение берём из `m_shown`, см. следующий шаг):

```cpp
const float clamped = std::clamp(m_shown, 0.0f, kMaxSpeed);   // за шкалу не выходить
const float needleDeg = kStartDeg + (clamped / kMaxSpeed) * kSweepDeg;
const float needleRad = needleDeg * float(M_PI) / 180.0f;

p.setPen(QPen(QColor(0xFF, 0x5A, 0x5A), 3));                  // приборный красный, толщина 3
p.drawLine(0, 0,
           static_cast<int>(70 * std::cos(needleRad)),
           static_cast<int>(70 * std::sin(needleRad)));

// Цифровое значение под центром
p.setPen(QColor(0xE8, 0xEA, 0xED));
p.setFont(QFont("monospace", 11, QFont::Bold));
p.drawText(-25, 10, QString::number(int(m_shown)) + "m/s");
```

Запусти — прибор уже работает. Но стрелка дёргается: каждые новые данные — прыжок. Это шаг 6.

## 6. Плавный ход стрелки (демпфирование)

Идея: стрелка не прыгает к цели, а приближается к ней экспоненциально. За время `dt` она проходит долю `k` оставшегося пути:

```cpp
k = 1 - exp(-dt / tau)
```

`tau` (тау) — «постоянная времени»: за столько миллисекунд стрелка проходит ~63 % пути. В проекте tau = 0.15 с — движение выглядит «приборно», не успевает надоесть.

В начало `paintEvent`, после настройки координат:

```cpp
#include <algorithm>   // std::clamp — в SpeedGauge.cpp

// Время с прошлого кадра; clamp страхует от гигантского скачка после паузы
const float dt = std::clamp(m_frameTimer.restart() / 1000.0f, 0.0f, 0.1f);
const float k = 1.0f - std::exp(-dt / 0.15f);
m_shown += (m_target - m_shown) * k;
```

И в конец `paintEvent` — просьба о следующем кадре, пока стрелка не дошла (иначе после последнего `update()` от `setSpeed` движение остановится на полпути):

```cpp
if (std::abs(m_target - m_shown) > 0.05f)
    update();
```

Это простейший вариант. В проекте (`BaseGaugeWidget`) добавлены троттлинг ~60 кадров/с (чтобы не жечь CPU на пустом десктопе) и вариант `stepTowardsAngle` для компаса, где 359° → 1° обязаны идти через 0, а не через весь круг назад.

## 7. Оживляем: main.cpp

```cpp
#include "SpeedGauge.h"
#include <QApplication>
#include <QTimer>
#include <cmath>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    SpeedGauge gauge;
    gauge.resize(320, 320);
    gauge.show();

    // Синтетические данные, как у sender: синусоида 30 ± 25
    float t = 0.0f;
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&] {
        gauge.setSpeed(30.0f + 25.0f * std::sin(t));
        t += 0.1f;
    });
    timer.start(50);   // 20 Гц — как реальный поток телеметрии

    return app.exec();
}
```

Сборка и запуск:

```sh
cmake -B build -G Ninja && cmake --build build && ./build/tutorial
```

Потяни окно за угол — прибор масштабируется, пропорции сохраняются. Это бесплатный бонус от шага 1.

## 8. Упражнения (по возрастанию)

1. **Смени шкалу** на 0–300 м (высотомер). Что изменится, а что нет? Заодно вынеси `kMaxValue/kStartDeg/kSweepDeg/kTickCount` в структуру `DialSpec`, как сделано в проекте, — и убедись, что высотомер и спидометр стали одним кодом.
2. **Вариометр** — прибор вертикальной скорости: вертикальная шкала −10..+10 м/с, стрелка ходит вверх-вниз. Учит рисовать прямые шкалы (там не нужны синусы — только маппинг значение → y).
3. **Часы с двумя стрелками** — часовая короткая, минутная длинная. Учит: несколько «значений» в одном виджете и общий демпфинг.
4. **Оверлей потери данных**: метод `setStale(bool)`, при `true` — полупрозрачный чёрный прямоугольник поверх и надпись `NO DATA` (см. `BaseGaugeWidget::paintEvent` в проекте).
5. **Мини-график**: виджет хранит `std::deque` последних N значений, рисует полилинию `QPainterPath`. Это уменьшенный `HistoryChart` из проекта.

## 9. Грабли, на которые наступают все

| Симптом | Причина |
|---|---|
| Прибор не перерисовывается | Забыл `update()` в сеттере |
| Мусор/краш при рисовании | `QPainter` создан вне `paintEvent` |
| Стрелка «зависает» на полпути | Нет запроса следующего кадра (шаг 6, конец) |
| Прибор овальный/обрезанный | Нет `qMin(width(), height())` — круг вписан в прямоугольник |
| Стрелка ходит «зеркально» | Забыл, что ось Y вниз: +90° это низ, а не верх |
| Лесенки на круге | Нет `Antialiasing` |
| Штрихи съезжают при малых окнах | Целочисленные вычисления в формулах углов — считай во float |
| CPU 100% при простое | Бесконечный `update()` без порога сходимости — добавь epsilon |

## 10. Куда дальше

- Прочитай `receiver/MainWindow.cpp` в этом репозитории — это туториал, доведённый до продакшена: общая база приборов, именованные константы, палитра из `Theme.h`.
- Затем `receiver/HistoryChart.cpp` — вторая техника рисования (пиксельные координаты, автоскейл, сетка).
- Затем `receiver/UdpWorker.cpp` + `receiver/main.cpp` — приборы, подключённые к живым данным из другого потока (про это — `STYLEGUIDE.md`).
- Официальный пример Qt **Analog Clock Example** — родоначальник этого паттерна; прочитай после туториала, будет «так вот что я уже сделал».
