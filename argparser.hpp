/**
 * Header only command line argument parser.
 */

#ifndef ARGPARSER_ARGPARSER_HPP_
#define ARGPARSER_ARGPARSER_HPP_

#include <iomanip>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace argp
{
// DECLARATIONS ================================================================

/**
 * Expected data type for Option.
 *
 * FLAG - bool (no data after it; true if found, false otherwise)
 * STRING - std::string
 * INT - int
 */
enum OptionType
{
    FLAG,
    STRING,
    INT,
};

/**
 * Struct holding data about a single command line option that should be
 * parsed.
 * Use these to initialize ArgParser class
 *
 * name - name of the option
 * identifiers - vector of possible command line flags indicating this option
 * opt_type - type of expected data
 */
struct Option
{
    std::string name;
    std::vector<std::string> identifiers;
    OptionType opt_type;
    std::string help;
};

/**
 * Class for parsing command line options.
 */
class ArgParser
{
 private:
    const std::vector<Option> options_;
    std::unordered_map<std::string, bool> options_flag;
    std::unordered_map<std::string, std::string> options_string;
    std::unordered_map<std::string, int> options_int;
    std::vector<std::string> unrecognised;

    bool default_flag;
    std::string default_string;
    int default_int;

    void set_value(Option opt, std::string value);

 public:
    /**
     * Constructor
     *
     * options - Vector defining options, that should be parsed by the object.
     *   You cannot remove or add any other later.
     */
    ArgParser(const std::vector<Option> &options);
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
     * set_defaults
     *
     * Set values, that should be returned, when the option was not found in
     * the parsed string.
     * These values are also returned, when the option was not set during the
     * initialization of this object.
     *
     * If you don't set these, they will be set to:
     * - flag = false
     * - string = ""
     * - int = -1
     *
     * b - default flag (bool)
     * s - default string
     * i - default int
     */
    void set_defaults(bool b, std::string s, int i);

    /**
     * get_flag
     *
     * Get value associated with the specified option, returning default value,
     * if it wasn't found (more details in set_value).
     * This option must be of type flag (bool).
     *
     * name - name of the option
     */
    bool get_flag(const std::string &name) const;
    /**
     * get_string
     *
     * Get value associated with the specified option, returning default value,
     * if it wasn't found (more details in set_value).
     * This option must be of type string.
     *
     * name - name of the option
     */
    std::string get_string(const std::string &name) const;
    /**
     * get_string
     *
     * Get value associated with the specified option, returning default value,
     * if it wasn't found (more details in set_value).
     * This option must be of type int.
     *
     * name - name of the option
     */
    int get_int(const std::string &name) const;

    /**
     * get_unrecognised
     *
     * Get all unrecognised command line options.
     *
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

using OptionsVector = std::vector<argp::Option>;

// DEFINITIONS =================================================================

inline void ArgParser::set_value(Option opt, std::string value)
{
    switch (opt.opt_type)
    {
    case OptionType::FLAG:
        this->options_flag.insert({opt.name, true});
        break;

    case OptionType::STRING:
        this->options_string.insert({opt.name, value});
        break;

    case OptionType::INT:
        this->options_int.insert({opt.name, std::stoi(value)});
        break;

    default:
        break;
    }
}

inline ArgParser::ArgParser(const std::vector<Option> &options)
    : options_(options)
{
    this->set_defaults(false, "", -1);
}

inline bool ArgParser::parse(int argc, const char *argv[],
                             int skip_first_n /* = 1 */)
{
    std::size_t lenght;
    for (int i = skip_first_n; i < argc; i++)
    {
        lenght = std::char_traits<char>::length(argv[i]);
        for (auto opt : this->options_)
        {
            for (std::string str : opt.identifiers)
            {
                if (str.length() == lenght && str.compare(argv[i]) == 0)
                {
                    if (opt.opt_type == OptionType::FLAG)
                    {
                        this->set_value(opt, "");
                    }
                    else if (i + 1 < argc)
                    {
                        try
                        {
                            this->set_value(opt, argv[++i]);
                        }
                        catch (const std::invalid_argument &e)
                        {
                            std::string msg = "Invalid argument type after: \"";
                            msg += str + "\"";

                            throw std::invalid_argument(msg);
                        }
                    }
                    else
                    {
                        std::string msg = "Missing argument after: \"";
                        msg += str + "\"";

                        throw std::logic_error(msg);
                    }
                    // break out of nested for loop
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

inline void ArgParser::set_defaults(bool b, std::string s, int i)
{
    this->default_flag   = b;
    this->default_string = s;
    this->default_int    = i;
}

inline bool ArgParser::get_flag(const std::string &name) const
{
    auto it = this->options_flag.find(name);
    if (it == this->options_flag.end())
        return this->default_flag;
    else
        return it->second;
}

inline std::string ArgParser::get_string(const std::string &name) const
{
    auto it = this->options_string.find(name);
    if (it == this->options_string.end())
        return this->default_string;
    else
        return it->second;
}

inline int ArgParser::get_int(const std::string &name) const
{
    auto it = this->options_int.find(name);
    if (it == this->options_int.end())
        return this->default_int;
    else
        return it->second;
}

inline const std::vector<std::string> &ArgParser::get_unrecognised() const
{
    return this->unrecognised;
}

inline void ArgParser::print_help(std::ostream &os, int min_w /* = 15 */) const
{
    auto flags = os.flags();
    os << std::left;
    for (Option opt : this->options_)
    {
        auto it  = opt.identifiers.begin();
        auto eit = opt.identifiers.end();
        while (it + 1 != eit)
        {
            os << *it << std::endl;
            ++it;
        }

        os << std::setw(min_w) << ((*it) + " ") << opt.help << std::endl;
    }
    os.flags(flags);
}

} // namespace argp

#endif // ARGPARSER_ARGPARSER_HPP_
