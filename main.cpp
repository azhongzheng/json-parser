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
        END_LINE,
        END_TAG
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
        static std::string get_content(Token *tok)
        {
            return static_cast<StringToken *>(tok)->value;
        }
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
        static int64_t get_content(Token *tok)
        {
            return static_cast<Integer *>(tok)->value;
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

        Tag get_cur_tag()
        {
            return current()->get_tag();
        }

        void print()
        {
            for (auto tok : tokens)
            {
                tok->print();
            }
        }
        size_t size() const
        {
            return tokens.size();
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
            //{133\"hey\ng\tgg\"}
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
        token_stream.push(new Token(END_TAG));
        return token_stream;
    }

}

namespace
{

}

namespace Parser
{

    enum NodeType
    {
        UNIT,
        ARRAY,
        GROUP,
        SINGLE
    };

    class Node
    {
    public:
        // virtual JSON *gen_json() = 0;

    private:
    };

    class Unit : public Node
    {
    public:
        Unit(const std::string &str) : is_number(false), text(str) {}
        Unit(int64_t v) : is_number(true), integer(v) {}

    private:
        bool is_number;
        std::string text;
        int64_t integer;
    };

    class Ojbect : public Node
    {
    public:
        Ojbect(const std::string &str, Node *v) : member_variable_name(str), value(v){};
        std::string get_name() const
        {
            return member_variable_name;
        }

    private:
        std::string member_variable_name;
        Node *value;
    };

    class Array : public Node
    {
    public:
        Array(const std::vector<Node *> &ele) : elements(ele){};

    private:
        std::vector<Node *> elements;
    };

    class Group : public Node
    {
    public:
        Group(const std::map<std::string, Node *> &tab) : member_table(tab) {}

    private:
        std::map<std::string, Node *> member_table;
    };

    Node *parse_unit(Lexer::TokenStream &token_stream);

    Ojbect *parse_single(Lexer::TokenStream &ts)
    {

        auto variable_name = ts.current();
        ts.match(Lexer::STRING);
        ts.match(Lexer::COLON);
        return new Ojbect(Lexer::StringToken::get_content(variable_name), parse_unit(ts));
    }

    Array *parse_array(Lexer::TokenStream &ts)
    {
        ts.match(Lexer::LSB);
        std::vector<Node *> vec;
        while (true)
        {
            vec.push_back(parse_unit(ts));
            if (ts.get_cur_tag() != Lexer::COMMA)
                break;
        }

        ts.match(Lexer::RSB);
        return new Array(vec);
    }

    Group *parse_group(Lexer::TokenStream &ts)
    {
        ts.match(Lexer::BEGIN);
        std::map<std::string, Node *> table;
        while (true)
        {
            auto variable_name = ts.current();
            ts.match(Lexer::STRING);
            ts.match(Lexer::COLON);
            table.insert({Lexer::StringToken::get_content(variable_name), parse_unit(ts)});
            if (ts.get_cur_tag() != Lexer::COMMA)
                break;
        }

        ts.match(Lexer::END);
        return new Group(table);
    }

    Node *parse_unit(Lexer::TokenStream &ts)
    {
        switch (ts.get_cur_tag())
        {
        case Lexer::INTEGER:
        {
            auto v = Lexer::Integer::get_content(ts.current());
            ts.match(Lexer::INTEGER);
            return new Unit(v);
            break;
        }
        case Lexer::STRING:
        {
            auto front_part = ts.current();
            ts.match(Lexer::STRING);
            if (front_part->get_tag() == Lexer::COLON)
            {
                ts.match(Lexer::COMMA);
                return new Ojbect(Lexer::StringToken::get_content(front_part), parse_unit(ts));
            }
            else
            {
                return new Unit(Lexer::StringToken::get_content(front_part));
            }
            break;
        }
        case Lexer::LSB:
        {
            return parse_array(ts);
        }
        case Lexer::BEGIN:
        {
            return parse_group(ts);
        }
        default:
            throw std::runtime_error("json-sysntax error");
            break;
        }
    }

}

int main(int argc, char const *argv[])
{
    /* code */
    using namespace std;
    using namespace Lexer;
    auto ts = build_token_stream("{133\"hey\ng\tgg\"}");
    ts.print();
    return 0;
}
