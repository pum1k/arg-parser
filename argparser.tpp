#ifndef ARGPARSER_ARGPARSER_TPP_
#define ARGPARSER_ARGPARSER_TPP_

#include "argparser.hpp"

namespace argp
{

split_options::split_options(const std::vector<OptionBase *> &options)
{
    for (auto opt : options)
    {
        switch (opt->get_type())
        {
        case split_options::Type::POSITIONAL:
            this->positional.push_back(
                static_cast<PositionalOptionBase *>(opt));
            break;

        case split_options::Type::KEYWORD:
            this->keyword.push_back(static_cast<KeywordOptionBase *>(opt));
            break;
        }
    }
}

inline OptionBase::OptionBase() : is_set_(false) {}

inline void OptionBase::parse(const std::vector<std::string_view> &strings)
{
    this->from_string(strings);

    is_set_ = true;
}

inline bool OptionBase::is_set() const { return is_set_; }

inline PositionalOptionBase::PositionalOptionBase(std::string name,
                                                  std::string help,
                                                  bool is_required)
    : name(name), help(help), is_required(is_required)
{
}

std::pair<std::string, std::string> PositionalOptionBase::get_help() const
{
    return {(this->is_required) ? this->name : "[" + this->name + "]",
            this->help};
}

inline split_options::Type PositionalOptionBase::get_type() const
{
    return split_options::Type::POSITIONAL;
}

inline KeywordOptionBase::KeywordOptionBase(
    std::vector<std::string> identifiers, std::string help)
    : identifiers(identifiers), help(help)
{
}

std::pair<std::string, std::string> KeywordOptionBase::get_help() const
{
    int length = std::accumulate(
        this->identifiers.begin(), this->identifiers.end(), 0,
        [](int length, std::string str) { return length + str.length() + 2; });

    std::string tmp;
    tmp.reserve(length);

    bool is_first = true;
    for (auto str : this->identifiers)
    {
        if (is_first)
        {
            tmp += str;
            is_first = false;
        }
        else
        {
            tmp += ", ";
            tmp += str;
        }
    }
    return {std::move(tmp), this->help};
}

inline split_options::Type KeywordOptionBase::get_type() const
{
    return split_options::Type::KEYWORD;
}

template <class T>
inline void PositionalOption<T>::from_string(
    const std::vector<std::string_view> &strings)
{
    std::string str = std::string(strings[0]);
    std::istringstream iss(str);
    iss >> this->val;
    if (!iss)
    {
        throw std::invalid_argument("Could not parse the data.");
    }
}

template <>
inline void PositionalOption<std::string>::from_string(
    const std::vector<std::string_view> &strings)
{
    this->val = std::string(strings[0]);
}

template <class T>
PositionalOption<T>::PositionalOption(std::string name, std::string help,
                                      bool is_required, T val /* = T() */)
    : PositionalOptionBase(name, help, is_required), val(val)
{
}

template <class T>
inline int PositionalOption<T>::get_param_count()
{
    return 0;
}

template <class T>
inline bool PositionalOption<T>::matches(std::string_view identifier)
{
    return !this->is_set();
}

template <class T>
inline T PositionalOption<T>::get_val() const
{
    return this->val;
}

template <class T>
inline constexpr const int KeywordOption<T>::NUM_OPTS = 1;

template <>
inline constexpr const int KeywordOption<bool>::NUM_OPTS = 0;

template <class T>
inline void KeywordOption<T>::from_string(
    const std::vector<std::string_view> &strings)
{
    std::string str = std::string(strings[1]);
    std::istringstream iss(str);
    iss >> this->val;
    if (!iss)
    {
        throw std::invalid_argument("Could not parse the data.");
    }
}

template <>
inline void KeywordOption<std::string>::from_string(
    const std::vector<std::string_view> &strings)
{
    this->val = std::string(strings[1]);
}

template <>
inline void KeywordOption<bool>::from_string(
    const std::vector<std::string_view> &strings)
{
    this->val = true;
}

template <class T>
inline KeywordOption<T>::KeywordOption(std::vector<std::string> identifiers,
                                       std::string help, T val /* = T() */)
    : KeywordOptionBase(identifiers, help), val(val)
{
}

template <class T>
inline int KeywordOption<T>::get_param_count()
{
    return NUM_OPTS;
}

template <class T>
inline bool KeywordOption<T>::matches(std::string_view identifier)
{
    for (auto id : this->identifiers)
    {
        if (id == identifier)
        {
            return true;
        }
    }

    return false;
}

template <typename T>
inline T KeywordOption<T>::get_val() const
{
    return val;
}

inline OptionBase *ArgParser::match_option(const char *arg)
{
    auto split_opts = split_options(this->options_);

    for (auto opt : split_opts.keyword)
    {
        if (opt->matches(arg))
        {
            return opt;
        }
    }

    for (auto opt : split_opts.positional)
    {
        if (opt->matches(arg))
        {
            return opt;
        }
    }

    return nullptr;
}

inline void ArgParser::handle_match(int &i, OptionBase *opt, int argc,
                                    const char *argv[])
{
    int param_count = opt->get_param_count();
    if (param_count < -1)
    {
        throw std::invalid_argument("Invalid number of requested parameters.");
    }

    if (param_count == -1)
    {
        param_count = argc - i - 1;
    }

    if (i + param_count >= argc)
    {
        throw std::out_of_range("Not enough arguments.");
    }

    std::vector<std::string_view> opts;
    opts.reserve(param_count + 1);

    for (int end = i + param_count + 1; i < end; i++)
    {
        opts.push_back(std::string_view(argv[i]));
    }
    --i;

    opt->parse(opts);
}

inline ArgParser::ArgParser(const std::vector<OptionBase *> &options)
    : options_(options)
{
}

inline bool ArgParser::parse(int argc, const char *argv[],
                             int skip_first_n /* = 1 */)
{
    for (int i = skip_first_n; i < argc; i++)
    {
        OptionBase *opt = this->match_option(argv[i]);
        if (opt != nullptr)
        {
            this->handle_match(i, opt, argc, argv);
        }
        else
        {
            this->unrecognised.push_back(argv[i]);
        }
        continue;
    }

    return this->unrecognised.empty();
}

inline const std::vector<std::string> &ArgParser::get_unrecognised() const
{
    return this->unrecognised;
}

class indented
{
 private:
    std::string_view str;
    int width;
    char fill;

