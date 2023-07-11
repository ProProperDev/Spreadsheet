#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

Cell::Cell(SheetInterface& sheet)
    : sheet_(sheet)
    , impl_(std::make_unique<EmptyImpl>())
    , dependents_cells_()
    , referenced_cells_()
    , cache_value_(std::nullopt) {
}

Cell::Cell(SheetInterface& sheet, std::string text)
    : Cell(sheet) {
        Set(text);
}

bool Cell::IsReferenced() const {
    return !dependents_cells_.empty() || !referenced_cells_.empty();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}
//Add a cell position dependent from current cell
void Cell::AddDependency(Position pos) {
    dependents_cells_.insert(pos);
}
//Add positions of dependent cells from current cell(this method is used when updating the content of an existing cell)
//perhaps it makes sense to use the copy constructor instead
void Cell::AddOldDependents(const std::set<Position> old_depend_cells) {
    dependents_cells_ = old_depend_cells;
}

const std::set<Position>& Cell::GetDependentsCells() const {
    return dependents_cells_;
}

void Cell::Set(std::string text) {
    Impl::ImplType impl_type = Impl::DefineImplType(text);
    switch(impl_type) {
        case Impl::ImplType::FORMULA:
            impl_ = std::make_unique<FormulaImpl>(text, sheet_);
            break;
        case Impl::ImplType::TEXT:
            impl_ = std::make_unique<TextImpl>(text);
            break;
        case Impl::ImplType::EMPTY:
            impl_ = std::make_unique<EmptyImpl>();
            break;
    }
    cache_value_.reset();
    ResetCacheDependentsCells();
}

Cell::~Cell() {}

Cell::Impl::ImplType Cell::Impl::DefineImplType(const std::string& text) {
    if (text.empty()) {
        return ImplType::EMPTY;
    }
    if (text[0] == '=') {
        return ImplType::FORMULA;
    }
    return ImplType::TEXT;   
}

void Cell::UpdateReferencedCells() {
    auto ref_cells = impl_->GetReferencedCells();
    referenced_cells_.clear();
    referenced_cells_.insert(ref_cells.cbegin(), ref_cells.cend());
}
//Recursively resetting the cache value of all cells up the tree
void Cell::ResetCacheDependentsCells() {
    if (dependents_cells_.empty()) {
        return;
    }
    for (auto depend_cell : dependents_cells_) {
        dynamic_cast<Cell*>(sheet_.GetCell(depend_cell))->cache_value_.reset();
        dynamic_cast<Cell*>(sheet_.GetCell(depend_cell))->ResetCacheDependentsCells();
    }
}
//From the cells referenced by the current cell, we remove the fact that it depends from them
//This method is used when updating the contents of a cell
void Cell::RemoveOldLinks(Position pos) {
    for (auto ref_cell : referenced_cells_) {
        dynamic_cast<Cell*>(sheet_.GetCell(ref_cell))->dependents_cells_.erase(pos);
    }
}

void Cell::Clear() {
    Set("");
    referenced_cells_.clear();
}

Cell::Value Cell::GetValue() const {
    if (!cache_value_) {
       cache_value_ = impl_->GetValue();
    }
    return *cache_value_;
}

std::string Cell::GetText() const {
    return impl_->GetText();
}


/////FormulaImpl/////

Cell::FormulaImpl::FormulaImpl(std::string text, const SheetInterface& sheet)
    : sheet_(sheet)
    , formula_(ParseFormula(text.substr(1))){ //Cutting '='
}

Cell::Value Cell::FormulaImpl::GetValue() const {
    auto out_value = formula_->Evaluate(sheet_);
    if (std::holds_alternative<double>(out_value)) {
        return std::get<double>(out_value);
    } else {
        return std::get<FormulaError>(out_value);
    }
}

std::string Cell::FormulaImpl::GetText() const {
    return FORMULA_SIGN + formula_->GetExpression();
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
    return formula_->GetReferencedCells();
}


/////TextImpl/////

Cell::TextImpl::TextImpl(std::string text)
    :value_(std::move(text)) {
}

Cell::Value Cell::TextImpl::GetValue() const {
    if (!value_.empty() && value_.at(0) == ESCAPE_SIGN) {
        return value_.substr(1);
    } else {
        return value_;
    }
}

std::string Cell::TextImpl::GetText() const {
    return value_;
}

std::vector<Position> Cell::TextImpl::GetReferencedCells() const {
    return {};
}


/////EmptyImpl/////

Cell::Value Cell::EmptyImpl::GetValue() const {
    return std::string();
}

std::string Cell::EmptyImpl::GetText() const {
    return std::string();
}

std::vector<Position> Cell::EmptyImpl::GetReferencedCells() const {
    return {};
}