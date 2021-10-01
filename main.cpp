#include <iostream>
#include <algorithm>
#include <string>
#include <map>
#include <vector>

namespace Lexer
{
    enum Tag
    {
        BEGIN,
        END,
        SEMI,
        INTEGER,
        STRING,
        LSB,
        RSB,
        COMMA,
        COLON,
        END_LINE
    };
    std::map<Tag, std::string> tag_to_string{
        {BEGIN, "{"}, {END, "}"}, {LSB, "["}, {RSB, "]"}, {COMMA, ","}, {COLON, ":"}};

    std::map<std::string, Tag> string_to_tag{
        {"{", BEGIN}, {"}", END}, {"[", LSB}, {"]", RSB}, {",", COMMA}, {":", COLON}};

    class Token
    {
    public:
        Token(Tag _tag) : tag(_tag){};
        virtual ~Token(){};
        virtual std::string to_string() const
        {
            if (tag_to_string.count(tag))
                return tag_to_string[tag];
            else
                throw std::runtime_error("Token::get_tag() unknown tag");
        }

        Tag get_tag() const
        {
            return tag;
        }

        void print() const
        {
            std::cout << to_string();
        }

    protected:
        Tag tag;
    };

    class StringToken final : public Token
    {
    public:
        StringToken(const std::string &str) : Token(STRING), value(str) {}
        std::string to_string() const override
        {
            return "<string:" + value + ">";
        }

    private:
        std::string value;
    };

    class Integer final : public Token
    {
    public:
        Integer(int64_t v) : Token(INTEGER), value(v) {}
        std::string to_string() const override
        {
            return "<integer:" + std::to_string(value) + ">";
        }

    private:
        int64_t value;
    };

    class TokenStream
    {
    public:
        TokenStream() = default;
        ~TokenStream()
        {
            for (auto a : tokens)
                delete a;
        }
        void push(Token *tok) { tokens.push_back(tok); }
        Token *current()
        {
            if (cur_p >= tokens.size())
                throw std::runtime_error("TokenStrem::current out of range");
            return tokens[cur_p];
        }

        void move_to_next()
        {
            cur_p++;
        }
        void match(Tag tag)
        {
            auto cur = current();
            if (cur->get_tag() == tag)
            {
                cur_p++;
                return;
            }
            throw std::runtime_error("TokenStream::match syntax error! token not mathed");
        }

        void print(){
            for (auto tok: tokens)
            {
                tok->print();
            }
            
        }

    private:
        std::vector<Token *> tokens;
        int cur_p;
    };

    class EndLine : public Token
    {
    public:
        EndLine() : Token(END_LINE){};
        std::string to_string() const override
        {
            return "\n";
        }
    };

    TokenStream build_token_stream(const std::string &str)
    {
        TokenStream token_stream;
        for (int i = 0; i < str.size(); i++)
        {
            char ch = str[i];
            if (isdigit(ch))
            {
                long long v = ch - '0';
                i++;
                while (i < str.size() && isdigit(str[i]))
                {
                    v *= 10;
                    v += str[i] - '0';
                    i++;
                }
                token_stream.push(new Integer(v));
                i--;
                continue;
            }

            if (ch == '\"')
            {
                std::string v;
                i++;
                while (i < str.size() && str[i] != '\"')
                {
                    if (str[i] == '\\')
                    {
                        if (i + 1 > str.size())
                            throw std::runtime_error("build_token_strema: invalid string");
                        i++;
                        switch (str[i])
                        {
                        case 'r':
                            v += '\r';
                            break;
                        case 'n':
                            v += '\n';
                            break;
                        case 't':
                            v += '\t';
                            break;
                        case '\\':
                        case '\"':
                        case '\'':
                            v += str[i];
                            break;
                        default:
                            throw std::runtime_error("build_token_strema: invalid string");
                        }
                    }
                    else
                        v += str[i];
                    i++;
                }
                token_stream.push(new StringToken(v));
                continue;
            }

            switch (ch)
            {
            case '[':
            case ']':
            case '{':
            case '}':
            case ':':
            case ',':
                token_stream.push(new Token(string_to_tag[std::string(1, ch)]));
                break;
            case '\r':
            case '\n':
                token_stream.push(new EndLine());
                break;
            default:
                break;
            }
        }
        // token_stream.push(new Token(END_TAG));
        return token_stream;
    }

}

int main(int argc, char const *argv[])
{
    /* code */
    using namespace std;
    using namespace Lexer;
    auto ts = build_token_stream("{1\"hey\ng\tgg\"}");
    ts.print();
    return 0;
}
