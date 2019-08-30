#include "options.hh"

using namespace focusstack;

Options::Options(int argc, const char *argv[])
{
  for (int i = 1; i < argc; i++)
  {
    m_options.emplace_back(argv[i]);
  }
}

bool Options::has_flag(std::string name) const
{
  for (const std::string &arg: m_options)
  {
    if (arg == name)
    {
      return true;
    }

    size_t pos = arg.find('=');
    if (pos != std::string::npos)
    {
      if (arg.substr(0, pos) == name)
      {
        return true;
      }
    }
  }

  return false;
}

std::string Options::get_arg(std::string name, std::string defval) const
{
  for (const std::string &arg: m_options)
  {
    size_t pos = arg.find('=');
    if (pos != std::string::npos)
    {
      if (arg.substr(0, pos) == name)
      {
        return arg.substr(pos + 1, arg.size() - (pos + 1));
      }
    }
  }

  return defval;
}

std::vector<std::string> Options::get_filenames() const
{
  std::vector<std::string> result;

  for (const std::string &arg: m_options)
  {
    if (arg.substr(0, 2) != "--")
    {
      result.push_back(arg);
    }
  }

  return result;
}
