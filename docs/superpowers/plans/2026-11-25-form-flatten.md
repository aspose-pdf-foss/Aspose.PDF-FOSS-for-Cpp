# Form::Flatten без потери значений — план реализации

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** `Form::Flatten()` + `Save()` переносят видимые виджеты формы в содержимое страниц, а изменённые значения загруженных полей получают новый `/AP`.

**Architecture:** Генераторы внешнего вида выносятся из `Document::AppendAcroFormUpdate` в внутренний модуль `field_appearance`. Сохранение с Flatten = обычный проход формы (новые поля с `/AP`, изменённые загруженные — новый `/AP`) + отдельный инкрементальный проход `AppendFlattenWidgetsUpdate`, который рисует каждый видимый виджет через `Do` и удаляет виджеты и `/AcroForm`.

**Tech Stack:** C++20, MSVC (build/msvc), GoogleTest, сборка `cmake --build build/msvc --config Release --target aspose_pdf_foss_tests --parallel` (cmake: `E:/Aspose.Git/AsposePdfCpp/cs_porter_produce/tc_build/cmake-3.28.2/bin/cmake.exe`, в git bash с `MSYS_NO_PATHCONV=1`).

**Spec:** `docs/superpowers/specs/2026-11-25-form-flatten-design.md`

## Global Constraints

- Публичный API не меняется; новый модуль — в `include/internal/` + `src/internal/`.
- Вид существующих новых полей не меняется (Helvetica 12, рамка 1pt, белый фон, зелёная галочка).
- Нетронутые загруженные виджеты переносятся со своим `/AP` (тот же объект, без перегенерации).
- Имена XObject-ресурсов на странице: `AspFl<N>`, без конфликта с существующими ключами.
- Исходное `/Contents` страницы всегда обрамляется потоками `q` и `Q`.
- Пропуск виджетов с `/F` Hidden (бит 2, значение 2) и NoView (бит 6, значение 32).
- После `Form::Flatten()` `Form::Count() == 0`.
- Тестовый бинарник один (`aspose_pdf_foss_tests`); тесты в `tests/*.cpp` подхватываются глобом, `tests/internal/` исключён.
- Коммиты с trailer `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`; без push.

## Review Focus

1. Поле добавлено `Form().Add(field)` без номера страницы — его виджета нет ни в одном `/Annots`: Flatten не падает, `/AcroForm` удаляется (тест в Task 4).
2. Страница без `/Contents` (пустая, `Pages().Add()`) или с `/Contents`-массивом — обрамление и дописывание работают (тест в Task 4: форма на новой пустой странице).
3. Виджет с `/AP /N`-словарём состояний без `/AS` или с `/AS`, которого нет в словаре — ничего не рисуется, без исключения (тест в Task 4).
4. `/Rect` с перевёрнутыми координатами (URX < LLX) — нормализуется перед расчётом `cm` (тест в Task 4).
5. Flatten у документа без полей — сохранение проходит, файл валиден (тест в Task 4).

---

### Task 1: Модуль `field_appearance` (рефакторинг без изменения поведения)

**Files:**
- Create: `include/internal/field_appearance.hpp`, `src/internal/field_appearance.cpp`
- Modify: `src/public/document.cpp` (лямбды `escape_ap`, `field_chrome`, `text_ap`, `list_ap`, `check_ap`, `circle_path`, `radio_ap` и тело кнопки в `AppendAcroFormUpdate` заменяются вызовами модуля), `CMakeLists.txt` (добавить `src/internal/field_appearance.cpp` в список исходников библиотеки рядом с другими `src/internal/*`)
- Test: `tests/field_appearance_test.cpp`

