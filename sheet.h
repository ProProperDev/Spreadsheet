#pragma once

#include "cell.h"
#include "common.h"

#include <unordered_map>
#include <functional>
#include <vector>


class Sheet : public SheetInterface {
public:
    ~Sheet() override;
 
    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;

    void PrintTexts(std::ostream& output) const override;
    
private:
    void CycleDependencyFound(CellInterface* tmp_cell, Position pos);

    void SafeAddDependForRefCells(CellInterface* depend_cell, Position pos);

    Size ComputePrintSize() const;
    
    struct HashSheet {
        size_t operator()(Position pos) const {
            return pos.row*39 + (pos.col)*39*39;
        }
    };

    std::unordered_map<Position, std::unique_ptr<Cell>, HashSheet> sheet_;
};