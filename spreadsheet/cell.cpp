#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <algorithm>

using namespace std;

Cell::Cell(SheetInterface& sheet) : sheet_(sheet), impl_(std::make_unique<EmptyImpl>()) {}

void Cell::Set(std::string text) {
    // Если текст в ячейке уже совпадает - не нужно ничего делать
    if (impl_->GetText() == text) {
        return;
    }
    
    // Сохраняем текущие зависимости
    const auto& current_referenced = impl_->GetReferencedCells();
    // Обновляем значение ячейки // проверяем новые зависимости
    if (!text.empty()) {
        if (text[0] == FORMULA_SIGN && text.size() > 1) {
            unique_ptr<Impl> tmp_impl = make_unique<FormulaImpl>(text.substr(1), sheet_);
            
            // Сохраняем новые зависимости
            const auto& new_referenced = tmp_impl->GetReferencedCells();
            // Проверяем, есть ли циклическая зависимость
            CheckDependency(new_referenced);
            // Обновляем зависимости даже если new_referenced пустой
            UpdateDependencies(new_referenced);
            
            impl_ = std::move(tmp_impl);
        }
        else {
            const auto& new_referenced = impl_->GetReferencedCells();
            CheckDependency(new_referenced);
            UpdateDependencies(new_referenced);
            
            impl_ = make_unique<TextImpl>(std::move(text));
        }
    }
    else {
        impl_ = make_unique<EmptyImpl>();
        // Передаем пустой список зависимостей для очистки всех зависимостей
        UpdateDependencies({});
    }
    
    // Обновляем зависимости на основе сохраненных текущих зависимостей
    UpdateDependencies(current_referenced);
    
    // Инвалидация кэша ячейки
    InvalidateCache();
}

// Обновляем зависимости и сбрасываем кэш ячейки при очистке,
// передавая пустой текст в качестве аргумента
void Cell::Clear() {
    Set("");
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

bool Cell::IsReferenced() const {
    return !reference_.empty();
}

void Cell::CheckDependency(const std::vector<Position>& dep_cell) const {
    unordered_set<CellInterface*> ref_cells;
    // Проверяем, является ли текущая ячейка зависимой от других ячеек
    CheckCircularDepend(dep_cell, ref_cells);
}

void Cell::CheckCircularDepend(const std::vector<Position>& dep_cell, std::unordered_set<CellInterface*>& ref_cells) const {
    for (const auto& c : dep_cell) {
        CellInterface* referenced = sheet_.GetCell(c);
        // Проверяем, не является ли текущая ячейка ссылкой на себя
        if (referenced == this) {
            throw CircularDependencyException("The cyclic dependence is found"s);
        }
        
        if (referenced && ref_cells.count(referenced) == 0) {
            const auto& another_ref = referenced->GetReferencedCells();
            // Проверяем другие ячейки, на которые ссылается ячейка
            if (!another_ref.empty()) {
                CheckCircularDepend(another_ref, ref_cells);
            }
            
            ref_cells.insert(referenced);
        }
    }
}

// Устанавливаем новые зависимые ячейки и обновляем списки зависимостей
void Cell::UpdateDependencies(const std::vector<Position>& new_ref) {
    // Очищаем зависимость ячейки из списка ячеек,
    // на которые ранее ссылалась текущая ячейка
    for_each(reference_.begin(), reference_.end(), [this](Cell* referenced) {
        referenced->depend_.erase(this);
    });

    reference_.clear();

    for (const auto& c : new_ref) {
        if (!sheet_.GetCell(c)) {
            sheet_.SetCell(c, ""s);
        }
        
        Cell* new_reference = dynamic_cast<Cell*>(sheet_.GetCell(c));
        
        reference_.insert(new_reference);
        new_reference->depend_.insert(this);
    }
}

void Cell::InvalidateCache() {
    impl_->ResetCache();
    InvalidateCacheRecursive();
}

void Cell::InvalidateCacheRecursive() {
    for (const auto& dep_cell : depend_) {
        // Проверяем наличие кэша в каждой зависимой ячейке
        if (dep_cell->impl_->GetCache().has_value()) {
            // Если кэш присутствует - сбрасываем его
            dep_cell->impl_->ResetCache();
            // Выполняем повторную рекурсию для данной зависимой ячейки
            dep_cell->InvalidateCacheRecursive();
        }
    }
}

// Impl class
std::vector<Position> Cell::Impl::GetReferencedCells() const {
    return {};
}

std::optional<FormulaInterface::Value> Cell::Impl::GetCache() const {
    return nullopt;
}

void Cell::Impl::ResetCache() {}

// EmptyImpl class
CellInterface::Value Cell::EmptyImpl::GetValue() const {
    return ""s;
}

std::string Cell::EmptyImpl::GetText() const {
    return ""s;
}

// TextImpl class
Cell::TextImpl::TextImpl(std::string expression) : value_(std::move(expression)) {}

CellInterface::Value Cell::TextImpl::GetValue() const {
    if (value_[0] == ESCAPE_SIGN) {
        return value_.substr(1);
    }

    return value_;
}

std::string Cell::TextImpl::GetText() const {
    return value_;
}

// FormulaImpl class
Cell::FormulaImpl::FormulaImpl(std::string expression, const SheetInterface& sheet)
    : formula_(ParseFormula(std::move(expression))), sheet_(sheet) {}

CellInterface::Value Cell::FormulaImpl::GetValue() const {
    if (!cache_.has_value()) {
        cache_ = formula_->Evaluate(sheet_);
    }
    
    auto value = formula_->Evaluate(sheet_);
    if (std::holds_alternative<double>(value)) {
        return std::get<double>(value);
    }
    
    return std::get<FormulaError>(value);
}

std::string Cell::FormulaImpl::GetText() const {
    return FORMULA_SIGN + formula_->GetExpression();
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
    return formula_->GetReferencedCells();
}

void Cell::FormulaImpl::ResetCache() {
    cache_.reset();
}

std::optional<FormulaInterface::Value> Cell::FormulaImpl::GetCache() const {
    return cache_;
}