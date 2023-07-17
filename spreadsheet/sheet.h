#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <vector>
#include <memory>
#include <deque>
#include <unordered_map>

class Sheet : public SheetInterface {
public:
    ~Sheet() override = default;
    
    // Установка значения ячейки на заданной позиции
    void SetCell(Position pos, std::string text) override;
    
    // Получение константного указателя на ячейку на заданной позиции
    const CellInterface* GetCell(Position pos) const override;
    // Получение указателя на ячейку на заданной позиции
    CellInterface* GetCell(Position pos) override;
    
    // Очистка ячейки на заданной позиции
    void ClearCell(Position pos) override;
    
    // Получение размера таблицы для печати
    Size GetPrintableSize() const override;
    // Печать значений ячеек в поток вывода
    void PrintValues(std::ostream& output) const override;
    // Печать текстов ячеек в поток вывода
    void PrintTexts(std::ostream& output) const override;

private:
    struct PositionHasher {
        size_t operator()(const Position& pos) const {
            return pos.row + pos.col * 37;
        }
    };
    
    // Проверка, является ли позиция допустимой
    static void IsValidPosition(Position pos);
    
    // Хранение ячеек таблицы
    std::unordered_map<Position, std::unique_ptr<Cell>, PositionHasher> sheet_;
};
