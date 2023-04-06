/**
 * Header only command line argument parser.
 */

#ifndef ARGPARSER_ARGPARSER_HPP_
#define ARGPARSER_ARGPARSER_HPP_

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace argp
{
// DECLARATIONS ================================================================

/**
 * This class defines interface that all command line options must use.
 *
 * Description of fields:
 * identifiers - vector of strings that should be matched to this option
 * help - string describing the option
 * is_set_ - value indicating, if the default value was overwritten (is set even
 *          if the new value is the same as the default)
 */
class OptionBase
{
 protected:
    std::vector<std::string> identifiers;
    std::string help;
    bool is_set_;

    /**
     * from_string
     *
     * Subclasses overriding this method must parse the options from the list of
     * string views in the parameter. After that, they are expected to be set
     * according to the options passed.
     *
     * First string in the vector is the identifier, that was matched, then
     * follows N strings, as requested by get_param_count method.
     *
     * This method should throw std::invalid_argument exception if the
     * conversion could not be done for any reason.
     */
    virtual void from_string(const std::vector<std::string_view> &strings) = 0;

 public:
    OptionBase(std::vector<std::string> identifiers, std::string help);

    /**
     * parse
     *
     * This method is a public wrapper for from_string method that ensures
     * the is_set_ field is filled after parsing the parameters.
     */
    void parse(const std::vector<std::string_view> &strings);

    /**
     * get_param_count
     *
     * This method is used for requesting the number of parameters required to
     * initialize the option object.
     *
     * return value:
     * - non-negative value - the number of additional parameters required
     * - `-1` - this options requires all remaining arguments
     */
    virtual int get_param_count() = 0;

    const std::vector<std::string> &get_identifiers() const;
    const std::string &get_help() const;
    bool is_set() const;
};

/**
 * This class is a simple option that converts a single string parameter to the
 * type specified in the template parameter. It uses stream extraction operator
 * to make this conversion, if the type does not support this operator defined,
 * you must create a specialization of this template or separate class to parse
 * it.
 *
 * There are also specializations for these types:
 * - std::string - convert the whole parameter to the string
 * - bool - no additional parameters required, false by default, true if found
 */
template <class T>
class Option : public OptionBase
{
 protected:
    T val;
    static const int NUM_OPTS;

    virtual void from_string(
        const std::vector<std::string_view> &strings) override;

 public:
    Option(std::vector<std::string> identifiers, std::string help, T val = T());

    virtual int get_param_count() override;

    T get_val() { return val; }
};

/**
 * Class for parsing command line options.
 */
class ArgParser
{
 private:
    const std::vector<OptionBase *> options_;
    std::vector<std::string> unrecognised;

 public:
    /**
     * Constructor
     *
     * options - Vector defining options, that should be parsed by the object.
     * You cannot remove or add any other later. Lifetime of option object must
     * not end before the lifetime of the ArgParser object, if they do, behavior
     * is undefined.
     */
    ArgParser(const std::vector<OptionBase *> &options);

    /**
     * parse
     *
     * Parse string(*) array and store options specified int this
     * object's constructor.
     *
     *  (*)In context of this function, string means null-terminated char array.
     *
     * argc - number of strings(*) in argv
     * argv - array of strings(*) to parse
     * skip_first_n - Ignore this number of strings at the start of argv.
     *   Default value is set to 1 because C/C++ command line arguments
     *   start with program name.
     *
     * return - true if all arguments were recognised
     *        - false if some arguments were not recognised (these can be
     *          accessed by calling get_unrecognised method)
     */
    bool parse(int argc, const char *argv[], int skip_first_n = 1);

    /**
     * get_unrecognised
     *
     * Get all unrecognised command line options.
     */
    const std::vector<std::string> &get_unrecognised() const;

    /**
     * print_help
     *
     * Output help to a specified output stream.
     *
     * os - output stream
     * min_w - minimal number of characters that will be used to display
     *   identifier of an option. Used for aligning help strings.
     */
    void print_help(std::ostream &os, int min_w = 15) const;
};

// DEFINITIONS =================================================================

inline OptionBase::OptionBase(std::vector<std::string> identifiers,
                              std::string help)
    : identifiers(identifiers), help(help), is_set_(false)
{
}

inline void OptionBase::parse(const std::vector<std::string_view> &strings)
{
    this->from_string(strings);

    is_set_ = true;
}

inline const std::vector<std::string> &OptionBase::get_identifiers() const
{
    return identifiers;
}

inline const std::string &OptionBase::get_help() const { return help; }

inline bool OptionBase::is_set() const { return is_set_; }

inline ArgParser::ArgParser(const std::vector<OptionBase *> &options)
    : options_(options)
{
}

template <class T>
inline constexpr const int Option<T>::NUM_OPTS = 1;

template <>
inline constexpr const int Option<bool>::NUM_OPTS = 0;

template <class T>
inline void Option<T>::from_string(const std::vector<std::string_view> &strings)
{
    std::string str = std::string(strings[1]);
    std::istringstream iss(str);
    iss >> this->val;
    if (!iss)
    {
        throw std::invalid_argument("Could not parse the data.");
    }
}

template <class T>
inline Option<T>::Option(std::vector<std::string> identifiers, std::string help,
                         T val /* = T() */)
    : OptionBase(identifiers, help), val(val)
{
}

template <class T>
inline int Option<T>::get_param_count()
{
    return NUM_OPTS;
}

template <>
inline void Option<std::string>::from_string(
    const std::vector<std::string_view> &strings)
{
    this->val = std::string(strings[1]);
}

template <>
inline void Option<bool>::from_string(
    const std::vector<std::string_view> &strings)
{
    this->val = true;
}

inline bool ArgParser::parse(int argc, const char *argv[],
                             int skip_first_n /* = 1 */)
{
    std::size_t length;
    for (int i = skip_first_n; i < argc; i++)
    {
        for (auto opt : this->options_)
        {
            for (const std::string &str : opt->get_identifiers())
            {
                if (str == argv[i])
                {
                    int param_count = opt->get_param_count();
                    if (param_count < -1)
                    {
                        throw std::invalid_argument(
                            "Invalid number of requsted parameters.");
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

                    goto outer_loop;
                }
            }
        }
        this->unrecognised.push_back(argv[i]);
        continue;
    outer_loop:;
    }

    return this->unrecognised.empty();
}

inline const std::vector<std::string> &ArgParser::get_unrecognised() const
{
    return this->unrecognised;
}

inline void ArgParser::print_help(std::ostream &os, int min_w /* = 15 */) const
{
    auto flags = os.flags();
    os << std::left;
    for (OptionBase *opt : this->options_)
    {
        auto it  = opt->get_identifiers().begin();
        auto eit = opt->get_identifiers().end();
        while (it + 1 != eit)
        {
            os << *it << std::endl;
            ++it;
        }

        os << std::setw(min_w) << ((*it) + " ") << opt->get_help() << std::endl;
    }
    os.flags(flags);
}

} // namespace argp

#endif // ARGPARSER_ARGPARSER_HPP_
