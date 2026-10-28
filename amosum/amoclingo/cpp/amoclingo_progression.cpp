#include "amoclingo_algorithms.h"
#include <cctype>
#include <stdexcept>

// expression tree of a Progression; values saturate at +-2^62
struct Progression::Node{
    char op;                    // '0' number, 'n' the step, '+', '-', '*', '^', 'u' unary minus
    int64_t value = 0;
    std::shared_ptr<const Node> left, right;
};

namespace {

    constexpr int64_t CAP = int64_t(1) << 62;

    int64_t clampValue(const __int128 v) noexcept{ return v > CAP ? CAP : (v < -CAP ? -CAP : (int64_t)v); }

    int64_t power(int64_t base, int64_t exponent) noexcept{
        if(exponent < 0) return 0;
        __int128 result = 1;
        for(int64_t i = 0; i < exponent; ++i){
            result *= base;
            if(result > CAP || result < -CAP) return clampValue(result);
        }
        return (int64_t)result;
    }

    int64_t evaluate(const Progression::Node* node, const int64_t n) noexcept{
        switch(node->op){
            case '0': return node->value;
            case 'n': return n;
            case 'u': return -evaluate(node->left.get(), n);
            case '+': return clampValue((__int128)evaluate(node->left.get(), n) + evaluate(node->right.get(), n));
            case '-': return clampValue((__int128)evaluate(node->left.get(), n) - evaluate(node->right.get(), n));
            case '*': return clampValue((__int128)evaluate(node->left.get(), n) * evaluate(node->right.get(), n));
            default : return power(evaluate(node->left.get(), n), evaluate(node->right.get(), n));     // '^'
        }
    }

    // recursive descent:  expr := term (('+'|'-') term)*   term := unary ('*' unary)*
    //                     unary := '-' unary | power        power := atom ('^' unary)?   (right associative)
    //                     atom := number | 'n' | '(' expr ')'
    struct Parser{
        const std::string& text;
        size_t pos = 0;

        using NodePtr = std::shared_ptr<const Progression::Node>;

        [[noreturn]] void fail(const std::string& what) const{
            throw std::invalid_argument("--progression " + text + ": " + what + " at position " + std::to_string(pos + 1));
        }
        void skipSpaces(){ while(pos < text.size() && std::isspace((unsigned char)text[pos])) ++pos; }
        bool accept(const char c){ skipSpaces(); if(pos < text.size() && text[pos] == c){ ++pos; return true; } return false; }
        static NodePtr make(const char op, NodePtr left, NodePtr right = nullptr){
            auto node = std::make_shared<Progression::Node>(); node->op = op; node->left = std::move(left); node->right = std::move(right); return node;
        }

        NodePtr expr(){
            NodePtr node = term();
            while(true){
                if(accept('+')) node = make('+', node, term());
                else if(accept('-')) node = make('-', node, term());
                else return node;
            }
        }
        NodePtr term(){
            NodePtr node = unary();
            while(accept('*')) node = make('*', node, unary());
            return node;
        }
        NodePtr unary(){
            if(accept('-')) return make('u', unary());
            NodePtr base = atom();
            if(accept('^')) return make('^', base, unary());
            return base;
        }
        NodePtr atom(){
            skipSpaces();
            if(pos >= text.size()) fail("unexpected end");
            if(accept('(')){
                NodePtr node = expr();
                if(!accept(')')) fail("')' expected");
                return node;
            }
            if(accept('n')) return make('n', nullptr);
            if(!std::isdigit((unsigned char)text[pos])) fail(std::string("unexpected '") + text[pos] + "'");
            __int128 value = 0;
            while(pos < text.size() && std::isdigit((unsigned char)text[pos])) value = std::min<__int128>(value * 10 + (text[pos++] - '0'), CAP);
            auto node = std::make_shared<Progression::Node>(); node->op = '0'; node->value = (int64_t)value;
            return node;
        }
        NodePtr parse(){
            NodePtr node = expr();
            skipSpaces();
            if(pos != text.size()) fail(std::string("unexpected '") + text[pos] + "'");
            return node;
        }
    };

    bool usesStep(const Progression::Node* node) noexcept{
        if(node == nullptr) return false;
        return node->op == 'n' || usesStep(node->left.get()) || usesStep(node->right.get());
    }
}

Progression::Progression(const std::string& expression): expression(expression){
    Parser parser{expression};
    root = parser.parse();
    linear = !usesStep(root.get()) && at(0) == 1;
}

int64_t Progression::at(const size_t n) const noexcept{
    return std::max<int64_t>(1, evaluate(root.get(), (int64_t)std::min<size_t>(n, CAP)));
}
