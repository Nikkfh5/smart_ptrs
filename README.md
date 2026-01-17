# smart_ptrs

Реализация “умных указателей” в C++ без использования `std::unique_ptr / std::shared_ptr / std::weak_ptr / std::enable_shared_from_this`.

Внутри:
- `UniquePtr<T, Deleter>` с поддержкой кастомных deleter-ов и оптимизацией размера через `CompressedPair` (EBO).
- `SharedPtr<T>` + `WeakPtr<T>` на базе control block с счётчиками strong/weak.
- `MakeShared<T>(args...)` с одной аллокацией (control block + объект вместе).
- `EnableSharedFromThis<T>` (`SharedFromThis()` / `WeakFromThis()`).

## Структура репозитория

- `common/`
  - `my_int.h` — маленький тип для тестов времени жизни объектов.
- `shared-from-this/`
  - `unique.h` — `UniquePtr`
  - `compressed_pair.h` — оптимизация хранения `{ptr, deleter}` (EBO)
  - `deleters.h` — тестовые deleter-ы
  - `shared.h` — `SharedPtr`, `MakeShared`, `EnableSharedFromThis`
  - `weak.h` — `WeakPtr`
  - `sw_fwd.h` — forward declarations + `BadWeakPtr`
  - `test_unique.cpp`, `test.cpp`, `test_weak.cpp` — тесты

## Сборка

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## UniquePtr
move-only семантика

Release, Reset, Swap, Get, GetDeleter

специализация для массивов T[]

хранение deleter-а вместе с указателем через CompressedPair

## SharedPtr / WeakPtr
базовый control block (ref_cnt_shared, ref_cnt_weak)

удаление managed-объекта при ref_cnt_shared == 0

удаление control block при ref_cnt_shared == 0 && ref_cnt_weak == 0

WeakPtr::Lock(), Expired(), UseCount()

конструктор SharedPtr из WeakPtr кидает BadWeakPtr при истёкшем объекте

## EnableSharedFromThis
привязка внутреннего weak-ссылочного состояния к control block

SharedFromThis() и WeakFromThis() для получения ссылок на текущий объект
