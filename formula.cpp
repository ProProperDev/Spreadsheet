#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <set>

using namespace std::literals;

namespace {
    class Formula : public FormulaInterface {
    public:
        explicit Formula(std::string expression) : ast_(ParseFormulaAST(expression)) {
        }

        Value Evaluate(const SheetInterface& sheet) const override {
            std::function<double(Position)> interpret_function = [&sheet](Position pos) {
                //Interpretation of an uninitialized cell
                if (sheet.GetCell(pos) == nullptr) {                                    
                    return 0.0;
                }
                auto value = sheet.GetCell(pos)->GetValue();
                //Interpretation of a cell containing a number
                if (std::holds_alternative<double>(value)) {
                    return std::get<double>(value);
                } else if (std::holds_alternative<std::string>(value)) {
                    std::string text = sheet.GetCell(pos)->GetText();
                //Empty text is also interpreted as double 0.0
                    if (text.empty()) {
                        return 0.0;
                    }
                //We throw an exception that this can only be interpreted as a text
                    if (text.front() == ESCAPE_SIGN) {
                        throw FormulaError(FormulaError::Category::Value); //Display #VALUE!
                    }
                //Trying to convert text to double
                    try {
                        return std::stod(text);
                    } catch (...) { 
                        throw FormulaError(FormulaError::Category::Value); //Display #VALUE!
                    }
                } else {
                    throw std::get<FormulaError>(value);
                }
            };

            try {
                return ast_.Execute(interpret_function);
            } catch (const FormulaError& fe) {
                return fe;
            }
        }

        std::string GetExpression() const override {
            std::ostringstream out;
            ast_.PrintFormula(out);
            return out.str();
        }

        std::vector<Position> GetReferencedCells() const override {
            auto positions = ast_.GetCells();
            std::set<Position> uniq_ref_cells(positions.begin(), positions.end());
            return {uniq_ref_cells.begin(), uniq_ref_cells.end()};
        }

        virtual ~Formula() override = default;

    private:
        FormulaAST ast_;
    };

}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {
        return std::make_unique<Formula>(std::move(expression));
    } catch (...) {
        throw FormulaException("Parsing error");
    }
}