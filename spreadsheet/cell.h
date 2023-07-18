#pragma once

#include "common.h"
#include "formula.h"

#include <optional>
#include <unordered_set>

class Cell : public CellInterface {
public:
    explicit Cell(SheetInterface& sheet);
    
    // Установка текста ячейки
    void Set(std::string text);
    // Очистка ячейки
    void Clear();
    // Получение значения ячейки
    Value GetValue() const override;
    // Получение текста ячейки
    std::string GetText() const override;
    // Получение списка ячеек, на которые ссылается текущая ячейка
    std::vector<Position> GetReferencedCells() const override;

private:
    class Impl {
    public:
        virtual ~Impl() = default;
        
        // Виртуальная функция получения значения ячейки
        virtual Value GetValue() const = 0;
        // Виртуальная функция получения текста ячейки
        virtual std::string GetText() const = 0;
        // Виртуальная функция получения списка ячеек, на которые ссылается текущая ячейка
        virtual std::vector<Position> GetReferencedCells() const;
        
        // Виртуальная функция получения кэша вычисленного значения ячейки
        virtual std::optional<FormulaInterface::Value> GetCache() const;
        // Виртуальная функция сброса кэша вычисленного значения ячейки
        virtual void ResetCache();
    };

    class EmptyImpl : public Impl {
    public:
        // Реализация функции получения значения пустой ячейки
        Value GetValue() const override;
        // Реализация функции получения текста пустой ячейки
        std::string GetText() const override;
    };

    class TextImpl : public Impl {
    public:
        // Конструктор класса TextImpl с текстовым значением ячейки
        explicit TextImpl(std::string expression);
        
        // Реализация функции получения значения текстовой ячейки
        Value GetValue() const override;
        // Реализация функции получения текста текстовой ячейки
        std::string GetText() const override;
        
    private:
        // Значение текстовой ячейки
        std::string value_;
    };

    class FormulaImpl : public Impl {
    public:
        // Конструктор класса FormulaImpl с формулой и ссылкой на таблицу
        explicit FormulaImpl(std::string expression, const SheetInterface& sheet);
        
        // Реализация функции получения значения ячейки с формулой
        Value GetValue() const override;
        // Реализация функции получения текста ячейки с формулой
        std::string GetText() const override;
        // Реализация функции получения списка ячеек, на которые ссылается ячейка с формулой
        std::vector<Position> GetReferencedCells() const override;
        
        // Получение кэша вычисленного значения формулы ячейки
        std::optional<FormulaInterface::Value> GetCache() const;
        // Сброс кэша вычисленного значения формулы ячейки
        void ResetCache();
        
    private:
        // Указатель на объект формулы
        std::unique_ptr<FormulaInterface> formula_;
        // Ссылка на таблицу
        const SheetInterface& sheet_;
        // Кэш вычисленного значения формулы ячейки
        mutable std::optional<FormulaInterface::Value> cache_;
    };
    
    // Проверка, является ли ячейка зависимой от других ячеек
    void CheckDependency(const std::vector<Position>& dep_cell) const;
    // Проверка на циклическую зависимость между ячейками
    void CheckCircularDepend(const std::vector<Position>& dep_cell, std::unordered_set<CellInterface*>& ref_cells) const;
    
    // Очистка кэша значения ячейки
    void InvalidateCache();
    // Рекурсивная очистка кэша значения ячейки и всех ячеек, от которых она зависит
    void InvalidateCacheRecursive();
    // Обновление списка зависимых ячеек
    void UpdateDependencies(const std::vector<Position>& new_ref_cells);
    
    // Ссылка на таблицу
    SheetInterface& sheet_;
    // Указатель на реализацию ячейки
    std::unique_ptr<Impl> impl_;
    // Отслеживание связей между ячейками
    std::unordered_set<Cell*> depend_; // Ячейки, которые зависят от других ячеек
    std::unordered_set<Cell*> reference_; // Ячейки от которых зависят другие ячейки
};