**Interfaces:**
- Produces (namespace `foundation::field_appearance`), все возвращают тело потока в локальных координатах `[0 0 w h]`:
  - `std::string EscapeLiteral(const std::string& s);` — экранирует `( ) \`
  - `std::string TextBox(double w, double h, const std::string& value);` — шрифт `/Helv 12`
  - `std::string ListBox(double w, double h, const std::vector<std::string>& options, int selected);`
  - `std::string CheckBox(double w, double h, bool on);`
  - `std::string RadioButton(double w, double h, bool on);`
  - `std::string PushButton(double w, double h, const std::string& caption);` — пустой caption → `"Button"`
  - `inline constexpr const char* kTextFontKey = "Helv"; kTextBaseFont = "Helvetica"; kCheckFontKey = "ZaDb"; kCheckBaseFont = "ZapfDingbats";`

- [ ] **Step 1: Failing test** `tests/field_appearance_test.cpp`, suite `FieldAppearance`:
  - `TextBoxDrawsValue`: `TextBox(100, 20, "Hi (x)")` содержит `"/Helv 12 Tf"` и `"(Hi \\(x\\)) Tj"`, начинается с `"q\n"`, заканчивается `"Q\n"`.
  - `TextBoxEmptyHasNoText`: `TextBox(100, 20, "")` не содержит `"Tj"`.
  - `CheckBoxOnHasMarkOffHasNot`: `CheckBox(12,12,true)` содержит `"0 0.5 0 RG"`, `CheckBox(12,12,false)` — нет.
  - `RadioButtonOnHasDot`: on содержит `" f\n"`, off — нет.
  - `ListBoxHighlightsSelected`: `ListBox(80,60,{"a","b"},1)` содержит `"0.6 0.75 0.9 rg"`, `"(a) Tj"`, `"(b) Tj"`; с `selected=-1` — нет `"0.6 0.75 0.9 rg"`.
  - `PushButtonDefaultCaption`: `PushButton(60,20,"")` содержит `"(Button) Tj"`.
- [ ] **Step 2: Run** — сборка падает (нет заголовка).
- [ ] **Step 3: Implement** — перенести тела лямбд дословно в функции модуля; в `document.cpp` вызывать их (поведение и байты потоков не меняются).
- [ ] **Step 4: Run** `aspose_pdf_foss_tests.exe --gtest_filter=FieldAppearance.*:Forms*` → PASS; весь набор → 0 FAILED.
- [ ] **Step 5: Commit** `refactor: extract form field appearance generators into field_appearance`

### Task 2: `Form::Flatten()` сохраняет поля до Save

**Files:**
- Modify: `include/aspose/pdf/forms/form.hpp` (private: `std::vector<Field*> flatten_fields_;`), `src/public/form.cpp` (`Flatten`), `src/public/document.cpp` (`AppendAcroFormUpdate`: при `flatten` перебирать `form_->flatten_fields_` вместо `form_->fields_`; ветку «только удалить `/AcroForm`» пока оставить после обычного прохода)
- Test: `tests/forms_flatten_smoke_test.cpp`

**Interfaces:**
- Produces: `Form::flatten_fields_` — поля на момент `Flatten()`; `Form::fields_` после `Flatten()` пуст; `field_pages_` не очищается.

- [ ] **Step 1: Failing test** suite `FormsFlatten`:
  - `FlattenEmptiesFormForUser`: документ с 1 страницей, `TextBoxField tb{doc}` c Rect `(100,700,300,720)`, `Value("Hello")`, `doc.Form().Add(tb, 1)`, `Flatten()` → `doc.Form().Count() == 0`.
  - `FlattenedNewFieldIsWrittenBeforeRemoval`: то же + `Save(tmp)`; в байтах файла есть `"(Hello) Tj"` (поток `/AP` записан) и перечитанный `Document` имеет `Form().Count() == 0`.
- [ ] **Step 2: Run** → второй тест FAIL (нет `(Hello) Tj`).
- [ ] **Step 3: Implement** по Interfaces.
- [ ] **Step 4: Run** `--gtest_filter=FormsFlatten.*:FormsLoadOnOpen.*` → PASS.
- [ ] **Step 5: Commit** `fix: Form::Flatten keeps fields until Save so new fields are written`

### Task 3: Новый `/AP` для изменённых загруженных полей

**Files:**
- Modify: `src/public/document.cpp` — цикл `modified_loaded` в `AppendAcroFormUpdate`; `ApplyFieldValue` получает имя состояния «включено».
- Test: `tests/forms_flatten_smoke_test.cpp`

**Interfaces:**
- Produces (file-local в `document.cpp`):
  - `std::string CheckboxOnState(const foundation::objects::Dict& widget, const foundation::objects::Dump& dump);` — первый ключ `/AP /N`, отличный от `Off`; иначе `"Yes"`.
  - `ApplyFieldValue(Dict& d, Field* fp, const std::string& on_state = "Yes")`.
- Правило: у переписываемого поля с `/Rect` — заменить `/AP` (`/N` = новый поток; для флажка словарь `{on_state: on, Off: off}`); с `/Kids` — переписать `/AP` (и `/AS` для флажков) у каждого ребёнка-виджета. Генерация через функции Task 1 + общий `emit_ap`.

- [ ] **Step 1: Failing test**:
  - `EditedLoadedTextFieldGetsNewAppearance`: создать документ с полем `name`=`"Old"`, сохранить, открыть, `Value("New")`, сохранить в tmp2; в байтах tmp2 есть `"(New) Tj"`.
  - `EditedLoadedCheckboxKeepsItsOnStateName`: сохранить невключённый флажок; в байтах файла заменить все `/Yes` на `/Chk` (та же длина — xref остаётся верным); открыть, `Checked(true)`, сохранить; в выходе есть `/AS /Chk` и нет `/AS /Yes`.
- [ ] **Step 2: Run** → FAIL.
- [ ] **Step 3: Implement** по Interfaces.
- [ ] **Step 4: Run** `--gtest_filter=FormsFlatten.*:Forms*` → PASS.
- [ ] **Step 5: Commit** `fix: regenerate /AP for value-edited loaded form fields`

### Task 4: Проход переноса виджетов на страницы

**Files:**
- Modify: `include/aspose/pdf/document.hpp` (private: `std::vector<std::byte> AppendFlattenWidgetsUpdate(const std::vector<std::byte>& working) const;`), `src/public/document.cpp` (реализация; вызов из `AppendAcroFormUpdate` после обычного прохода при `flatten`, затем удаление `/AcroForm` — существующий код), `src/public/form.cpp` (комментарий `Flatten`), `include/aspose/pdf/forms/form.hpp` (комментарий)
- Test: `tests/forms_flatten_smoke_test.cpp`

**Interfaces:**
- Consumes: байты после обычного прохода (Task 2/3).
- Алгоритм на страницу (по `tree_`-листам, перечитанным из `working`): собрать `/Annots`-виджеты (`/Subtype /Widget`); для каждого видимого — поток `/AP /N` (ref на Stream или словарь состояний по `/AS`); `cm` = отображение `/BBox` (с учётом `/Matrix`, по умолчанию единичная) в нормализованный `/Rect`; новые объекты: поток `q\n`, поток с `q <cm> cm /AspFl<k> Do Q` на каждый виджет и `Q\n` в начале; `/Contents` = `[qstream, ...old, Qstream+widgets]`; `/Resources /XObject` += `AspFl<k>` → ref потока (ресурсы — через `ResolveDictCopy`); `/Annots` без перенесённых и без всех виджетов; страницы без виджетов не трогаются.

- [ ] **Step 1: Failing tests** (хелпер `RenderPng(Document&, int page) -> std::vector<uint8_t> rgba` через `PngDevice` + `foundation::png_decoder`; `DarkPixelsIn(rgba, rect, page_h, dpi)`):
  - `FlattenBurnsNewTextValue`: поле `"Hello"` на (100,700,300,720) → Flatten → Save → reopen: нет `/AcroForm` и `/Widget` в последней ревизии страницы, есть `" Do"`; рендер 72 dpi — тёмные пиксели внутри rect.
  - `FlattenBurnsEditedLoadedValue`: загруженное поле, `Value("WWWWWWWW")` vs исходное `"i"` — число тёмных пикселей в rect после Flatten больше, чем у исходного рендера.
  - `FlattenReusesUntouchedLoadedAppearance`: id потока `/AP /N` виджета до == ref `AspFl1` в ресурсах страницы после.
  - `FlattenCheckboxStates`: включённый флажок → в rect есть зелёные пиксели; выключенный → нет.
  - `FlattenSkipsHiddenWidgetKeepsOtherAnnots`: виджет с `/F 2` не даёт `Do`; `SquareAnnotation` на той же странице остаётся в `/Annots`.
  - Review Focus: `FlattenFieldWithoutPage` (Add без страницы → Save ок, нет `/AcroForm`), `FlattenOnBlankNewPage` (страница из `Pages().Add()`), `FlattenStateDictWithoutAS` (байтовая подмена `/AS /Yes` → `/AS /Zzz` → без исключения, без `Do` для него), `FlattenReversedRect` (Rect (300,720,100,700) → пиксели в нормализованном rect), `FlattenNoFields` (`Flatten()` на документе без полей → Save → открывается).
- [ ] **Step 2: Run** → FAIL.
- [ ] **Step 3: Implement** `AppendFlattenWidgetsUpdate` по алгоритму.
- [ ] **Step 4: Run** `--gtest_filter=FormsFlatten.*:Forms*` → PASS; весь набор → 0 FAILED.
- [ ] **Step 5: Commit** `feat: Form::Flatten burns widget appearances into page content`

### Task 5: Документация и финальная проверка

**Files:** `CHANGELOG.md` (Unreleased → Fixed: Flatten сохраняет значения; изменённые загруженные поля получают новый `/AP`; флажки используют своё имя состояния), README «Scope and Limitations» — не меняется (ограничение там не упомянуто).

- [ ] **Step 1:** Полная сборка `ALL_BUILD` (с примерами) — exit 0, без новых предупреждений.
- [ ] **Step 2:** Весь набор тестов — 0 FAILED, число SKIPPED = 23.
- [ ] **Step 3: Commit** `docs: changelog for form flatten`
