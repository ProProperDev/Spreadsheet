#pragma once

#include "common.h"

#include <memory>
#include <vector>

// A formula that allows calculating and updating an arithmetic expression.
// Supported Features:
// * Simple binary operations and numbers, brackets: 1+2*3, 2.5*(2+3.5/7)
// * Cell values as variables: A1+B2*C3
// Cells, contained in the formula, can be both formulas and text. If cell value is text,
// but the text can be interpreted as a number, then it will be interpreted as a number.
// An empty cell or a cell with empty text is interpreted as the number zero.
class FormulaInterface {
public:
    using Value = std::variant<double, FormulaError>;

    virtual ~FormulaInterface() = default;

    // A reference to the table is passed to the Evaluate() method
    // as an argument.
    // Returns the calculated value of the formula for the transmitted sheet or an error.
    // If calculating any of the cells specified in the formula leads to an error, then
    // it is this error that is returned. If there are several such errors, 
    // any of this errors is returned.
    virtual Value Evaluate(const SheetInterface& sheet) const = 0;

    // Returns an expression that describes the formula.
    // It does not contain spaces and extra brackets.
    virtual std::string GetExpression() const = 0;

    // Returns a list of cells, which are directly involved in the formula calculation
    // The list is sorted in ascending order and does not contain duplicate cells.
    virtual std::vector<Position> GetReferencedCells() const = 0;
};

// Parses the transmitted expression and returns the formula object.
// Throws a FormulaException if the formula is syntactically incorrect.
std::unique_ptr<FormulaInterface> ParseFormula(std::string expression);
