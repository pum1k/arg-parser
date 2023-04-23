#ifndef ARGPARSER_ARGPARSER_TPP_
#define ARGPARSER_ARGPARSER_TPP_

#include "argparser.hpp"

namespace argp
{

inline split_options::split_options(const std::vector<OptionBase *> &options)
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

inline std::pair<std::string, std::string> PositionalOptionBase::get_help()
    const
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

inline std::pair<std::string, std::string> KeywordOptionBase::get_help() const
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
inline PositionalOption<T>::PositionalOption(std::string name, std::string help,
                                             bool is_required,
                                             T val /* = T() */)
    : PositionalOptionBase(name, help, is_required), val(val)
{
}

template <class T>
inline int PositionalOption<T>::get_param_count()
{
    return 0;
}

template <class T>
inline bool PositionalOption<T>::matches(std::string_view)
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
    const std::vector<std::string_view> &)
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

namespace impl
{

inline OptionBase *match_option(const char *arg,
                                const std::vector<OptionBase *> &opts)
{
    auto split_opts = split_options(opts);

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

inline void handle_match(int &i, OptionBase *opt, int argc, const char *argv[])
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

inline indented::indented(std::string_view str, size_t width,
                          char fill /* = ' ' */)
    : str(str), width(width), fill(fill)
{
}

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

} // namespace impl

inline std::vector<std::string> parse(int argc, const char *argv[],
                                      const std::vector<OptionBase *> &opts,
                                      int skip_first_n /* = 1 */)
{
    std::vector<std::string> unrecognised;

    for (int i = skip_first_n; i < argc; i++)
    {
        OptionBase *opt = impl::match_option(argv[i], opts);
        if (opt != nullptr)
        {
            impl::handle_match(i, opt, argc, argv);
        }
        else
        {
            unrecognised.push_back(argv[i]);
        }
        continue;
    }

    return unrecognised;
}

inline void print_help(std::ostream &os, std::string_view cmd,
                       const std::vector<OptionBase *> &opts,
                       size_t min_w /* = 25 */)
{
    auto split_opts = split_options(opts);

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

    for (OptionBase *opt : opts)
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
        os << impl::indented(help.second, min_w) << "\n";
    }
    os.flags(flags);
}

} // namespace argp

#endif // ARGPARSER_ARGPARSER_TPP_
