# Диспетчер и модуль stvlink перенесены из karavan в stv

**Статус:** accepted

> ADR хранится в репозитории stv, потому что решение меняет состав самой
> библиотеки: payload-независимые классы приёмной стороны stvlink добавлены
> в stv для всех проектов, использующих stv и karavan.

stvlink — публичный протокол обмена, его приёмный тракт (парсер кадра,
передающий декоратор, диспетчеризация по `msg_id`, модуль обработки
очереди парсера) не зависит от семантики полезной нагрузки. Тем не менее
диспетчер, базовый класс, структура настройки и модуль исторически
находились в karavan (`karavan/include/karavan/cambridge.hpp`) вперемешку
со структурами payload протокола Cambridge
(`cambridge_exploder_telemetry`, `lidar_advance`, `exploder_setup`). Из-за
этого проект, использующий stvlink без payload Cambridge, вынужден был
зависеть от karavan, а терминология схемы (`stvlink.plantuml`) расходилась
с размещением кода: публичный протокол обрабатывался чужеродной
библиотекой.

Решено перенести всю обработку stvlink, кроме полезной нагрузки, из
karavan в stv:

- `karavan::cambridge_dispatcher` → `stv::stvlink_dispatcher`
  (файл `stv/include/stv/communication/stvlink_dispatcher.hpp`);
- `karavan::cambridge_base` → `stv::stvlink_base`;
- `karavan::cambridge_setup` → `stv::stvlink_setup`;
- `karavan::cambridge` (модуль) → `stv::stvlink_module`;
- `karavan::cambridge_message_span` → `stv::stvlink_message_span`;
- `karavan::cambridge_message_handler_fnc_type` →
  `stv::stvlink_message_handler_fnc_type`;
- тест диспетчера перенесён из karavan в stv
  (`stv/include/stv/communication/tests/test_stvlink_dispatcher.cpp`),
  полезная нагрузка в нём заменена структурой-заглушкой — тесты stv
  линкуются только со `stv::stv` и не могут включать заголовки karavan;
- в karavan старые имена сохранены как `[[deprecated]]`-алиасы
  (в `karavan/include/karavan/cambridge.hpp`) для внешних потребителей;
  алиасы `cambridge_mcu`/`cambridge_mcu_setup`/`cambridge_gui`/
  `cambridge_gui_setup` в `cambridge_mcu.hpp`/`cambridge_gui.hpp`
  переадресованы на stv-имена без атрибута;
- payload-структуры Cambridge (`cambridge_exploder_telemetry`,
  `lidar_advance`, `exploder_setup`) и их сериализация остаются в karavan
  без изменений.

Формат кадра на проводе не изменился: диспетчер читает тот же заголовок
сообщения `stv::stvlink_sender::message_header_t` и срезает его через
`message_header_size()` (4 байта), поведение приёмного тракта прежнее.

## Considered Options

- **Оставить обработку stvlink в karavan** — отвергнуто: stvlink —
  публичный протокол, его приёмный тракт не зависит от payload Cambridge;
  зависимость «stvlink-протокол ↔ karavan» блокирует использование stvlink
  в проектах без payload Cambridge.
- **Перенести в stv вместе с payload-структурами Cambridge** — отвергнуто:
  payload-структуры — часть доменной модели устройства (телеметрия,
  настройки лидара, параметры устройства), а не протокола; они остаются в
  karavan и сериализуются в stvlink-сообщения на стороне потребителя.
- **Удалить старые имена без deprecated-алиасов** — отвергнуто: ломка API
  karavan для внешних потребителей без переходного периода; алиасы с
  `[[deprecated]]` дают предупреждение при миграции и не влияют на
  поведение.

## Consequences

- **Расширение API stv:** добавлены `stv::stvlink_dispatcher`,
  `stv::stvlink_base`, `stv::stvlink_setup`, `stv::stvlink_module`,
  `stv::stvlink_message_span`, `stv::stvlink_message_handler_fnc_type`;
  проекты, использующие stvlink, больше не обязаны зависеть от karavan.
- **Устаревание API karavan:** имена `karavan::cambridge_dispatcher`,
  `karavan::cambridge_base`, `karavan::cambridge_setup`,
  `karavan::cambridge`, `karavan::cambridge_message_span`,
  `karavan::cambridge_message_handler_fnc_type` помечены
  `[[deprecated]]` и будут удалены в будущем релизе; внутренние call sites
  (board, gui, karavan-тесты) переведены на stv-имена, предупреждений
  deprecated в сборке не возникает.
- Поведение и формат кадра stvlink на проводе не изменились.
- Терминология и схема `stvlink.plantuml` приведены к размещению кода:
  диспетчер и модуль относятся к stv.
