#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include "Server.hpp"

namespace fs = std::filesystem;

struct TestCase
{
	std::string name;
	std::string config_text;
	bool expect_throw;
	std::function<void(const std::vector<Server>&)> verify;
	std::function<void(const fs::path&)> setup;
};

static std::vector<char*> make_envp(std::vector<std::string>& env_storage)
{
	std::vector<char*> envp;
	for (std::string& item : env_storage)
		envp.push_back(item.data());
	envp.push_back(nullptr);
	return envp;
}

static std::vector<std::string> make_env_storage(const fs::path& path_dir)
{
	return {"PATH=" + path_dir.string()};
}

static fs::path write_temp_config(const fs::path& dir, const std::string& name, const std::string& contents)
{
	fs::path file_path = dir / name;
	std::ofstream out(file_path.c_str());
	if (!out.is_open())
		throw std::runtime_error("failed to create temp config: " + file_path.string());
	out << contents;
	return file_path;
}

static std::vector<Server> importconfigfile_test(const char *configfile, char **envp)
{
	std::ifstream config(configfile);
	std::vector<Server> servers;

	if (!config.is_open())
		throw std::runtime_error("File does not exist");
	while (!config.eof())
	{
		std::string line;
		std::getline(config, line);
		if (line.find("server") != line.npos)
			servers.emplace_back(config, envp);
		else if (!line.empty() && line.find("server") == line.npos)
			throw std::runtime_error("unexpected attribute: " + line);
	}
	return servers;
}

static bool run_case(const TestCase& tc, const fs::path& temp_dir)
{
	fs::path config_path = write_temp_config(temp_dir, tc.name + ".conf", tc.config_text);
	if (tc.setup)
		tc.setup(temp_dir);

	const unsigned timeout_ticks = 30;
	pid_t pid = fork();
	if (pid == -1)
		throw std::runtime_error("fork failed");
	if (pid == 0)
	{
		auto env_storage = make_env_storage(temp_dir);
		auto envp = make_envp(env_storage);
		std::string config_path_text = config_path.string();
		std::vector<char> config_path_storage(config_path_text.begin(), config_path_text.end());
		config_path_storage.push_back('\0');

		try
		{
			std::vector<Server> servers = importconfigfile_test(config_path_storage.data(), envp.data());
			if (tc.expect_throw)
				throw std::runtime_error("expected parser failure but parsing succeeded");
			if (tc.verify)
				tc.verify(servers);
			std::cout << "[PASS] " << tc.name << '\n';
			::_exit(0);
		}
		catch (const std::exception& e)
		{
			if (!tc.expect_throw)
			{
				std::cerr << "[FAIL] " << tc.name << " -> exception: " << e.what() << '\n';
				::_exit(1);
			}
			std::cout << "[PASS] " << tc.name << " -> exception: " << e.what() << '\n';
			::_exit(0);
		}
	}

	int status = 0;
	unsigned waited = 0;
	while (waited < timeout_ticks)
	{
		pid_t result = waitpid(pid, &status, WNOHANG);
		if (result == pid)
			break;
		usleep(100000);
		++waited;
	}

	if (waited >= timeout_ticks)
	{
		kill(pid, SIGKILL);
		waitpid(pid, &status, 0);
		std::cout << "[FAIL] " << tc.name << " -> TIMEOUT\n";
		return false;
	}

	if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
		return true;

	std::cout << "[FAIL] " << tc.name << " -> child exited with status "
	          << (WIFEXITED(status) ? WEXITSTATUS(status) : -1) << '\n';
	return false;
}

