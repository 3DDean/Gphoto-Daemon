#pragma once
#include "instruction.h"
#include "named_pipe.h"
#include "status.h"
#include <ctre.hpp>
#include <vector>
struct logger
{
	logger(std::string_view log_path)
		: log_path(log_path)
	{
	}

	std::string log_path;
};

// TODO Add config loading
template <typename InstructionSet>
struct command_pipe
{
	using stack_allocated_buffer = stack_buffer<512>;
	read_pipe<stack_allocated_buffer> instruction_pipe;
	InstructionSet instructions;
	status_manager statusManager;
	logger log;

	command_pipe(
		InstructionSet instructions,
		std::string_view pipe_path,
		std::string_view status_path,
		std::string_view log_path)
		: instructions(instructions),
		  instruction_pipe(pipe_path),
		  statusManager(status_path),
		  log(log_path)
	{
	}

	command_pipe(
		InstructionSet instructions,
		std::string_view name,
		std::string_view dir)
		: instructions(instructions),
		  instruction_pipe(std::string(dir) + name.data() + ".pipe"),
		  statusManager(std::string(dir) + name.data() + ".status"),
		  log(std::string(dir) + name.data() + ".log")
	{
	}

	template <typename... ArgsT>
	void parse_commands(ArgsT &&...args)
	{
		// TODO investigate this weird interface, I don't think it needs to be quite this weird
		for (auto pipeData : instruction_pipe)
		{
			for (auto match : ctre::split<"\n">(instruction_pipe.get_string_view()))
			{
				std::string_view matchingStr = match;
				if (auto [match2, commandResult, valueResult] = ctre::match<"(\\w+)\\s*(.+)?">(matchingStr); match2)
				{
					std::string_view command((std::string_view)commandResult);
					std::string value((std::string_view)valueResult);

					std::vector<std::string_view> cmd_args;
					for (auto arg : ctre::split<"(?<!\") (?!\")">(value))
						if (!arg.to_view().empty())
							cmd_args.emplace_back(arg);

					if (!instructions.parse_command(statusManager, command, cmd_args, args...))
					{
						// TODO output this to status file
						std::cout << "Unknown Command\n";
					}

					pipeData.consume(match.end());
				}
				// TODO Command parsing

				std::cout << matchingStr << "\n";
			}
		}

		instruction_pipe.clear();
	}
};