 public:
    indented(std::string_view str, int width, char fill = ' ')
        : str(str), width(width), fill(fill)
    {
    }

    friend std::ostream &operator<<(std::ostream &os, const indented &val);
};

inline std::ostream &operator<<(std::ostream &os, const indented &val)
{
    for (auto c : val.str)
    {
        os << c;
        if (c == '\n' || c == '\r')
        {
            for (size_t i = 0; i < val.width; i++)
            {
                os << val.fill;
            }
        }
    }

    return os;
}

inline void ArgParser::print_help(std::ostream &os, std::string_view cmd,
                                  int min_w /* = 25 */) const
{
    // std::vector<PositionalOptionBase *> pos_opts;
    // for (auto opt : this->options_)
    // {
    //     PositionalOptionBase *pos_opt =
    //         dynamic_cast<PositionalOptionBase *>(opt);
    //     if (pos_opt != nullptr)
    //     {
    //         pos_opts.push_back(pos_opt);
    //     }
    // }
    auto split_opts = split_options(this->options_);

    auto flags = os.flags();
    os << std::left;

    os << "Usage: " << cmd;
    if (split_opts.keyword.size() > 0)
    {
        os << " <options>";
    }

    for (auto pos_opt : split_opts.positional)
    {
        os << " " << pos_opt->get_help().first;
    }
    os << "\nOptions:\n";

    for (OptionBase *opt : this->options_)
    {
        std::pair<std::string, std::string> &&help = opt->get_help();

        if (help.first.length() <= min_w - 2)
        {
            os << std::setw(min_w - 2) << help.first << "  ";
        }
        else
        {
            os << help.first << "\n" << std::setw(min_w) << "";
        }
        os << indented(help.second, min_w) << "\n";
    }
    os.flags(flags);
}

} // namespace argp

#endif // ARGPARSER_ARGPARSER_TPP_
