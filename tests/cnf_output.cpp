// Standalone driver for comparing in-memory output to Satsuma's DIMACS path.
#include "satsuma.h"
#include "parser.h"
#include <iostream>
#include <string>

class Output final : public satsuma::cnf_output
{
public:
  void begin(int variables, int clauses) override
  {
    std::cout << "p cnf " << variables << ' ' << clauses << '\n';
  }
  void clause(const int* literals, size_t size) override
  {
    for (size_t i = 0; i < size; ++i)
      std::cout << literals[i] << ' ';
    std::cout << "0\n";
  }
};

int main(int argc, char** argv)
{
  if (argc < 3)
    return 1;
  std::string filename(argv[2]);
  cnf2wl formula;
  parse_dimacs_to_cnf2wl(filename, formula, true);
  empty_stream silent;
  satsuma::tracker tracker;
  tracker.h_silent = true;
  satsuma::preprocessor preprocessor;
  preprocessor.set_tracker(&tracker);
  preprocessor.set_log_output(&silent);
  preprocessor.set_add_reduced_as_unit(true);
  preprocessor.set_preprocess_cnf_unit(true);
  preprocessor.set_proof_dense_crossover(60);
  preprocessor.set_component_size_limit(500000);
  preprocessor.set_order_model_budget(750000);
  preprocessor.set_dense_model_budget(20000000);
  if (std::string(argv[1]) == "fix")
  {
    preprocessor.set_orbitopal_fixing(true);
    preprocessor.set_orbitopal_fixing_only(true);
    preprocessor.set_schreier_fixing(true);
    preprocessor.set_negation_fixing(true);
    preprocessor.set_reorder(true);
    preprocessor.set_reorder_cliquer(true);
    preprocessor.set_iterate(true);
    preprocessor.set_preprocess_cnf_pure(true);
  }
  if (argc == 4)
    preprocessor.set_full_skip_limit(0);
  Output output;
  preprocessor.set_cnf_output(&output);
  preprocessor.preprocess(formula);
}
