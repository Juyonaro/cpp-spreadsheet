#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <iostream>
#include <cassert>

using namespace std::literals;

void Sheet::SetCell(Position pos, std::string text) {
    // Проверяем, является ли позиция допустимой
    CheckValidPosition(pos);

    // Если ячейка на данной позиции не существует, создаем новую
    if (sheet_.count(pos) == 0) {
        sheet_[pos] = std::make_unique<Cell>(*this);
    }

    // Устанавливаем значение текста в ячейке
    sheet_[pos]->Set(text);
}

const CellInterface* Sheet::GetCell(Position pos) const {
    // Вызываем неконстантный метод GetCell и возвращаем результат
    return const_cast<Sheet*>(this)->GetCell(pos);
}

CellInterface* Sheet::GetCell(Position pos) {
    // Проверяем, является ли позиция допустимой
    CheckValidPosition(pos);
    
    // Если ячейка на данной позиции не существует, возвращаем nullptr
    if (sheet_.count(pos) == 0) {
        return nullptr;
    }
    
    // Возвращаем указатель на ячейку на данной позиции
    return sheet_.at(pos).get();
}

void Sheet::ClearCell(Position pos) {
    // Проверяем, является ли позиция допустимой
    CheckValidPosition(pos);
    
    // Получаем ссылку на ячейку на данной позиции
    const auto& cell = sheet_.find(pos);
    // Если ячейки на данной позиции не существует или есть зависимости, очищаем ячейку
    if (cell != sheet_.end() && cell->second != nullptr) {
        cell->second->Clear();
        cell->second.reset();
    }
}

Size Sheet::GetPrintableSize() const {
    // Инициализируем размер таблицы для вывода на печать
    Size printable_size = { 0, 0 };
    
    // Находим максимальную позицию в таблице
    for (auto it = sheet_.begin(); it != sheet_.end(); ++it) {
        if (it->second != nullptr) {
            const int col = it->first.col;
            const int row = it->first.row;
            
            printable_size.cols = std::max(printable_size.cols, col + 1);
            printable_size.rows = std::max(printable_size.rows, row + 1);
        }
    }
    
    // Возвращаем размер таблицы
    return printable_size;
}

void Sheet::PrintValues(std::ostream& output) const {
    // Получаем размер таблицы
    Size size = GetPrintableSize();
    
    // Выводим значение ячейки в поток вывода, разделяя значения тбауляцией
    for (int r = 0; r < size.rows; ++r) {
        for (int c = 0; c < size.cols; ++c) {
            if (c > 0) {
                output << "\t";
            }
            
            const auto& it = sheet_.find({ r, c });
            // Получаем значение ячейки
            if (it != sheet_.end() && it->second != nullptr && !it->second->GetText().empty()) {
                std::visit([&](const auto value) {
                    output << value;
                }, it->second->GetValue());
            }
        }
        output << "\n";
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    // Получаем размер таблицы
    Size size = GetPrintableSize();
    
    // Выводим значение ячейки в поток вывода, разделяя значения табуляцией
    for (int r = 0; r < size.rows; ++r) {
        for (int c = 0; c < size.cols; ++c) {
            if (c > 0) {
                output << "\t";
            }
            
            const auto& it = sheet_.find({ r, c });
            // Получаем значение ячейки
            if (it != sheet_.end() && it->second != nullptr && !it->second->GetText().empty()) {
                output << it->second->GetText();
            }
        }
        output << "\n";
    }
}

void Sheet::CheckValidPosition(Position pos) {
    // Проверяем, является ли позиция допустимой, иначе выбрасываем исключение
    if (!pos.IsValid()) {
        throw InvalidPositionException("invalid position"s);
    }
}

// Функция для создания экземпляра класса SheetInterface
std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}