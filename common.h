#pragma once

#include <iosfwd>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

// Cell position. Index start from zero.
struct Position {
    int row = 0;
    int col = 0;

    bool operator==(Position rhs) const;
    bool operator<(Position rhs) const;

    bool IsValid() const;
    std::string ToString() const;

    static Position FromString(std::string_view str);

    static const int MAX_ROWS = 16384;
    static const int MAX_COLS = 16384;
    static const Position NONE;
};

struct Size {
    int rows = 0;
    int cols = 0;

    bool operator==(Size rhs) const;
};

// Describes errors that may occur during calculating a formula.
class FormulaError {
public:
    enum class Category {
        Ref,    // a link to a cell with an incorrect position
        Value,  // a cell cannot be interpreted as a number
        Div0,  // as a result of the calculation there was a division by zero
    };

    FormulaError(Category category);

    Category GetCategory() const;

    bool operator==(FormulaError rhs) const;

    std::string_view ToString() const;

private:
    Category category_;
};

std::ostream& operator<<(std::ostream& output, FormulaError fe);

// An exception thrown when trying to pass an incorrect position to the method
class InvalidPositionException : public std::out_of_range {
public:
    using std::out_of_range::out_of_range;
};

// An exception thrown when trying to set a syntactically incorrect
// formula
class FormulaException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// An exception thrown when trying to set a formula
// that leads to a cyclic dependency of cells
class CircularDependencyException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class CellInterface {
public:
    // The cell text, or the formula value, or an error message from
    // formula
    using Value = std::variant<std::string, double, FormulaError>;

    virtual ~CellInterface() = default;

    // Returns the visible value of the cell.
    // In the case of a text cell, this is text (without escaping characters).
    // In the case of a formula, this is the numerical value of the formula
    // or an error message.
    virtual Value GetValue() const = 0;
    // Returns the internal text of the cell, as if we had started editing it.
    // In the case of a text cell, it is text (possibly containing escape characters).
    // In the case of a formula, it is expression.
    virtual std::string GetText() const = 0;

    // Returns a list of cells that are directly involved in this formula.
    // The list is sorted in ascending order and does not contain duplicate cells.
    // In the case of a text cell, the list is empty.
    virtual std::vector<Position> GetReferencedCells() const = 0;
};

inline constexpr char FORMULA_SIGN = '=';
inline constexpr char ESCAPE_SIGN = '\'';

// Sheet Interface
class SheetInterface {
public:
    virtual ~SheetInterface() = default;

    // Sets the contents of the cell. If the text begins with the "=" sign, then it
    // interpreted as a formula. If a syntactically incorrect
    // formula, then a FormulaException exception is thrown and the cell value is not
    // changes. If a formula is given that leads to a cyclic dependency
    // (in particular, if the formula uses the current cell), then
    // a Circular Dependency Exception is thrown and the cell value is not
    // changes.
    // Clarifications on the formula entry:
    // * If the text contains only a character "=" and nothing else, then it is not a
    // formula
    // * If the text starts with a character "'" (apostrophe), then when the cell value is output
    // by method GetValue() "'" ignore. You can use, if necessary
    // start text with a sign "=", but so that it is not interpreted as a formula.
    virtual void SetCell(Position pos, std::string text) = 0;

    // Return the cell value.
    // If cell is empty, return nullptr.
    virtual const CellInterface* GetCell(Position pos) const = 0;
    virtual CellInterface* GetCell(Position pos) = 0;

    // Clear cell.
    // Next call GetCell() for current cell return nullptr or
    // object with empty text.
    virtual void ClearCell(Position pos) = 0;

    // Calculates the size of the area that is involved in printing.
    // Defined as the bounding rectangle of all cells with non-empty
    // text.
    virtual Size GetPrintableSize() const = 0;

    // Outputs the entire table to the transmitted stream. Columns are separated
    // by a tabulation sign. A newline character is output after each line.
    // For convertation cells into a string, the following methods are used GetValue()
    // or GetText(). An empty cell is always interpreted by an empty.
    virtual void PrintValues(std::ostream& output) const = 0;
    virtual void PrintTexts(std::ostream& output) const = 0;
};

// Creates a ready-to-use empty table.
std::unique_ptr<SheetInterface> CreateSheet();
