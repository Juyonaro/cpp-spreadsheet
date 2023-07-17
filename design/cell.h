#pragma once

#include "common.h"
#include "formula.h"
#include "sheet.h"

class Cell : public CellInterface {
public:
    Cell(SheetInterface& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;
    
private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;
    
    bool IsCircularDependency();
    void InvalidateCache();
    
    std::unique_ptr<Impl> impl_;
    SheetInterface& sheet_;
    
    // Отслеживание связей между ячейками
    std::unordered_set<Cell*> depend_; // Ячейки, которые зависят от других ячеек
    std::unordered_set<Cell*> reference_; // Ячейки от которых зависят другие ячейки
};