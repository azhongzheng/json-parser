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
        {BEGIN, "{"}, {END, "}"}, {LSB, "["}, {RSB, "]"}, {COMMA, ","}, {COLON, ":"}, {END_TAG, "EOF"}};

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
        int cur_p = 0;
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
        STRING,
        INT,
        ARRAY,
        GROUP
    };

    class Node
    {
    public:
        Node(NodeType nt);
        int get_int();
        std::string get_str();
        Node *operator[](const std::string &str);
        Node *operator[](size_t idx);
        NodeType get_type() const { return type; }
        virtual ~Node();

    private:
        NodeType type;
    };

    class Unit : public Node
    {
    public:
        Unit(const std::string &str);
        Unit(int64_t v);
        static int64_t get_integer(Node *node);
        static std::string get_str(Node *node);
        ~Unit() {}

    private:
        bool is_number;
        std::string text;
        int64_t integer;
    };

    class Group : public Node
    {
    public:
        Group(const std::map<std::string, Node *> &tab);
        Node *operator[](const std::string &str) const;
        ~Group();

    private:
        std::map<std::string, Node *> member_table;
    };

    class Array : public Node
    {
    public:
        Array(const std::vector<Node *> &ele);
        Node *operator[](size_t idx) const;
        ~Array();

    private:
        std::vector<Node *> elements;
    };

    //=====================================
    // Node
    Node::Node(NodeType nt) : type(nt) {}
    int Node::get_int()
    {
        if (type == INT)
        {
            return Unit::get_integer(this);
        }
        else
            throw std::runtime_error("type not matched");
    }
    std::string Node::get_str()
    {
        if (type == STRING)
        {
            return Unit::get_str(this);
        }
        else
            throw std::runtime_error("type not matched");
    }
    Node *Node::operator[](const std::string &str)
    {
        if (type != GROUP)
        {
            throw std::runtime_error("type not matched, experted an array");
        }
        return static_cast<Group *>(this)->operator[](str);
    }
    Node *Node::operator[](size_t idx)
    {
        if (type != ARRAY)
        {
            throw std::runtime_error("type not matched, expected an array");
        }
        return static_cast<Array *>(this)->operator[](idx);
    }
    Node::~Node() {}
    // Unit
    Unit::Unit(const std::string &str) : Node(STRING), is_number(false), text(str) {}
    Unit::Unit(int64_t v) : Node(INT), is_number(true), integer(v) {}
    int64_t Unit::get_integer(Node *node)
    {
        return static_cast<Unit *>(node)->integer;
    }
    std::string Unit::get_str(Node *node)
    {
        return static_cast<Unit *>(node)->text;
    }

    //Array
    Array::Array(const std::vector<Node *> &ele) : Node(ARRAY), elements(ele){};
    Node *Array::operator[](size_t idx) const
    {
        if (idx > elements.size())
            throw std::runtime_error("Array out of range!");
        return elements[idx];
    }
    Array::~Array()
    {
        for (auto e : elements)
        {
            delete e;
        }
    }
    // Group
    Group::Group(const std::map<std::string, Node *> &tab) : Node(GROUP), member_table(tab) {}
    Node *Group::operator[](const std::string &str) const
    {
        auto it = member_table.find(str);
        if (it == member_table.end())
        {
            throw std::runtime_error("key " + str + " not found");
        }
        return it->second;
    }
    Group::~Group()
    {
        for (auto it : member_table)
            delete it.second;
    }

    Node *parse_unit(Lexer::TokenStream &token_stream);
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
            return (Node *)new Unit(v);
        }
        case Lexer::STRING:
        {
            auto front_part = ts.current();
            ts.match(Lexer::STRING);

            return (Node *)new Unit(Lexer::StringToken::get_content(front_part));
        }
        case Lexer::LSB:
        {
            return (Node *)parse_array(ts);
        }
        case Lexer::BEGIN:
        {
            return (Node *)parse_group(ts);
        }
        default:
            throw std::runtime_error("json-sysntax error");
            break;
        }
    }

}

class JSON
{
public:
    JSON(const std::string &str)
    {
        auto ts = Lexer::build_token_stream(str);
        node = Parser::parse_unit(ts);
    }
    ~JSON()
    {
        delete node;
    }

    int get_int()
    {
        return node->get_int();
    }
    std::string get_str()
    {
        return node->get_str();
    }
    Parser::Node *operator[](const std::string &str)
    {
        return node->operator[](str);
    }
    Parser::Node *operator[](size_t idx)
    {
        return node->operator[](idx);
    }

private:
    Parser::Node *node;
};

int main(int argc, char const *argv[])
{
    /* code */
    using namespace std;
    using namespace Lexer;
    auto ts = build_token_stream("{133\"hey\ng\tgg\"}");
    // ts.print();

    try
    {
        JSON js("[12, 23]");
        std::cout << js.get_int();
    }
    catch (const std::exception &e)
    {
        std::cout << e.what() << '\n';
    }

    return 0;
}
