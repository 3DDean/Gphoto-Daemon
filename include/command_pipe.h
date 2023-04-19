#pragma once
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
	read_pipe instruction_pipe;
	buffer buff;

	InstructionSet instructions;
	status_manager statusManager;
	logger log;

	command_pipe(
		InstructionSet instructions,
		std::string_view name,
		std::string_view dir)
		: instructions(std::move(instructions)),
		  instruction_pipe(std::string(dir) + name.data() + ".pipe"),
		  statusManager(std::string(dir) + name.data() + ".status"),
		  log(std::string(dir) + name.data() + ".log")
	{}

	template <typename... ArgsT>
	void process_commands(ArgsT &&...args)
	{
		auto [lock, data] = buff.read();

		std::string_view command_str(data.begin(), data.size());

		for (auto match : ctre::split<"\n">(command_str))
		{
			std::string_view matchingStr = match;
			if (auto [match2, commandResult, valueResult] = ctre::match<"(\\w+)\\s*(.+)?">(matchingStr); match2)
			{
				std::string_view command((std::string_view)commandResult);
				std::string value((std::string_view)valueResult);

				std::vector<std::string_view> cmd_args;
				bool inside = false;

				auto push_back = [&](auto &arg)
				{
					if (!arg.to_view().empty())
						cmd_args.emplace_back(arg);
				};

				auto split_str = [&](auto &str_to_split)
				{
					for (auto arg : ctre::split<"(?<!\")[ ](?!\")">(str_to_split))
						push_back(arg);
				};

				for (auto substr : ctre::split<"\"">(value))
				{
					if (inside)
						push_back(substr);
					else
						split_str(substr);

					inside = !inside;
				}

				if (!instructions.parse_command(statusManager, command, cmd_args, args...))
				{
					std::string_view match_str = match2;
					statusManager.error(std::string("command not find ") + match_str.data());
				}

				data.consume(match.end());
			}

			std::cout << matchingStr << "\n";
		}
		// TODO implement buffer clearing
	}

	buffer *get_buffer() { return &buff; }

	read_pipe *get_pipe() { return &instruction_pipe; }
};
