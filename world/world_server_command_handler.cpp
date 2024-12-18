/**
 * EQEmulator: Everquest Server Emulator
 * Copyright (C) 2001-2019 EQEmulator Development Team (https://github.com/EQEmu/Server)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY except by those people which sell it, which
 * are required to give you total support for your newly bought product;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include "../common/global_define.h"
#include "world_server_command_handler.h"
#include "../common/eqemu_logsys.h"
#include "../common/json/json.h"
#include "../common/version.h"
#include "worlddb.h"
#include "../common/database_schema.h"
#include "../common/database/database_dump_service.h"
#include "../common/content/world_content_service.h"
#include "../common/repositories/criteria/content_filter_criteria.h"
#include "../common/rulesys.h"

namespace WorldserverCommandHandler {

	/**
	 * @param argc
	 * @param argv
	 */
	void CommandHandler(int argc, char** argv)
	{
		if (argc == 1) { return; }

		argh::parser cmd;
		cmd.parse(argc, argv, argh::parser::PREFER_PARAM_FOR_UNREG_OPTION);
		EQEmuCommand::DisplayDebug(cmd);

		/**
		 * Declare command mapping
		 */
		auto function_map = EQEmuCommand::function_map;

		/**
		 * Register commands
		 */
		function_map["world:version"] = &WorldserverCommandHandler::Version;
		function_map["database:version"] = &WorldserverCommandHandler::DatabaseVersion;
		function_map["database:schema"] = &WorldserverCommandHandler::DatabaseGetSchema;
		function_map["database:dump"] = &WorldserverCommandHandler::DatabaseDump;
		function_map["test:test"] = &WorldserverCommandHandler::TestCommand;
		function_map["test:expansion"] = &WorldserverCommandHandler::ExpansionTestCommand;

		EQEmuCommand::HandleMenu(function_map, cmd, argc, argv);
	}

	/**
	* @param argc
	 * @param argv
	 * @param cmd
	 * @param description
	 */
	void Version(int argc, char** argv, argh::parser& cmd, std::string& description)
	{
		description = "Shows server version";

		if (cmd[{"-h", "--help"}]) {
			return;
		}

		Json::Value database_version;

		database_version["compile_date"] = COMPILE_DATE;
		database_version["compile_time"] = COMPILE_TIME;
		database_version["database_version"] = CURRENT_BINARY_DATABASE_VERSION;
		database_version["server_version"] = CURRENT_VERSION;

		std::stringstream payload;
		payload << database_version;

		std::cout << payload.str() << std::endl;
	}

	/**
	 * @param argc
	 * @param argv
	 * @param cmd
	 * @param description
	 */
	void DatabaseVersion(int argc, char** argv, argh::parser& cmd, std::string& description)
	{
		description = "Shows database version";

		if (cmd[{"-h", "--help"}]) {
			return;
		}

		Json::Value database_version;

		database_version["database_version"] = CURRENT_BINARY_DATABASE_VERSION;

		std::stringstream payload;
		payload << database_version;

		std::cout << payload.str() << std::endl;
	}

	/**
 * @param argc
 * @param argv
 * @param cmd
 * @param description
 */
	void DatabaseGetSchema(int argc, char** argv, argh::parser& cmd, std::string& description)
	{
		description = "Displays server database schema";


		if (cmd[{"-h", "--help"}]) {
			return;
		}

		Json::Value              player_tables_json;
		std::vector<std::string> player_tables = DatabaseSchema::GetPlayerTables();
		for (const auto& table: player_tables) {
			player_tables_json.append(table);
		}

		Json::Value              content_tables_json;
		std::vector<std::string> content_tables = DatabaseSchema::GetContentTables();
		for (const auto& table: content_tables) {
			content_tables_json.append(table);
		}

		Json::Value              server_tables_json;
		std::vector<std::string> server_tables = DatabaseSchema::GetServerTables();
		for (const auto& table: server_tables) {
			server_tables_json.append(table);
		}

		Json::Value schema;

		schema["content_tables"] = content_tables_json;
		schema["player_tables"] = player_tables_json;
		schema["server_tables"] = server_tables_json;

		std::stringstream payload;
		payload << schema;

		std::cout << payload.str() << std::endl;
	}

	/**
	 * @param argc
	 * @param argv
	 * @param cmd
	 * @param description
	 */
	void DatabaseDump(int argc, char** argv, argh::parser& cmd, std::string& description)
	{
		description = "Dumps server database tables";

		if (cmd[{"-h", "--help"}]) {
			return;
		}

		std::vector<std::string> arguments = {};
		std::vector<std::string> options = {
			"--all",
			"--content-tables",
			//"--login-tables",
			"--player-tables",
			"--state-tables",
			"--system-tables",
			"--query-serv-tables",
			"--table-structure-only",
			"--table-lock",
			"--dump-path=",
			"--dump-output-to-console",
			"--compress"
		};


		if (argc < 3) {
			EQEmuCommand::ValidateCmdInput(arguments, options, cmd, argc, argv);
			return;
		}

		auto database_dump_service = new DatabaseDumpService();
		bool dump_all = cmd[{"-a", "--all"}];

		if (!cmd("--dump-path").str().empty()) {
			database_dump_service->SetDumpPath(cmd("--dump-path").str());
		}

		/**
		 * Set Option
		 */
		database_dump_service->SetDumpContentTables(cmd[{"--content-tables"}] || dump_all);
		//database_dump_service->SetDumpLoginServerTables(cmd[{"--login-tables"}] || dump_all);
		database_dump_service->SetDumpPlayerTables(cmd[{"--player-tables"}] || dump_all);
		database_dump_service->SetDumpStateTables(cmd[{"--state-tables"}] || dump_all);
		database_dump_service->SetDumpSystemTables(cmd[{"--system-tables"}] || dump_all);
		database_dump_service->SetDumpQueryServerTables(cmd[{"--query-serv-tables"}] || dump_all);
		database_dump_service->SetDumpAllTables(dump_all);

		database_dump_service->SetDumpWithNoData(cmd[{"--table-structure-only"}]);
		database_dump_service->SetDumpTableLock(cmd[{"--table-lock"}]);
		database_dump_service->SetDumpWithCompression(cmd[{"--compress"}]);
		database_dump_service->SetDumpOutputToConsole(cmd[{"--dump-output-to-console"}]);

		/**
		 * Dump
		 */
		database_dump_service->Dump();
	}

	/**
 * @param argc
 * @param argv
 * @param cmd
 * @param description
 */
	void TestCommand(int argc, char **argv, argh::parser &cmd, std::string &description)
	{
		description = "Test command";

		if (cmd[{"-h", "--help"}]) {
			return;
		}
	}

	/**
	 * @param argc
	 * @param argv
	 * @param cmd
	 * @param description
	 */
	void ExpansionTestCommand(int argc, char **argv, argh::parser &cmd, std::string &description)
	{
		description = "Expansion test command";

		if (cmd[{"-h", "--help"}]) {
			return;
		}

		if (!RuleManager::Instance()->LoadRules(&database, "default")) {
			LogInfo("No rule set configured, using default rules");
		}

		content_service.SetCurrentExpansion(RuleR(World, CurrentExpansion));

		std::vector<ContentFlagsRepository::ContentFlags> flags = {};
		auto                                              f = ContentFlagsRepository::NewEntity();
		f.enabled = 1;

		std::vector<std::string> flag_names = {
			"hateplane_enabled",
			"patch_nerf_7077",
		};

		for (auto &name : flag_names) {
			f.flag_name = name;
			flags.push_back(f);
		}

		content_service.SetContentFlags(flags);

		LogInfo(
			"Current expansion is [{}] ({}) is Velious Enabled [{}] Criteria [{}]",
			content_service.GetCurrentExpansion(),
			Expansion::ExpansionName[(int)floor(content_service.GetCurrentExpansion())],
			content_service.IsTheScarsOfVeliousEnabled() ? "true" : "false",
			ContentFilterCriteria::apply()
		);
	}


}
