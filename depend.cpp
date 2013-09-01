/*
 * Copyright (C) 2013 Daniel Pfeifer <daniel@pfeifer-mail.de>
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See accompanying file LICENSE_1_0.txt or copy at
 *   http://www.boost.org/LICENSE_1_0.txt
 */

#include <string>
#include <fstream>
#include <sstream>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/regex.hpp>
#include <boost/unordered_map.hpp>

namespace fs = boost::filesystem;
using header_map = boost::unordered_map<fs::path, fs::path>;

std::string alias(fs::path const& path)
{
	std::string result = path.string();
	boost::replace_first(result, "libs/", "boost::");
	boost::replace_all(result, "/", "::");
	return result;
}

std::string target(fs::path const& path)
{
	std::string result = path.string();
	boost::replace_first(result, "libs/", "boost_");
	boost::replace_first(result, "tools/", "");
	boost::replace_all(result, "/", "_");
	return result;
}

fs::path remove_from_start(fs::path const& inPath, fs::path const& remPath)
{
	fs::path result;

	auto itIn = inPath.begin();
	auto itRem = remPath.begin();

	while ((itIn != inPath.end()) && (itRem != remPath.end()) && (*itIn == *itRem))
	{
		++itIn;
		++itRem;
	}

	for (; itIn != inPath.end(); ++itIn)
	{
		result /= *itIn;
	}

	return result;
}

std::string load_file(fs::path const& filename)
{
	fs::ifstream file(filename);
	std::ostringstream content;
	content << file.rdbuf();
	return content.str();
}

std::vector<fs::path> load_projects()
{
	std::vector<fs::path> result;

	auto const text = load_file(".gitmodules");
	boost::regex const submodule_path("path\\s*=\\s*([^\\s]+)");
	boost::sregex_iterator m1(text.begin(), text.end(), submodule_path), m2;

	std::for_each(m1, m2,
		[&result](boost::match_results<std::string::const_iterator> const& what)
		{
			result.emplace_back(what[1]);
		});

	std::sort(std::begin(result), std::end(result));

	return result;
}

std::vector<fs::path> parse_includes(fs::path const& filename)
{
	std::vector<fs::path> result;

	auto const text = load_file(filename);

	boost::regex expr("\n#include\\s+[\"<](boost/[^\">]*)[\">]");
	boost::sregex_iterator m1(text.begin(), text.end(), expr), m2;

	std::for_each(m1, m2,
		[&result](boost::match_results<std::string::const_iterator> const& what)
		{
			result.emplace_back(what[1]);
		});

	return result;
}

header_map load_map(std::vector<fs::path> const& projects)
{
	header_map result;

	for (auto& proj : projects)
	{
		auto const include_dir = proj / "include";
		if (!fs::is_directory(include_dir))
		{
			continue;
		}

		fs::recursive_directory_iterator m1(include_dir), m2;
		std::for_each(m1, m2,
			[&](fs::directory_entry const& entry)
			{
				if (fs::is_directory(entry))
				{
					return;
				}

				auto const header = remove_from_start(entry.path(), include_dir);
				result[header] = proj;
			});
	}

	return result;
}

void analyze(fs::path const& proj, header_map const& map)
{
	auto const include_dir = proj / "include";
//	if (!fs::is_directory(include_dir))
//	{
//		return;
//	}

	std::set<fs::path> depends_public;
	std::set<fs::path> depends_private;

	fs::recursive_directory_iterator m1(proj), m2;
	std::for_each(m1, m2,
		[&](fs::directory_entry const& entry)
		{
			if (fs::is_directory(entry))
			{
				return;
			}

			std::set<fs::path>& depends
				= boost::starts_with(entry.path(), include_dir)
				? depends_public : depends_private;


			for (auto& file : parse_includes(entry.path()))
			{
				try
				{
					depends.insert(map.at(file));
				}
				catch (std::out_of_range&)
				{
				}
			}
		});

	depends_public.erase(proj);
	depends_private.erase(proj);

	if (!depends_public.empty())
	{
		std::cout << "target_link_libraries(" << target(proj) << " PUBLIC\n";
		for (auto const& d : depends_public)
		{
			std::cout << "  " << alias(d) << std::endl;
		}
		std::cout << "  )\n" << std::endl;
	}

	std::set<fs::path> depends_priv;
	std::set_difference(depends_private.begin(), depends_private.end(),
			depends_public.begin(), depends_public.end(),
			std::inserter(depends_priv, depends_priv.begin()));

	if (!depends_priv.empty())
	{
		std::cout << "target_link_libraries(" << target(proj) << " PRIVATE\n";
		for (auto const& d : depends_priv)
		{
			std::cout << "  " << alias(d) << std::endl;
		}
		std::cout << "  )\n" << std::endl;
	}
}

int main(int argc, char* argv[])
{
	auto const projects = load_projects();
	auto const map = load_map(projects);

	for (auto& proj : projects)
	{
		analyze(proj, map);
	}

	return 0;
}
