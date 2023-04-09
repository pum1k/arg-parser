/**
 * Header only command line argument parser.
 */

#ifndef ARGPARSER_ARGPARSER_HPP_
#define ARGPARSER_ARGPARSER_HPP_

#include <algorithm>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace argp
{
struct split_options;
class OptionBase;
class PositionalOptionBase;
class KeywordOptionBase;
template <class T>
class PositionalOption;
template <class T>
class KeywordOption;

struct split_options
{
    enum Type
    {
        POSITIONAL,
        KEYWORD
    };

    std::vector<PositionalOptionBase *> positional;
    std::vector<KeywordOptionBase *> keyword;

    split_options(const std::vector<OptionBase *> &options);
};

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
    OptionBase();
    virtual ~OptionBase(){};

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

    /**
     * matches
     *
     * This method tells the parser, if the option matches the identifier from
     * command line. Returning false continues search for matching option. If
     * the methods returns true, the parse method of the object is called with
     * the right argument count.
     */
    virtual bool matches(std::string_view identifier) = 0;

    /**
     * get_help
     *
     * Returns two strings -- first should be the name / identifiers of the
     * option, second should be a brief description.
     */
    virtual std::pair<std::string, std::string> get_help() const = 0;

    virtual split_options::Type get_type() const = 0;

    bool is_set() const;
};

/**
 * Base class for defining positional arguments.
 *
 * When parsing, all keyword arguments (returning KEYWORD from get_type) are
 * tried before calling matches on any subclass of this class.
 */
class PositionalOptionBase : public OptionBase
{
 protected:
    std::string name;
    std::string help;
    bool is_required;

 public:
    PositionalOptionBase(std::string name, std::string help, bool is_required);

    virtual std::pair<std::string, std::string> get_help() const override;
    virtual split_options::Type get_type() const override;
};

/**
 * Base class for defining keyword arguments.
 */
class KeywordOptionBase : public OptionBase
{
 protected:
    std::vector<std::string> identifiers;
    std::string help;

 public:
    KeywordOptionBase(std::vector<std::string> identifiers, std::string help);

    virtual std::pair<std::string, std::string> get_help() const override;
    virtual split_options::Type get_type() const override;
};

/**
 * This class declares a simple positional argument. It tries to parse the first
 * argument that it is given.
 *
 * It converts a single string parameter to the type specified in the template
 * parameter. It uses stream extraction operator to make this conversion, if the
 * type does not support this operator, you must create either it,
 * a specialization of this template or separate class to parse it.
 *
 * There are also specializations for these types:
 * - std::string - convert the whole parameter to the string
 */
template <class T>
class PositionalOption : public PositionalOptionBase
{
 protected:
    T val;

    virtual void from_string(
        const std::vector<std::string_view> &strings) override;

 public:
    PositionalOption(std::string name, std::string help, bool is_required,
                     T val = T());

    virtual int get_param_count() override;
    virtual bool matches(std::string_view identifier) override;

    T get_val() const;
};

/**
 * This class declares a simple keyword argument. It matches arguments by
 * comparing the command line arguments with the values stored in identifiers
 * field.
 *
 * It converts a single string parameter to the type specified in the template
 * parameter. It uses stream extraction operator to make this conversion, if the
 * type does not support this operator, you must create either it,
 * a specialization of this template or separate class to parse it.
 *
 * There are also specializations for these types:
 * - std::string - convert the whole parameter to the string
 * - bool - no additional parameters required, false by default, true if found
 */
template <class T>
class KeywordOption : public KeywordOptionBase
{
 protected:
    T val;
    static const int NUM_OPTS;

    virtual void from_string(
        const std::vector<std::string_view> &strings) override;

 public:
    KeywordOption(std::vector<std::string> identifiers, std::string help,
                  T val = T());

    virtual int get_param_count() override;
    virtual bool matches(std::string_view identifier) override;

    T get_val() const;
};

namespace impl
{

/**
 * match_option
 *
 * Internal function for finding if any option matches the curent argument
 * parsed.
 *
 * return value:
 * - matching option or nullptr if no option is matched
 */
OptionBase *match_option(const char *arg,
                         const std::vector<OptionBase *> &opts);

/**
 * handle_match
 *
 * Internal function that calls option's parse method with the right number
 * of arguments when it is found.
 */
void handle_match(int &i, OptionBase *opt, int argc, const char *argv[]);

class indented
{
 private:
    std::string_view str;
    int width;
    char fill;

 public:
    indented(std::string_view str, int width, char fill = ' ');

    friend std::ostream &operator<<(std::ostream &os, const indented &val);
};

} // namespace impl

/**
 * parse
 *
 * Parse string array according to the options given in the argument.
 *
 * argc - number of c-style strings in argv
 * argv - array of c-style strings to parse
 *
 * opts - vector of options to be parsed from the input
 *
 * skip_first_n - Ignore this number of strings at the start of argv.
 *   Default value is set to 1 because C/C++ command line arguments
 *   start with program name.
 *
 * return - true if all arguments were recognised
 *        - false if some arguments were not recognised (these can be
 *          accessed by calling get_unrecognised method)
 */
std::vector<std::string> parse(int argc, const char *argv[],
                               const std::vector<OptionBase *> &opts,
                               int skip_first_n = 1);

/**
 * print_help
 *
 * Output help to a specified output stream.
 *
 * os - output stream
 * cmd - command to be printed in the first line (`Usage: <cmd> <params>`)
 * opts - options for which the help should be printed
 * min_w - minimal number of characters that will be used to display
 *   identifier of an option. Used for aligning help strings.
 */
void print_help(std::ostream &os, std::string_view cmd,
                const std::vector<OptionBase *> &opts, int min_w = 25);

} // namespace argp

#include "argparser.tpp"

#endif // ARGPARSER_ARGPARSER_HPP_
