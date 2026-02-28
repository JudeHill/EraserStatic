#pragma once
#include "llm_handler.h"
#include "shared_var_identifier.h"

void test_llms_alive();
void eval_shared_variable_llm_consistency(const Filepath &in_filepath, const Filepath& out_filepath = "llm_consistency.txt");
