#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>
#include <set>

class Sheet;

class Cell : public CellInterface {
public:
    Cell(SheetInterface& sheet);

    Cell(SheetInterface& sheet, std::string text);

    void Set(std::string text);

    void Clear();

    Value GetValue() const override;

    std::string GetText() const override;

    std::vector<Position> GetReferencedCells() const override;

    bool IsReferenced() const;

    void AddDependency(Position pos);

    void RemoveOldLinks(Position pos);

    void AddOldDependents(const std::set<Position> old_depend_cells);

    const std::set<Position>& GetDependentsCells() const;

    void UpdateReferencedCells();

    ~Cell();
private:
    class Impl {
    public:
        enum class ImplType {
            FORMULA,
            TEXT,
            EMPTY
        };

        static ImplType DefineImplType(const std::string& text);

        virtual Value GetValue() const = 0;

        virtual std::string GetText() const = 0;
        
        virtual std::vector<Position> GetReferencedCells() const = 0;

        virtual ~Impl() = default;
    };

    class EmptyImpl : public Impl {
    public:
        EmptyImpl() = default;
        
        virtual Value GetValue() const override;

        virtual std::string GetText() const override;

        std::vector<Position> GetReferencedCells() const override;

        virtual ~EmptyImpl() override = default;
    };

    class TextImpl : public Impl {
    public:
        TextImpl(std::string text);

        virtual Value GetValue() const override;

        virtual std::string GetText() const override;

        std::vector<Position> GetReferencedCells() const override;

        virtual ~TextImpl() override = default;

    private:
        std::string value_;
    };

    class FormulaImpl : public Impl {
    public:
        FormulaImpl(std::string text, const SheetInterface& sheet);

        virtual Value GetValue() const override;

        virtual std::string GetText() const override;

        std::vector<Position> GetReferencedCells() const override;

        virtual ~FormulaImpl() override = default;
    private:
        const SheetInterface& sheet_;
        std::unique_ptr<FormulaInterface> formula_;
    };

private:
    void ResetCacheDependentsCells();
   
    SheetInterface& sheet_;  
     
    std::unique_ptr<Impl> impl_;          
//Schematic representation of dependencies: referenced_cells_<---ACTUAL_CELL<---dependents_cells_
//ACTUAL_CELL - current cell
    std::set<Position> dependents_cells_;    
    std::set<Position> referenced_cells_;      
    
    mutable std::optional<Value> cache_value_;  
};