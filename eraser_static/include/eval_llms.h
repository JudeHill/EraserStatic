/* 
 * Project: EraserStatic
 * (https://github.com/JudeHill/EraserStatic)
 *
 * Copyright (C) 2025-2026 Jude Hill <jude-stephen-hill@outlook.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include "llm_handler.h"
#include "shared_var_identifier.h"

void test_llms_alive();
void eval_shared_variable_llm_consistency(const Filepath &in_filepath, bool slow_llm_requests, const Filepath& out_filepath = "llm_consistency.txt");
