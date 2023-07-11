#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

Size Sheet::ComputePrintSize() const {
    if (sheet_.empty()) {
        return {0, 0};
    }
    int max_row = 0;
    int max_col = 0;
    for (const auto& [pos, cell] : sheet_) {
        max_row = max_row >= pos.row ? max_row : pos.row;
        max_col = max_col >= pos.col ? max_col : pos.col;
    }
    return {max_row + 1, max_col + 1};
}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Trying SetCell with Invalid position");
    }

    std::unique_ptr new_cell_ptr = std::make_unique<Cell>(*this, text);

    CycleDependencyFound(new_cell_ptr.get(), pos);

//If the cell is already initialized,
//then copy the dependencies to a new cell
    if (sheet_.count(pos)) {
        sheet_[pos]->RemoveOldLinks(pos);
        new_cell_ptr->AddOldDependents(sheet_[pos]->GetDependentsCells());
    }

    new_cell_ptr->UpdateReferencedCells();

//If the cell we want to add references(depends) to uninitialized cells,
//then in this method we initialize the necessary cells as empty 
//and we add dependencies to them, in order not to lose dependencies in the future
    SafeAddDependForRefCells(new_cell_ptr.get(), pos);

    sheet_[pos] = std::move(new_cell_ptr);   
}

void Sheet::SafeAddDependForRefCells(CellInterface* depend_cell, Position pos) {
    if (!depend_cell) {
        return;
    }
    auto ref_cells = depend_cell->GetReferencedCells();
    std::for_each(begin(ref_cells), end(ref_cells),
                  [&pos, this](auto ref_cell_pos) { if (!sheet_.count(ref_cell_pos)) {
                                                        sheet_[ref_cell_pos] = std::make_unique<Cell>(*this);
                                                    }
                                                        sheet_[ref_cell_pos]->AddDependency(pos);});
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Trying GetCell with Invalid position");
    }
    if (sheet_.empty() || sheet_.count(pos) == 0) {
        return nullptr;
    }
    return sheet_.at(pos).get();
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Trying GetCell with Invalid position");
    }
    if (sheet_.empty() || sheet_.count(pos) == 0) {
        return nullptr;
    }
    return sheet_[pos].get();
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Trying ClearCell with Invalid position");
    }
    if (sheet_.empty() || sheet_.count(pos) == 0) {
        return;
    }
//An empty cell does not depend on others, then in the cells, previously
//referenced by the current cell delete this position like "dependence cell"
    sheet_[pos]->RemoveOldLinks(pos);
    sheet_[pos]->Clear();
    sheet_.erase(pos);
}

Size Sheet::GetPrintableSize() const {
    return ComputePrintSize();
}

void Sheet::PrintValues(std::ostream& output) const {
      auto print_ = [&output](const auto& obj) { output << obj; };
    Size sheet_area = GetPrintableSize();

    for (int row = 0; row < sheet_area.rows; ++row) {
        bool row_space_flag = false;
        for (int col = 0; col < sheet_area.cols; ++col) {
            Position pos{ row, col };
            auto cell = GetCell(pos);

            if (row_space_flag) {
                output << '\t';
            }
            if (cell) {
                std::visit(print_, cell->GetValue());
            }
            row_space_flag = true;
        }
        output << std::endl;
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    Size sheet_area = GetPrintableSize();

    for (int row = 0; row < sheet_area.rows; ++row) {
        bool row_space_flag = false;
        for (int col = 0; col < sheet_area.cols; ++col) {
            Position pos{ row, col };
            auto cell = GetCell(pos);

            if (row_space_flag) {
                output << '\t';
            }
            if (cell) {
                output << cell->GetText();
            }
            row_space_flag = true;
        }
        output << std::endl;
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}

void Sheet::CycleDependencyFound(CellInterface* tmp_cell, Position pos) {
    std::vector<Position> cells_to_check = tmp_cell->GetReferencedCells();
    std::set<Position> checked_positions;

    while (!cells_to_check.empty()) {
        Position current_pos = cells_to_check[0];
        if (current_pos == pos) {
            throw CircularDependencyException("");
        }
        if (checked_positions.count(current_pos)){
            cells_to_check.erase(cells_to_check.begin());
            continue;
        }
        checked_positions.insert(current_pos);
        CellInterface* cell;
        try {
            cell = GetCell(current_pos);
        } catch (InvalidPositionException&) {
            cell = nullptr;
        }
        if (cell){       
            for (auto cell_ : cell->GetReferencedCells()) {
                cells_to_check.push_back(cell_);
            }
        }
        cells_to_check.erase(cells_to_check.begin());
    }
}