int main()
{
	const fs::path temp_root = fs::temp_directory_path() / "webserv_config_parser_tests";
	fs::create_directories(temp_root);

	const std::string empty_config = "";
	const std::string whitespace_config = "\n\n\t\n";

	const std::string valid_config =
		"server\n"
		"{\n"
		"\tlisten = any:0\n"
		"\tMaxRequestBodySize = 1024\n"
		"\troute\n"
		"\t{\n"
		"\t\troute = /\n"
		"\t\tmethods = get post\n"
		"\t\troot = /tmp\n"
		"\t\tdefault = index.html\n"
		"\t\tdirectorylisting = true\n"
		"\t}\n"
		"}\n";

	const std::string multi_route_config =
		"server\n"
		"{\n"
		"\tlisten = any:0\n"
		"\tMaxRequestBodySize = 2048\n"
		"\troute\n"
		"\t{\n"
		"\t\troute = /\n"
		"\t\tmethods = get\n"
		"\t\troot = /tmp\n"
		"\t\tdefault = index.html\n"
		"\t\tdirectorylisting = false\n"
		"\t}\n"
		"\troute\n"
		"\t{\n"
		"\t\troute = /upload\n"
		"\t\tmethods = post delete\n"
		"\t\troot = /tmp/uploads\n"
		"\t\tdefault = upload.html\n"
		"\t\tdirectorylisting = true\n"
		"\t}\n"
		"\tcgi = .php fakecgi\n"
		"}\n";

	const std::string unsupported_methods_config =
		"server\n"
		"{\n"
		"\tlisten = any:0\n"
		"\troute\n"
		"\t{\n"
		"\t\troute = /\n"
		"\t\tmethods = put\n"
		"\t\troot = /tmp\n"
		"\t\tdefault = index.html\n"
		"\t\tdirectorylisting = false\n"
		"\t}\n"
		"}\n";

	const std::string cgi_config =
		"server\n"
		"{\n"
		"\tlisten = any:0\n"
		"\troute\n"
		"\t{\n"
		"\t\troute = /\n"
		"\t\tmethods = get\n"
		"\t\troot = /tmp\n"
		"\t\tdefault = index.html\n"
		"\t\tdirectorylisting = false\n"
		"\t}\n"
		"\tcgi = .php fakecgi\n"
		"}\n";

	const std::string missing_cgi_target_config =
		"server\n"
		"{\n"
		"\tlisten = any:0\n"
		"\troute\n"
		"\t{\n"
		"\t\troute = /\n"
		"\t\tmethods = get\n"
		"\t\troot = /tmp\n"
		"\t\tdefault = index.html\n"
		"\t\tdirectorylisting = false\n"
		"\t}\n"
		"\tcgi = .php does-not-exist\n"
		"}\n";

	const std::string invalid_listen_config =
		"server\n"
		"{\n"
		"\tlisten = invalid-host:abc\n"
		"\troute\n"
		"\t{\n"
		"\t\troute = /\n"
		"\t\tmethods = get\n"
		"\t\troot = /tmp\n"
		"\t\tdefault = index.html\n"
		"\t\tdirectorylisting = false\n"
		"\t}\n"
		"}\n";

	const std::string missing_server_open_brace_config =
		"server\n"
		"listen = any:0\n"
		"route\n"
		"{\n"
		"\troute = /\n"
		"\tmethods = get\n"
		"\troot = /tmp\n"
		"\tdefault = index.html\n"
		"\tdirectorylisting = false\n"
		"}\n";

	const std::string missing_server_close_brace_config =
		"server\n"
		"{\n"
		"\tlisten = any:0\n"
		"\troute\n"
		"\t{\n"
		"\t\troute = /\n"
		"\t\tmethods = get\n"
		"\t\troot = /tmp\n"
		"\t\tdefault = index.html\n"
		"\t\tdirectorylisting = false\n"
		"\t}\n";

	const std::string missing_brace_config =
		"server\n"
		"{\n"
		"\tlisten = any:0\n"
		"\troute\n"
		"\t{\n"
		"\t\troute = /\n"
		"\t\tmethods = get\n"
		"\t\troot = /tmp\n"
		"\t\tdefault = index.html\n"
		"\t\tdirectorylisting = false\n";

	const std::string top_level_junk = "banana = 1\n";

	const std::string missing_route_root_config =
		"server\n"
		"{\n"
		"\tlisten = any:0\n"
		"\troute\n"
		"\t{\n"
		"\t\troute = /\n"
		"\t\tmethods = get\n"
		"\t\tdefault = index.html\n"
		"\t\tdirectorylisting = false\n"
		"\t}\n"
		"}\n";

	const std::string missing_route_default_config =
		"server\n"
		"{\n"
		"\tlisten = any:0\n"
		"\troute\n"
		"\t{\n"
		"\t\troute = /\n"
		"\t\tmethods = get\n"
		"\t\troot = /tmp\n"
		"\t\tdirectorylisting = false\n"
		"\t}\n"
		"}\n";

	const std::string directorylisting_case_variant_config =
		"server\n"
		"{\n"
		"\tlisten = any:0\n"
		"\troute\n"
		"\t{\n"
		"\t\troute = /\n"
		"\t\tmethods = get\n"
		"\t\troot = /tmp\n"
		"\t\tdefault = index.html\n"
		"\t\tdirectoryListing = false\n"
		"\t}\n"
		"}\n";

	const std::string unknown_server_directive_config =
		"server\n"
		"{\n"
		"\tlisten = any:0\n"
		"\tbanana = split\n"
		"\troute\n"
		"\t{\n"
		"\t\troute = /\n"
		"\t\tmethods = get\n"
		"\t\troot = /tmp\n"
		"\t\tdefault = index.html\n"
		"\t\tdirectorylisting = false\n"
		"\t}\n"
		"}\n";

	const std::string route_with_upload_config =
		"server\n"
		"{\n"
		"\tlisten = any:0\n"
		"\troute\n"
		"\t{\n"
		"\t\troute = /\n"
		"\t\tmethods = get post\n"
		"\t\troot = /tmp\n"
		"\t\tdefault = index.html\n"
		"\t\tdirectorylisting = true\n"
		"\t\tupload = /tmp/uploads\n"
		"\t}\n"
		"}\n";

	const std::string custom_error_pages_config =
		"server\n"
		"{\n"
		"\tlisten = any:0\n"
		"\t403 = /errors/forbidden.html\n"
		"\t404 = /errors/notfound.html\n"
		"\t500 = /errors/server.html\n"
		"\troute\n"
		"\t{\n"
		"\t\troute = /\n"
		"\t\tmethods = get\n"
		"\t\troot = /tmp\n"
		"\t\tdefault = index.html\n"
		"\t\tdirectorylisting = false\n"
		"\t}\n"
		"}\n";

	const std::string dual_listen_config =
		"server\n"
		"{\n"
		"\tlisten = any:0\n"
		"\tlisten = any:0\n"
		"\troute\n"
		"\t{\n"
		"\t\troute = /\n"
		"\t\tmethods = get\n"
		"\t\troot = /tmp\n"
		"\t\tdefault = index.html\n"
		"\t\tdirectorylisting = false\n"
		"\t}\n"
		"}\n";

	const std::vector<TestCase> tests = {
		{"empty config", empty_config, false,
			[](const std::vector<Server>& servers)
			{
				if (!servers.empty())
					throw std::runtime_error("expected no servers from empty config");
			}, nullptr},
		{"whitespace-only config", whitespace_config, true,
			[](const std::vector<Server>& servers)
			{
				if (!servers.empty())
					throw std::runtime_error("expected no servers from whitespace-only config");
			}, nullptr},
		{"valid basic server", valid_config, false,
			[](const std::vector<Server>& servers)
			{
				if (servers.size() != 1)
					throw std::runtime_error("expected exactly one server");
				if (servers[0].routes.size() != 1)
					throw std::runtime_error("expected exactly one route");
				if (servers[0].routes[0].route != "/")
					throw std::runtime_error("route mismatch");
				if (servers[0].routes[0].default_dir_file != "index.html")
					throw std::runtime_error("default file mismatch");
				if (servers[0].routes[0].http_methods.size() != 2)
					throw std::runtime_error("expected GET and POST methods");
				if (servers[0].MaxRequestBodySize != 1024)
					throw std::runtime_error("body limit mismatch");
			}, nullptr},
		{"valid multi-route server", multi_route_config, false,
			[](const std::vector<Server>& servers)
			{
				if (servers.size() != 1)
					throw std::runtime_error("expected exactly one server");
				if (servers[0].routes.size() != 2)
					throw std::runtime_error("expected two routes");
				if (servers[0].routes[1].route != "/upload")
					throw std::runtime_error("second route mismatch");
				if (servers[0].routes[1].http_methods.size() != 2)
					throw std::runtime_error("expected POST and DELETE on second route");
				if (servers[0].cgiconfigs.find(".php") == servers[0].cgiconfigs.end())
					throw std::runtime_error("expected .php CGI mapping");
			},
			[](const fs::path& temp_dir)
			{
				std::ofstream fake((temp_dir / "fakecgi").c_str());
				fake << "#!/bin/sh\nexit 0\n";
			}},
		{"invalid methods token rejects parse", unsupported_methods_config, true, nullptr, nullptr},
		{"cgi lookup via PATH", cgi_config, false,
			[](const std::vector<Server>& servers)
			{
				if (servers.size() != 1)
					throw std::runtime_error("expected exactly one server");
				auto it = servers[0].cgiconfigs.find(".php");
				if (it == servers[0].cgiconfigs.end())
					throw std::runtime_error("expected .php CGI mapping");
				if (it->second.find("fakecgi") == std::string::npos)
					throw std::runtime_error("cgi path mismatch");
			},
			[](const fs::path& temp_dir)
			{
				std::ofstream fake((temp_dir / "fakecgi").c_str());
				fake << "#!/bin/sh\nexit 0\n";
			}},
		{"route upload_dir stored", route_with_upload_config, false,
			[](const std::vector<Server>& servers)
			{
				if (servers.size() != 1 || servers[0].routes.size() != 1)
					throw std::runtime_error("expected one server one route");
				if (servers[0].routes[0].upload_dir != "/tmp/uploads")
					throw std::runtime_error("upload_dir mismatch");
			}, nullptr},
		{"custom error pages map", custom_error_pages_config, false,
			[](const std::vector<Server>& servers)
			{
				if (servers.size() != 1)
					throw std::runtime_error("expected one server");
				const auto &paths = servers[0].error_page_paths;
				auto i403 = paths.find(403);
				auto i404 = paths.find(404);
				auto i500 = paths.find(500);
				if (i403 == paths.end() || i403->second != "/errors/forbidden.html")
					throw std::runtime_error("error_page_paths 403 mismatch");
				if (i404 == paths.end() || i404->second != "/errors/notfound.html")
					throw std::runtime_error("error_page_paths 404 mismatch");
				if (i500 == paths.end() || i500->second != "/errors/server.html")
					throw std::runtime_error("error_page_paths 500 mismatch");
			}, nullptr},
		{"dual listen specs first wins sock", dual_listen_config, false,
			[](const std::vector<Server>& servers)
			{
				if (servers.size() != 1)
					throw std::runtime_error("expected one server");
				if (servers[0].listen_specs.size() != 2)
					throw std::runtime_error("expected two listen_specs");
				if (servers[0].sock.info.first != servers[0].listen_specs[0].first)
					throw std::runtime_error("sock port should match first listen");
				if (servers[0].sock.info.second != servers[0].listen_specs[0].second)
					throw std::runtime_error("sock host should match first listen");
			}, nullptr},
		{"missing CGI target", missing_cgi_target_config, true, nullptr, nullptr},
		{"invalid listen syntax", invalid_listen_config, true, nullptr, nullptr},
		{"missing server opening brace", missing_server_open_brace_config, true, nullptr, nullptr},
		{"route missing root", missing_route_root_config, true, nullptr, nullptr},
		{"route missing default", missing_route_default_config, true, nullptr, nullptr},
		{"route camelCase directorylisting", directorylisting_case_variant_config, true, nullptr, nullptr},
		{"missing closing brace", missing_brace_config, true, nullptr, nullptr},
		{"missing server closing brace", missing_server_close_brace_config, true, nullptr, nullptr},
		{"unexpected top-level text", top_level_junk, true, nullptr, nullptr},
		{"unknown server directive", unknown_server_directive_config, true, nullptr, nullptr}
	};

	int pass = 0;
	for (const TestCase& tc : tests)
	{
		if (run_case(tc, temp_root))
			++pass;
	}

	std::cout << "\nSummary: " << pass << " / " << tests.size() << " passed\n";
	fs::remove_all(temp_root);
	return pass == static_cast<int>(tests.size()) ? 0 : 1;
}