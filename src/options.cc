#include "options.hh"

using namespace focusstack;

Options::Options(int argc, const char *argv[])
{
  for (int i = 1; i < argc; i++)
  {
    m_options.emplace_back(argv[i]);
  }

  m_parsed.resize(m_options.size(), false);
}

bool Options::has_flag(std::string name)
{
  for (int i = 0; i < m_options.size(); i++)
  {
    const std::string &arg = m_options.at(i);
    if (arg == name)
    {
      return true;
    }

    size_t pos = arg.find('=');
    if (pos != std::string::npos)
    {
      if (arg.substr(0, pos) == name)
      {
        m_parsed.at(i) = true;
        return true;
      }
    }
  }

  return false;
}

std::string Options::get_arg(std::string name, std::string defval)
{
  for (int i = 0; i < m_options.size(); i++)
  {
    const std::string &arg = m_options.at(i);
    size_t pos = arg.find('=');
    if (pos != std::string::npos)
    {
      if (arg.substr(0, pos) == name)
      {
        m_parsed.at(i) = true;
        return arg.substr(pos + 1, arg.size() - (pos + 1));
      }
    }
  }

  return defval;
}

std::vector<std::string> Options::get_filenames()
{
  std::vector<std::string> result;

  for (int i = 0; i < m_options.size(); i++)
  {
    const std::string &arg = m_options.at(i);
    if (arg.substr(0, 2) != "--")
    {
      result.push_back(arg);
      m_parsed.at(i) = true;
    }
  }

  return result;
}

std::vector<std::string> Options::get_unparsed()
{
  std::vector<std::string> result;

  for (int i = 0; i < m_options.size(); i++)
  {
    if (!m_parsed.at(i))
    {
      result.push_back(m_options.at(i));
    }
  }

  return result;
}

