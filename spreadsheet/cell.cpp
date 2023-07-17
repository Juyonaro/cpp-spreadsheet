#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <algorithm>

using namespace std;

Cell::Cell(SheetInterface& sheet) : sheet_(sheet), impl_(std::make_unique<EmptyImpl>()) {}

void Cell::Set(std::string text) {
    // Очищаем список ссылок перед установкой нового значения ячейки
    reference_.clear();
    
    if (!text.empty()) {
        if (text[0] == FORMULA_SIGN && text.size() > 1) {
            unique_ptr<Impl> tmp_impl = make_unique<FormulaImpl>(text.substr(1), sheet_);
            
            const auto& new_referenced = tmp_impl->GetReferencedCells();
            IsDependency(new_referenced);
            
            if (!new_referenced.empty()) {
                UpdateDependencies(new_referenced);
            }
            
            impl_ = std::move(tmp_impl);
        }
        else {
            impl_ = make_unique<TextImpl>(std::move(text));
        }
    }
    else {
        impl_ = make_unique<EmptyImpl>();
    }
    
    InvalidateCache();
}

void Cell::Clear() {
    impl_ = make_unique<EmptyImpl>();
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

void Cell::IsDependency(const std::vector<Position>& dep_cell) const {
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
    reference_.clear();
    
    for_each(depend_.begin(), depend_.end(), [this](Cell* dependent) {
        dependent->depend_.erase(this);
    });
    
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
    
    std::unordered_set<Cell*> ref_cells;
    InvalidateCacheRecursive(ref_cells);
}

void Cell::InvalidateCacheRecursive(std::unordered_set<Cell*>& ref_cells) {
    for (const auto& dep_cell : depend_) {
        dep_cell->impl_->ResetCache();
        
        if (ref_cells.count(dep_cell) == 0) {
            if (!dep_cell->depend_.empty()) {
                dep_cell->InvalidateCacheRecursive(ref_cells);
            }
            
            ref_cells.insert(dep_cell);
        }
    }
}

// EmptyImpl class
std::vector<Position> Cell::Impl::GetReferencedCells() const {
    return {};
}

std::optional<FormulaInterface::Value> Cell::Impl::GetCache() const {
    return nullopt;
}

void Cell::Impl::ResetCache() {}

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
    
    switch (cache_.value().index()) {
        case 0:
            return get<0>(cache_.value());
        case 1:
            return get<1>(cache_.value());
        default:
            assert(false);
            throw std::runtime_error("Error"s);
    }
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